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

void
noCarrier (dsd_opts * opts, dsd_state * state)
{
  unsigned int i;

  state->dibit_buf_p = state->dibit_buf + 200;
  for (i = 0; i < 200; i++) {
    state->dibit_buf[i] = 0;
  }
  if (opts->mbe_out_fd != -1) {
      closeMbeOutFile (opts, state);
  }
  state->inbuf_size = 512;
  state->inbuf_pos = 512;
  state->jitter = -1;
  state->lastsynctype = -1;
  state->carrier = 0;
  state->max = 15000;
  state->min = -15000;
  state->center = 0;
  state->err_str[0] = 0;
  strcpy (state->ftype, "       ");
  state->errs = 0;
  state->errs2 = 0;
  state->lasttg = 0;
  state->last_radio_id = 0;
  state->lastp25type = 0;
  state->repeat = 0;
  state->nac = 0;
  state->numtdulc = 0;
  strcpy (state->slot0light, " slot0 ");
  strcpy (state->slot1light, " slot1 ");
  state->firstframe = 0;
  if (opts->audio_gain == (float) 0) {
      state->aout_gain = 25;
  }
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_idx = 0;
  mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);
}

static inline void initOpts (dsd_opts * opts)
{
  opts->errorbars = 1;
  opts->datascope = 0;
  opts->verbose = 2;
  opts->audio_in_fd = -1;
  opts->mbe_out_dir[0] = 0;
  opts->mbe_out_path[0] = 0;
  opts->mbe_out_fd = -1;
  opts->audio_gain = 0;
  opts->wav_out_fd = -1;
  opts->uvquality = 3;
  //opts->mod_qpsk = 0;
  opts->inverted_dmr = 0;       // most transmitter + scanner + sound card combinations show non-inverted signals for this
  opts->mod_threshold = 26;
  opts->ssize = 36;
  opts->msize = 15;
}

static void initState (dsd_state * state)
{
  int i;

  //state->dibit_buf = malloc (sizeof (int) * 1048576);
  state->dibit_buf = mmap(NULL, sizeof (int) * 1048576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON|MAP_NORESERVE, -1, 0);
  state->dibit_buf_p = state->dibit_buf + 200;
  state->repeat = 0;
  state->audio_out_temp_buf_p = state->audio_out_temp_buf;
  //state->wav_out_bytes = 0;
  state->center = 0;
  state->jitter = -1;
  state->synctype = -1;
  state->min = -15000;
  state->max = 15000;
  state->lmid = 0;
  state->umid = 0;
  state->minref = -12000;
  state->maxref = 12000;
  state->lastsample = 0;
  for (i = 0; i < 128; i++) {
      state->sbuf[i] = 0;
  }
  state->sidx = 0;
  for (i = 0; i < 1024; i++) {
      state->maxbuf[i] = 15000;
      state->minbuf[i] = -15000;
  }
  state->midx = 0;
  state->err_str[0] = 0;
  strcpy (state->ftype, "          ");
  state->symbolcnt = 0;
  state->rf_mod = 2;
  state->numflips = 0;
  state->lastsynctype = -1;
  state->offset = 0;
  state->carrier = 0;
  state->lasttg = 0;
  state->talkgroup = 0;
  state->lasttg = 0;
  state->radio_id = 0;
  state->numtdulc = 0;
  state->errs = 0;
  state->errs2 = 0;
  state->optind = 0;
  state->firstframe = 0;
  strcpy (state->slot0light, " slot0 ");
  strcpy (state->slot1light, " slot1 ");
  state->aout_gain = 25;
  memset (state->aout_max_buf, 0, sizeof (float) * 200);
  state->aout_max_buf_idx = 0;
  state->samplesPerSymbol = 10;
  state->symbolCenter = 4;
  state->currentslot = 0;
  mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);

  state->debug_audio_errors = 0;
  state->debug_header_errors = 0;
  state->debug_header_critical_errors = 0;

#ifdef TRACE_DSD
  state->debug_sample_index = 0;
  state->debug_label_file = NULL;
  state->debug_label_dibit_file = NULL;
#endif
}

static void usage ()
{
  printf ("\n");
  printf ("Usage:\n");
  printf ("  dsd [options]            Live scanner mode\n");
  printf ("  dsd -h                   Show help\n");
  printf ("\n");
  printf ("Display Options:\n");
  printf ("  -e            Show Frame Info and errorbars (default)\n");
  printf ("  -q            Don't show Frame Info/errorbars\n");
  printf ("  -s            Datascope (disables other display options)\n");
  printf ("  -v <num>      Frame information Verbosity\n");
  printf ("\n");
  printf ("Input/Output options:\n");
  printf ("  -i <device>   Audio input device (default is /dev/audio, - for piped stdin)\n");
  printf ("  -d <dir>      Create mbe data files, use this directory\n");
  printf ("  -g <num>      Audio output gain (default = 0 = auto, disable = -1)\n");
  printf ("  -w <file>     Output synthesized speech to a .wav file\n");
  printf ("\n");
  printf ("Decoder options:\n");
  printf ("  -u <num>      Unvoiced speech quality (default=3)\n");
  printf ("  -xr           Expect inverted DMR/MOTOTRBO signal\n");
  printf ("\n");
  printf ("Advanced decoder options:\n");
  printf ("  -S <num>      Symbol buffer size for QPSK decision point tracking\n");
  printf ("                 (default=36)\n");
}

void
cleanupAndExit (dsd_opts * opts, dsd_state * state)
{
  noCarrier (opts, state);
  closeWavOutFile (opts);

  printf("\n");
  printf("Total audio errors: %i\n", state->debug_audio_errors);
  printf("Total header errors: %i\n", state->debug_header_errors);
  printf("Total irrecoverable header errors: %i\n", state->debug_header_critical_errors);

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
  rs6_init(&state.ReedSolomon_24_12_13, generator_polynomial_p25, 6);
  rs6_init(&state.ReedSolomon_24_16_09, generator_polynomial_p25, 4);
  rs6_init(&state.ReedSolomon_36_20_17, generator_polynomial_p25, 8);
#endif

  while ((c = getopt (argc, argv, "hep:qv:si:o:d:g:nw:B:C:R:f:u:x:S:")) != -1)
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
        case 'd':
          dsd_strlcpy(opts.mbe_out_dir, optarg, 1023);
          opts.mbe_out_dir[1023] = '\0';
          printf ("Writing mbe data files to directory %s\n", opts.mbe_out_dir);
          break;
        case 'g':
          opts.audio_gain = strtod(optarg, NULL);
          if (opts.audio_gain < (float) 0 )
            {
              printf ("Disabling audio out gain setting\n");
            }
          else if (opts.audio_gain == (float) 0)
            {
              opts.audio_gain = (float) 0;
              printf ("Enabling audio out auto-gain\n");
            }
          else
            {
              printf ("Setting audio out gain to %f\n", opts.audio_gain);
              state.aout_gain = opts.audio_gain;
            }
          break;
        case 'w':
          printf ("Writing audio to file %s\n", optarg);
          openWavOutFile (&opts, optarg);
          break;
        case 'f':
          printf ("Decoding only DMR/MOTOTRBO frames.\n");
          break;
        case 'u':
          opts.uvquality = strtoul(optarg, NULL, 10);
          if (opts.uvquality > 63) {
              opts.uvquality = 63;
          }
          printf ("Setting unvoice speech quality to %i waves per band.\n", opts.uvquality);
          break;
        case 'x':
          if (optarg[0] == 'r')
            {
              opts.inverted_dmr = 1;
              printf ("Expecting inverted DMR/MOTOTRBO signals.\n");
            }
          break;
        case 'S':
          opts.ssize = strtoul(optarg, NULL, 10);
          if (opts.ssize > 128) {
              opts.ssize = 128;
          } else if (opts.ssize < 1) {
              opts.ssize = 1;
          }
          printf ("Setting QPSK symbol buffer to %i\n", opts.ssize);
          break;
        default:
          usage ();
          exit (0);
        }
    }

  if (opts.wav_out_fd == -1) {
    openWavOutFile (&opts, "dsd.wav");
  }

  if (openAudioInDevice (&opts, audio_in_dev) < 0) {
    return -1;
  } else {
    printf ("Audio In Device: %s\n", audio_in_dev);
  }
  opts.audio_in_type |= 0x04;

  if (opts.datascope) {
    write(1, "\033[2J", 4);
  }

  while (1) {
      noCarrier (&opts, &state);
      state.synctype = getFrameSync (&opts, &state);
      // recalibrate center/umid/lmid
      state.center = ((state.max) + (state.min)) / 2;
      state.umid = (((state.max) - state.center) * 5 / 8) + state.center;
      state.lmid = (((state.min) - state.center) * 5 / 8) + state.center;
      while (state.synctype != -1) {
          if (!opts.datascope) {
              processFrame (&opts, &state);
          }
          state.synctype = getFrameSync (&opts, &state);
          // recalibrate center/umid/lmid
          state.center = ((state.max) + (state.min)) / 2;
          state.umid = (((state.max) - state.center) * 5 / 8) + state.center;
          state.lmid = (((state.min) - state.center) * 5 / 8) + state.center;
      }
  }

  cleanupAndExit (&opts, &state);
  return (0);
}

