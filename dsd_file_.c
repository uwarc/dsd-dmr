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

void saveAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d)
{
  int i, j, k;
  unsigned char b, buf[8];
  unsigned char err;

  err = (unsigned char) state->errs2;
  buf[0] = err;

  k = 0;
  for (i = 0; i < 6; i++) {
      b = 0;
      for (j = 0; j < 8; j++) {
          b = b << 1;
          b = b + ambe_d[k];
          k++;
      }
      buf[i+1] = b;
  }
  b = ambe_d[48];
  buf[7] = b;
  write(opts->mbe_out_fd, buf, 8);
}

void saveImbe4400Data (dsd_opts * opts, dsd_state * state, char *imbe_d)
{
  int i, j, k;
  unsigned char b, buf[12];
  unsigned char err;

  err = (unsigned char) state->errs2;
  buf[0] = err;

  k = 0;
  for (i = 0; i < 11; i++) {
      b = 0;
      for (j = 0; j < 8; j++) {
          b = b << 1;
          b = b + imbe_d[k];
          k++;
      }
      buf[i+1] = b;
  }
  write(opts->mbe_out_fd, buf, 12);
}

static unsigned int mbe_golay2312 (unsigned int *block)
{
  unsigned int i, errs = 0, in = *block;

  Golay23_Correct (block);

  in >>= 11;
  in ^= *block;
  for (i = 0; i < 12; i++) {
      if ((in >> i) & 1) {
          errs++;
      }
  }

  return (errs);
}

static void mbe_demodulateImbe7200x4400Data (char imbe[8][23])
{
  int i, j = 0, k;
  unsigned short pr[115], foo = 0;

  // create pseudo-random modulator
  for (i = 11; i >= 0; i--) {
      foo <<= 1;
      foo |= imbe[0][11+i];
  }

  pr[0] = (16 * foo);
  for (i = 1; i < 115; i++) {
      pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) >> 16));
  }
  for (i = 1; i < 115; i++) {
      pr[i] >>= 15;
  }

  // demodulate imbe with pr
  k = 1;
  for (i = 1; i < 4; i++) {
      for (j = 22; j >= 0; j--)
        imbe[i][j] ^= pr[k++];
  }
  for (i = 4; i < 7; i++) {
      for (j = 14; j >= 0; j--)
        imbe[i][j] ^= pr[k++];
  }
}

static int mbe_eccImbe7200x4400Data (char imbe_fr[8][23], char *imbe_d)
{
  int i, j = 0, errs = 0;
  unsigned int hin, block;
  char *imbe = imbe_d;

  for (j = 11; j >= 0; j--) {
    *imbe++ = imbe_fr[0][j+11];
  }

  for (i = 1; i < 4; i++) {
      block = 0;
      for (j = 22; j >= 0; j--) {
          block <<= 1;
          block |= imbe_fr[i][j];
      }
      errs += mbe_golay2312 (&block);
      for (j = 0; j < 12; j++) {
          *imbe++ = ((block >> j) & 1);
      }
  }
  for (i = 4; i < 7; i++) {
      hin = 0;
      for (j = 0; j < 15; j++) {
          hin <<= 1;
          hin |= imbe_fr[i][14-j];
      }
      block = hin;
      p25_hamming15_11_3_decode(&block);
      errs += ((hin >> 4) != block);
      for (j = 14; j >= 4; j--) {
          *imbe++ = ((block & 0x0400) >> 10);
          block = block << 1;
      }
  }
  for (j = 6; j >= 0; j--) {
      *imbe++ = imbe_fr[7][j];
  }

  return (errs);
}

void
process_IMBE (dsd_opts* opts, dsd_state* state, char imbe_fr[8][23])
{
  //if (state->p25kid == 0)
  {
    // Check for a non-standard c0 transmitted. This is explained here: https://github.com/szechyjs/dsd/issues/24
    unsigned int nsw = 0x00038000;
    unsigned int block = 0;
    int j;
    for (j = 22; j >= 0; j--) {
        block <<= 1;
        block |= imbe_fr[0][j];
    }

    if (block == nsw) {
        // Skip this particular value. If we let it pass it will be signaled as an erroneus IMBE
        printf("(Non-standard IMBE c0 detected, skipped)\n");
    } else {
        char imbe_d[88];
        state->errs2 = mbe_golay2312 (&block);
        for (j = 11; j >= 0; j--) {
            imbe_fr[0][j+11] = (block & 0x0800) >> 11;
            block <<= 1;
        }
        for (j = 0; j < 88; j++) {
            imbe_d[j] = 0;
        }
        mbe_demodulateImbe7200x4400Data (imbe_fr);
        state->errs2 += mbe_eccImbe7200x4400Data (imbe_fr, imbe_d);
        processIMBEFrame (opts, state, imbe_d);
    }
  }
}

void demodAmbe3600x24x0Data (int *errs2, char ambe_fr[4][24], char *ambe_d)
{
  int i, j, k;
  unsigned int block = 0, errs = 0;
  unsigned short pr[115], foo;
  char *ambe = ambe_d;

  for (j = 22; j >= 0; j--) {
      block <<= 1;
      block |= ambe_fr[0][j+1];
  }

  errs = mbe_golay2312(&block);

  // create pseudo-random modulator
  foo = block;
  pr[0] = (16 * foo);
  for (i = 1; i < 24; i++) {
      pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) >> 16));
  }
  for (i = 1; i < 24; i++) {
      pr[i] >>= 15;
  }

  // just copy C0
  for (j = 11; j >= 0; j--) {
      *ambe++ = ((block >> j) & 1);
  }

  // demodulate C1 with pr
  // then, ecc and copy C1
  k = 1;
  for (i = 22; i >= 0; i--) {
      block <<= 1;
      block |= (ambe_fr[1][i] ^ pr[k++]);
  }

  errs += mbe_golay2312(&block);
  for (j = 11; j >= 0; j--) {
      *ambe++ = ((block >> j) & 1);
  }

  // just copy C2
  for (j = 10; j >= 0; j--) {
      *ambe++ = ambe_fr[2][j];
  }

  // just copy C3
  for (j = 13; j >= 0; j--) {
      *ambe++ = ambe_fr[3][j];
  }

  *errs2 = errs;
}

void mbe_printVersion (char *str)
{
  strcpy(str, "Dummy mbelib, .amb/.imb file write only, no decoding to audio supported");
}

void mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced)
{
}

void
processAMBEFrame (dsd_opts * opts, dsd_state * state, char ambe_fr[4][24])
{
  char ambe_d[49];

  demodAmbe3600x24x0Data (&state->errs2, ambe_fr, ambe_d);
  if (opts->mbe_out_fd != -1) {
      saveAmbe2450Data (opts, state, ambe_d);
  }
  state->debug_audio_errors += state->errs2;
}

void
processIMBEFrame (dsd_opts * opts, dsd_state * state, char imbe_d[88])
{
  if (opts->mbe_out_fd != -1) {
      saveImbe4400Data (opts, state, imbe_d);
  }
  state->debug_audio_errors += state->errs2;
}

void
closeMbeOutFile (dsd_opts * opts, dsd_state * state)
{
  unsigned int is_imbe = !(state->synctype & ~1U); // Write IMBE magic if synctype == 0 or 1
  char new_path[1024];
  time_t tv_sec;
  struct tm timep;
  int result;

  if (opts->mbe_out_fd != -1) {
      tv_sec = opts->mbe_out_last_timeval;

      close(opts->mbe_out_fd);
      opts->mbe_out_fd = -1;
      gmtime_r(&tv_sec, &timep);
      snprintf (new_path, 1023, "%s/nac0-%04u-%02u-%02u-%02u:%02u:%02u-tg%u-src%u.%cmb", opts->mbe_out_dir,
                timep.tm_year + 1900, timep.tm_mon + 1, timep.tm_mday,
                timep.tm_hour, timep.tm_min, timep.tm_sec, state->talkgroup, state->radio_id, (is_imbe ? 'i' : 'a'));
      result = rename (opts->mbe_out_path, new_path);
  }
}

void
openMbeOutFile (dsd_opts * opts, dsd_state * state)
{
  struct timeval tv;
  unsigned int is_imbe = !(state->synctype & ~1U); // Write IMBE magic if synctype == 0 or 1
  char magic[4] = { '.', 'a', 'm', 'b' };

  gettimeofday (&tv, NULL);
  opts->mbe_out_last_timeval = tv.tv_sec;
  snprintf (opts->mbe_out_path, 1023, "%s/%ld.%cmb", opts->mbe_out_dir, tv.tv_sec, (is_imbe ? 'i' : 'a'));

  if ((opts->mbe_out_fd = open (opts->mbe_out_path, O_WRONLY | O_CREAT, 0644)) < 0) {
      printf ("Error, couldn't open %s\n", opts->mbe_out_path);
      return;
  }

  // write magic
  // 0x626D612E / 0x626D692E
  if ((state->synctype == 0) || (state->synctype == 1)) {
      magic[1] = 'i';
  }
  write (opts->mbe_out_fd, magic, 4);
}

