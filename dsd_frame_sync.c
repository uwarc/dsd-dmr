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

void Shellsort_int(int *in, unsigned int n)
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
   *  0 = +P25p1
   *  1 = -P25p1
   *  2 = +X2-TDMA (non inverted signal data frame)
   *  3 = -X2-TDMA (inverted signal data frame)
   *  4 = +DMR (non inverted signal data frame)
   *  5 = -DMR (inverted signal data frame)
   *  6 = +NXDN (non inverted data frame)
   *  7 = -NXDN (inverted data frame)
   *  8 = +D-STAR
   *  9 = -D-STAR
   * 10 = +D-STAR_HD
   * 11 = -D-STAR_HD
   * 12 = +DMR (non inverted signal voice frame)
   * 13 = -DMR (inverted signal voice frame)
   * 14 = +X2-TDMA (non inverted signal voice frame)
   * 15 = -X2-TDMA (inverted signal voice frame)
   * 16 = +NXDN (non inverted voice frame)
   * 17 = -NXDN (inverted voice frame)
   */

  int i, t, dibit, sync, symbol, synctest_pos, lastt;
  char synctest[25];
  char *synctest_p;
  char synctest_buf[10240];
  int lmin, lmax, lidx;
  int lbuf[24], lbuf2[24];

  for (i = 18; i < 24; i++) {
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

  while (sync == 0) {
      t++;
      symbol = getSymbol (opts, state, 0);

      lbuf[lidx] = symbol;
      state->sbuf[state->sidx] = symbol;
      if (lidx == 23) {
          lidx = 0;
      } else {
          lidx++;
      }

      if (state->sidx == (state->ssize - 1)) {
          state->sidx = 0;
      } else {
          state->sidx++;
      }

      if (lastt == 23) {
          lastt = 0;
          state->rf_mod = 2;
      } else {
          lastt++;
      }

      //determine dibit state
      if (symbol > 0) {
          dibit = 49;               // '1'
      } else {
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
          if (opts->datascope && (lidx == 0)) {
            print_datascope(state, lbuf2, 24);
          }

          memcpy (synctest, (synctest_p - 23), 24);

          if ((memcmp (synctest, P25P1_SYNC, 24) == 0) || (memcmp (synctest, INV_P25P1_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            strcpy (state->ftype, "P25p1");
            if (synctest[0] == '1') {
                state->lastsynctype = 0;
            } else {
                state->lastsynctype = 1;
            }
            goto update_sync_and_return;
          }

          if ((memcmp (synctest, DMR_MS_DATA_SYNC, 24) == 0) || (memcmp (synctest, DMR_BS_DATA_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            state->dmrMsMode = (synctest[22] == '1');
            strcpy (state->ftype, "DMR  ");
            if (opts->inverted_dmr == 0) {
                // data frame
                if (state->lastsynctype != 4) {
                    state->firstframe = 1;
                }
                state->lastsynctype = 4;
            } else {
                // inverted voice frame
                if (state->lastsynctype != 13) {
                    state->firstframe = 1;
                }
                state->lastsynctype = 13;
            }
            goto update_sync_and_return;
          }

          if ((memcmp (synctest, DMR_MS_VOICE_SYNC, 24) == 0) || (memcmp (synctest, DMR_BS_VOICE_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            state->dmrMsMode = (synctest[22] == '3');
            strcpy (state->ftype, "DMR  ");
            if (opts->inverted_dmr == 0) {
                // voice frame
                if (state->lastsynctype != 12) {
                   state->firstframe = 1;
                }
                state->lastsynctype = 12;
            } else {
                // inverted data frame
                if (state->lastsynctype != 5) {
                    state->firstframe = 1;
                }
                state->lastsynctype = 5;
            }
            goto update_sync_and_return;
          }

          if ((memcmp (synctest, X2TDMA_BS_DATA_SYNC, 24) == 0) || (memcmp (synctest, X2TDMA_MS_DATA_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            state->dmrMsMode = (synctest[22] == '1');
            strcpy (state->ftype, "X2-TDMA ");
            if (opts->inverted_x2tdma == 0) {
                // data frame
                state->lastsynctype = 2;
            } else {
                // inverted voice frame
                if (state->lastsynctype != 15) {
                   state->firstframe = 1;
                }
                state->lastsynctype = 15;
            }
            goto update_sync_and_return;
          }

          if ((memcmp (synctest, X2TDMA_BS_VOICE_SYNC, 24) == 0) || (memcmp (synctest, X2TDMA_MS_VOICE_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            state->dmrMsMode = (synctest[22] == '3');
            strcpy (state->ftype, "X2-TDMA ");
            if (opts->inverted_x2tdma == 0) {
                // voice frame
                if (state->lastsynctype != 14) {
                   state->firstframe = 1;
                }
                state->lastsynctype = 14;
            } else {
                // inverted data frame
                state->lastsynctype = 3;
            }
            goto update_sync_and_return;
          }

          if ((memcmp (synctest+6, NXDN_BS_VOICE_SYNC, 18) == 0) ||
              (memcmp (synctest+6, NXDN_MS_VOICE_SYNC, 18) == 0) ||
              (memcmp (synctest+6, NXDN_BS_DATA_SYNC, 18) == 0)  ||
              (memcmp (synctest+6, NXDN_MS_DATA_SYNC, 18) == 0)) {
              if (synctest[21] == '1') {
                  state->lastsynctype = 6;
              } else {
                  state->lastsynctype = 16;
              }
              if ((state->lastsynctype == 6) || (state->lastsynctype == 16)) {
                  state->offset = synctest_pos;
                  strcpy (state->ftype, "NXDN96  ");
                  goto update_sync_and_return;
              }
          }
          if ((memcmp (synctest+6, INV_NXDN_BS_VOICE_SYNC, 18) == 0) || 
              (memcmp (synctest+6, INV_NXDN_MS_VOICE_SYNC, 18) == 0) ||
              (memcmp (synctest+6, INV_NXDN_BS_DATA_SYNC, 18) == 0) ||
              (memcmp (synctest+6, INV_NXDN_MS_DATA_SYNC, 18) == 0)) {
              if (synctest[21] == '3') {
                  state->lastsynctype = 7;
              } else {
                  state->lastsynctype = 17;
              }
              if ((state->lastsynctype == 7) || (state->lastsynctype == 17)) {
                  state->offset = synctest_pos;
                  strcpy (state->ftype, "NXDN96  ");
                  goto update_sync_and_return;
              }
          }

          if ((memcmp (synctest, DSTAR_SYNC, 24) == 0) || (memcmp (synctest, INV_DSTAR_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            strcpy (state->ftype, "D-STAR  ");
            if (synctest[0] == '3') {
                state->lastsynctype = 8;
            } else {
                state->lastsynctype = 9;
            }
            goto update_sync_and_return;
          }
          if ((memcmp (synctest, DSTAR_HD_SYNC, 24) == 0) || (memcmp (synctest, INV_DSTAR_HD_SYNC, 24) == 0)) {
            state->offset = synctest_pos;
            strcpy (state->ftype, "D-STARHD");
            if (synctest[0] == '1') {
                state->lastsynctype = 10;
            } else {
                state->lastsynctype = 11;
            }
            goto update_sync_and_return;
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
          state->lastsynctype = -1;
      }

      if (state->lastsynctype != -1) {
          //if (synctest_pos >= 1800) {
          if (synctest_pos >= 9000) {
              if ((state->lastsynctype != -1) && !(opts->datascope)) {
                  printf ("Sync: no sync\n");
              }
              state->lastsynctype = -1;
#if 1
              state->max = 3000;
              state->min = -3000;
              state->umid = 1000;
              state->lmid = -1000;
              state->center = 0;
#endif
              return (-1);
          }
      }
  }

  return (-1);

update_sync_and_return:
   // recalibrate center/umid/lmid
   state->max = ((state->max) + (lmax)) / 2;
   state->min = ((state->min) + (lmin)) / 2;
   state->center = ((state->max) + (state->min)) / 2;
   state->umid = (((state->max) - state->center) * 5 / 8) + state->center;
   state->lmid = (((state->min) - state->center) * 5 / 8) + state->center;
   return state->lastsynctype;
}

