/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "dsd.h"
#include <sys/mman.h>

#ifndef NO_REEDSOLOMON
/* specify irreducible polynomial coeffts */
static unsigned int generator_polynomial_dmr = 0x11D; /* MM = 8, TT = 2 */
static unsigned int generator_polynomial_p25 = 0x43; /* MM = 6, TT = 8 */
#endif

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t dsd_strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and return */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		return siz;
	}

	return(s - src - 1);	/* count does not include NUL */
}

uint32_t dsd_div32(uint32_t num, uint32_t den, uint32_t *rem)
{
    uint32_t bit = 1;
    uint32_t quo;

    if (den > num) {
        if (rem) {
            *rem = num;
        }
        return 0;
    }

    /* normalize to make den >= num, but not overflow */
    while ((den < num) && !(den >> 31)) {
        bit <<= 1;
        den <<= 1;
    }
    quo = 0;

    /* generate quotient, one bit at a time */
    while (bit) {
        if (den <= num) {
            num -= den;
            quo += bit;
        }
        bit >>= 1;
        den >>= 1;
    }

    if (rem) {
        *rem = num;
    }
    return quo;
}

void
noCarrier (dsd_opts * opts, dsd_state * state)
{
  unsigned int i;

  state->dibit_buf_p = state->dibit_buf + 128;
  for (i = 0; i < 128; i++) {
    state->dibit_buf[i] = 0;
  }
  state->lastsynctype = -1;
  strcpy (state->ftype, "       ");
  state->errs2 = 0;
  state->talkgroup = 0;
  state->radio_id = 0;
  state->lastp25type = 0;
  state->numtdulc = 0;
  strcpy (state->slot0light, " slot0 ");
  strcpy (state->slot1light, " slot1 ");
  state->firstframe = 0;
}

static inline void initOpts (dsd_opts * opts)
{
  opts->errorbars = 1;
  opts->datascope = 0;
  opts->verbose = 2;
  opts->p25enc = 0;
  opts->p25status = 0;
  opts->p25tg = 0;
  opts->audio_in_fd = -1;
  opts->audio_in_format = 0;
  opts->mbe_out_dir[0] = 0;
  opts->mbe_out_path[0] = 0;
  opts->mbe_out_fd = -1;
  opts->agc_enable = 1;
  opts->audio_gain = 0;
  opts->wav_out_fd = -1;
  opts->uvquality = 3;
  //opts->mod_qpsk = 0;
  opts->inverted_dmr = 0;       // most transmitter + scanner + sound card combinations show non-inverted signals for this
  opts->inverted_x2tdma = 1;    // most transmitter + scanner + sound card combinations show inverted signals for this
  opts->msize = 16;
}

static void initState (dsd_state * state)
{
  int i;

  state->dibit_buf_p = state->dibit_buf + 128;
  state->audio_out_temp_buf_p = state->audio_out_temp_buf;
  //state->wav_out_bytes = 0;
  state->inbuf_size = 1024;
  state->inbuf_pos = 1024;
  state->samplesPerSymbol = 10;
  state->synctype = -1;
  state->lastsynctype = -1;
  state->min = -15000;
  state->max = 15000;
  state->lmid = 0;
  state->umid = 0;
  state->center = 0;
  for (i = 0; i < 128; i++) {
      state->sbuf[i] = 0;
  }
  state->sidx = 0;
  state->ssize = 36;
  state->d_history_last = 0;
  state->d_symbol_clock = 0.0f;
  state->d_symbol_spread = 2.0f; // nominal symbol spread of 2.0 gives outputs at -3, -1, +1, +3
  state->d_symbol_time = (1.0f / (float)state->samplesPerSymbol);
  state->fine_frequency_correction = 0.0f;
  for (i = 0; i <= FSK4_NTAPS; i++) {
      state->d_history[i] = 0.0f;
  }
  for (i = 0; i < 1024; i++) {
      state->maxbuf[i] = 15000;
      state->minbuf[i] = -15000;
  }
  state->midx = 0;
  strcpy (state->ftype, "          ");
  state->symbolcnt = 0;
  state->rf_mod = 2;
  state->offset = 0;
  state->talkgroup = 0;
  state->radio_id = 0;
  state->numtdulc = 0;
  state->errs2 = 0;
  state->firstframe = 0;
  strcpy (state->slot0light, " slot0 ");
  strcpy (state->slot1light, " slot1 ");
  state->aout_gain = 25;
  for (i = 0; i < 25; i++) {
    state->aout_max_buf[i] = 0.0f;
  }
  state->aout_max_buf_idx = 0;
  state->currentslot = 0;
  state->dmrMsMode = 0;
  mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);

  state->debug_audio_errors = 0;
  state->debug_header_errors = 0;

#ifdef TRACE_DSD
  state->debug_sample_index = 0;
  state->debug_label_file = NULL;
  state->debug_label_dibit_file = NULL;
#endif
}

static const unsigned char static_hdr_portion[8] = {
   0x52, 0x49, 0x46, 0x46, 0xFF, 0xFF, 0xFF, 0x7F
};

static void write_wav_header(int fd, uint32_t rate)
{
   WAVHeader w;
   write(fd, &static_hdr_portion, 8);

   w.pad0 = 0x45564157;
   w.pad1 = 0x20746D66;
   w.hdr_len = 0x10;
   w.wav_id = 1;
   w.channels = 1;
   w.samplerate = rate;
   w.bitrate = rate*2;
   w.block_align = 0x00100010;
   w.data_elem = 0x61746164;
   w.data_len = 0x7fffffff;
   write(fd, &w, sizeof(WAVHeader));
}

static void usage ()
{
  const char *usage_str = "Usage:\n"
  "  dsd [options]            Live scanner mode\n"
  "  dsd -h                   Show help\n\n"
  "Display Options:\n"
  "  -e            Show Frame Info and errorbars (default)\n"
  "  -q            Don't show Frame Info/errorbars\n"
  "  -s            Datascope (disables other display options)\n"
  "  -v <num>      Frame information Verbosity\n\n"
  "Input/Output options:\n"
  "  -i <device>   Audio input device (default is /dev/audio, - for piped stdin)\n"
  "  -d <dir>      Create mbe data files, use this directory\n"
  "  -g <num>      Audio output gain (default = 0 = auto, disable = -1)\n"
  "  -w <file>     Output synthesized speech to a .wav file\n\n"
  "Decoder options:\n"
  "  -u <num>      Unvoiced speech quality (default=3)\n"
  "  -xx           Expect non-inverted X2-TDMA signal\n"
  "  -xr           Expect inverted DMR/MOTOTRBO signal\n\n"
  "Advanced decoder options:\n"
  "  -S <num>      Symbol buffer size for QPSK decision point tracking\n"
  "                 (default=36)\n";
  write(1, usage_str, strlen(usage_str));
}

void
cleanupAndExit (dsd_opts * opts, dsd_state * state)
{
  if (opts->mbe_out_fd != -1) {
    closeMbeOutFile (opts, state);
  }
  close(opts->wav_out_fd);

  printf("\nTotal audio errors: %u\nTotal header errors: %u\n", state->debug_audio_errors, state->debug_header_errors);

#ifdef TRACE_DSD
  if (state->debug_label_file != NULL) {
      fclose(state->debug_label_file);
      state->debug_label_file = NULL;
  }
  if(state->debug_label_dibit_file != NULL) {
      fclose(state->debug_label_dibit_file);
      state->debug_label_dibit_file = NULL;
  }
#endif

  close(opts->audio_in_fd);
  printf ("Exiting.\n");
  exit (0);
}

void
sigfun (int sig)
{
    exitflag = 1;
}

int
main (int argc, char **argv)
{
  struct sigaction vec;
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  char *audio_in_dev = NULL;
  dsd_opts opts;
  dsd_state state;
  char versionstr[25];
  mbe_printVersion (versionstr);

  printf ("Digital Speech Decoder 1.7.0-dev\n");
  printf ("mbelib version %s\n", versionstr);

  initOpts (&opts);
  initState (&state);

  exitflag = 0;

  vec.sa_handler = sigfun;
  sigemptyset(&vec.sa_mask);
  vec.sa_flags = 0;
  sigaction(SIGINT, &vec, NULL);
  sigaction(SIGTERM, &vec, NULL);
  vec.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &vec, NULL);

#ifndef NO_REEDSOLOMON
  rs8_init(&state.ReedSolomon_12_09_04, generator_polynomial_dmr, 2);
  //rs6_init(&state.ReedSolomon_24_12_13, generator_polynomial_p25, 6);
  //rs6_init(&state.ReedSolomon_24_16_09, generator_polynomial_p25, 4);
  //rs6_init(&state.ReedSolomon_36_20_17, generator_polynomial_p25, 8);
#endif

  while ((c = getopt (argc, argv, "hep:qv:si:t:d:g:nw:B:C:R:f:u:x:S:A:M:")) != -1)
    {
      opterr = 0;
      switch (c)
        {
        case 'h':
          usage ();
          exit (0);
        case 'e':
          opts.errorbars = 1;
          break;
        case 'p':
          if (optarg[0] == 'e') {
              opts.p25enc = 1;
          } else if (optarg[0] == 's') {
              opts.p25status = 1;
          } else if (optarg[0] == 't') {
              opts.p25tg = 1;
          }
          break;
        case 'q':
          opts.errorbars = 0;
          opts.verbose = 0;
          break;
        case 's':
          opts.errorbars = 0;
          opts.datascope = 1;
          break;
        case 'v':
          opts.verbose = strtoul(optarg, NULL, 10);
          break;
        case 'i':
          audio_in_dev = optarg;
          break;
        case 't':
          opts.audio_in_format = strtoul(optarg, NULL, 10);
          if (opts.audio_in_format > 4) {
            printf("Error: invalid choice %u for audio input format! (Must be 0-4)\n",
                   opts.audio_in_format);
            return -1;
          }
          break;
        case 'd':
          dsd_strlcpy(opts.mbe_out_dir, optarg, 1023);
          opts.mbe_out_dir[1023] = '\0';
          printf ("Writing mbe data files to directory %s\n", opts.mbe_out_dir);
          break;
        case 'g':
          opts.agc_enable = strtoul(optarg, NULL, 10);
          if (!opts.agc_enable) {
              printf ("Disabling audio out gain setting\n");
          } else {
              printf ("Enabling audio out auto-gain\n");
          } 
          break;
        case 'w':
          printf ("Writing audio to file %s\n", optarg);
          if ((opts.wav_out_fd = open(optarg, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) {
            printf ("Error - could not open wav output file %s\n", optarg);
          }
          write_wav_header(opts.wav_out_fd, 8000);
          break;
        case 'f':
          break;
        case 'u':
          opts.uvquality = strtoul(optarg, NULL, 10);
          if (opts.uvquality > 63) {
              opts.uvquality = 63;
          }
          printf ("Setting unvoice speech quality to %i waves per band.\n", opts.uvquality);
          break;
        case 'x':
          if (optarg[0] == 'x') {
              opts.inverted_x2tdma = 0;
              printf ("Expecting non-inverted X2-TDMA signals.\n");
          } else if (optarg[0] == 'r') {
              opts.inverted_dmr = 1;
              printf ("Expecting inverted DMR/MOTOTRBO signals.\n");
          }
          break;
        case 'M':
          opts.msize = strtoul(optarg, NULL, 10);
          if (opts.msize > 1024) {
            opts.msize = 1024;
          } else if (opts.msize < 1) {
            opts.msize = 1;
          }
          printf ("Setting QPSK Min/Max buffer to %i\n", opts.msize);
          break;
        case 'S':
          state.ssize = strtoul(optarg, NULL, 10);
          if (state.ssize > 128) {
              state.ssize = 128;
          } else if (state.ssize < 1) {
              state.ssize = 1;
          }
          printf ("Setting QPSK symbol buffer to %i\n", state.ssize);
          break;
        default:
          usage ();
          exit (0);
        }
    }

  if (openAudioInDevice (&opts, audio_in_dev) < 0) {
    printf ("Error, couldn't open file %s\n", audio_in_dev);
    return -1;
  } else {
    printf ("Audio In Device: %s\n", audio_in_dev);
  }

  if (opts.datascope) {
    write(1, "\033[2J", 4);
  }
 
  while (1) {
      noCarrier (&opts, &state);
      state.synctype = getFrameSync (&opts, &state);
      while (state.synctype != -1) {
          if (!opts.datascope) {
              processFrame (&opts, &state);
          }
          state.synctype = getFrameSync (&opts, &state);
      }
  }

  cleanupAndExit (&opts, &state);
  return (0);
}

