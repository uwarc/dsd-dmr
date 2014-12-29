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

static void Shellsort_int(int *in, unsigned int n)
{
  int i, j;
  int v;
  unsigned int inc = 1;

  do {
    inc = 3 * inc + 1;
  } while (inc <= n);

  do {
    inc = inc / 3;
    for (i = inc + 1; i <= n; i++) {
      v = in[i-1];
      j = i;
      while (in[j-inc-1] > v) {
        in[j-1] = in[j-inc-1];
        j -= inc;
        if (j <= inc)
          break;
      }
      in[j-1] = v;
    }
  } while (inc > 1);
}

int
getFrameSync (dsd_opts * opts, dsd_state * state)
{
  /* detects frame sync and returns frame type
   *  6 = +D-STAR
   *  7 = -D-STAR
   *  8 = +NXDN (non inverted voice frame)
   *  9 = -NXDN (inverted voice frame)
   * 10 = +DMR (non inverted signal data frame)
   * 11 = -DMR (inverted signal voice frame)
   * 12 = +DMR (non inverted signal voice frame)
   * 13 = -DMR (inverted signal data frame)
   * 16 = +NXDN (non inverted data frame)
   * 17 = -NXDN (inverted data frame)
   * 18 = +D-STAR_HD
   * 19 = -D-STAR_HD
   */

  int i, t, dibit, sync, symbol, synctest_pos, lastt;
  char synctest[25];
  char *synctest_p;
  char synctest_buf[10240];
  int lmin, lmax, lidx;
  int lbuf[24], lbuf2[24];

  for (i = 18; i < 24; i++)
    {
      lbuf[i] = 0;
      lbuf2[i] = 0;
    }

  // detect frame sync
  t = 0;
  synctest[24] = 0;
  synctest_pos = 0;
  synctest_p = synctest_buf;
  sync = 0;
  lmin = 0;
  lmax = 0;
  lidx = 0;
  lastt = 0;
  state->numflips = 0;

  while (sync == 0) {
      t++;
      symbol = getSymbol (opts, state, 0);

      lbuf[lidx] = symbol;
      state->sbuf[state->sidx] = symbol;
      if (lidx == 23)
        {
          lidx = 0;
        }
      else
        {
          lidx++;
        }
      if (state->sidx == (opts->ssize - 1))
        {
          state->sidx = 0;
        }
      else
        {
          state->sidx++;
        }

      if (lastt == 23)
        {
          lastt = 0;
          if (state->numflips > 18)
            {
                state->rf_mod = 2;
            }
          else
            {
                state->rf_mod = 0;
            }
          state->numflips = 0;
        }
      else
        {
          lastt++;
        }

      //if (state->dibit_buf_p > state->dibit_buf + 900000) {
      //if (state->dibit_buf_p > state->dibit_buf + 72000) {
      if (state->dibit_buf_p > state->dibit_buf + 48000) {
    	  state->dibit_buf_p = state->dibit_buf + 200;
      }

      //determine dibit state
      if (symbol > 0)
        {
          *state->dibit_buf_p = 1;
          state->dibit_buf_p++;
          dibit = 49;               // '1'
        }
      else
        {
          *state->dibit_buf_p = 3;
          state->dibit_buf_p++;
          dibit = 51;               // '3'
        }

      *synctest_p = dibit;
      if (t >= 18) {
          for (i = 0; i < 24; i++) {
              lbuf2[i] = lbuf[i];
          }

          Shellsort_int (lbuf2, 24);
          lmin = (lbuf2[2] + lbuf2[3] + lbuf2[4]) / 3;
          lmax = (lbuf2[21] + lbuf2[20] + lbuf2[19]) / 3;
          state->maxref = state->max;
          state->minref = state->min;
          if (opts->datascope) {
            print_datascope(state, lidx, lbuf2);
          }

          //strncpy (synctest, (synctest_p - 23), 24);
          memcpy (synctest, (synctest_p - 23), 24);
          if ((memcmp (synctest, DMR_MS_DATA_SYNC, 24) == 0) || (memcmp (synctest, DMR_BS_DATA_SYNC, 24) == 0)) {
            state->carrier = 1;
            state->offset = synctest_pos;
            state->max = ((state->max) + (lmax)) / 2;
            state->min = ((state->min) + (lmin)) / 2;
            if (opts->inverted_dmr == 0) {
                // data frame
                strcpy (state->ftype, " +DMR      ");
                state->lastsynctype = 10;
                return (10);
            } else {
                // inverted voice frame
                strcpy (state->ftype, " -DMR      ");
                if (state->lastsynctype != 11) {
                    state->firstframe = 1;
                }
                state->lastsynctype = 11;
                return (11);
            }
          }
          if ((memcmp (synctest, DMR_MS_VOICE_SYNC, 24) == 0) || (memcmp (synctest, DMR_BS_VOICE_SYNC, 24) == 0)) {
            state->carrier = 1;
            state->offset = synctest_pos;
            state->max = ((state->max) + lmax) / 2;
            state->min = ((state->min) + lmin) / 2;
            if (opts->inverted_dmr == 0) {
                // voice frame
                strcpy (state->ftype, " +DMR      ");
                if (state->lastsynctype != 12) {
                   state->firstframe = 1;
                }
                state->lastsynctype = 12;
                return (12);
            } else {
                // inverted data frame
                strcpy (state->ftype, " -DMR      ");
                state->lastsynctype = 13;
                return (13);
            }
          }

          if ((t == 24) && (state->lastsynctype != -1)) {
              if ((state->lastsynctype == 11) && ((memcmp (synctest, DMR_BS_VOICE_SYNC, 24) != 0) || (memcmp (synctest, DMR_MS_VOICE_SYNC, 24) != 0))) {
                  state->carrier = 1;
                  state->offset = synctest_pos;
                  state->max = ((state->max) + lmax) / 2;
                  state->min = ((state->min) + lmin) / 2;
                  strcpy (state->ftype, " (DMR)     ");
                  state->lastsynctype = -1;
                  return (11);
              } else if ((state->lastsynctype == 12) && ((memcmp (synctest, DMR_BS_DATA_SYNC, 24) != 0) || (memcmp (synctest, DMR_MS_DATA_SYNC, 24) != 0))) { 
                  state->carrier = 1;
                  state->offset = synctest_pos;
                  state->max = ((state->max) + lmax) / 2;
                  state->min = ((state->min) + lmin) / 2;
                  strcpy (state->ftype, " (DMR)     ");
                  state->lastsynctype = -1;
                  return (12);
              }
          }
      }

      if (exitflag == 1) {
          cleanupAndExit (opts, state);
      }

      if (synctest_pos < 10200) {
          synctest_pos++;
          synctest_p++;
      } else {
          // buffer reset
          synctest_pos = 0;
          synctest_p = synctest_buf;
          noCarrier (opts, state);
      }

      if (state->lastsynctype != -1) {
          if (synctest_pos >= 1800) {
              if (state->carrier == 1) {
                  printf ("Sync: no sync\n");
              }
              noCarrier (opts, state);
              return (-1);
          }
      }
  }

  return (-1);
}

