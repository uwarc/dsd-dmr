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

#include <assert.h>
#include "dsd.h"

static int
invert_dibit(int dibit)
{
    return (dibit ^ 2);
}

static int digitize (dsd_opts* opts, dsd_state* state, int symbol)
{
  // determine dibit state
  if ((state->synctype == 6) || (state->synctype == 7) ||
      (state->synctype == 14) || (state->synctype == 15) || 
      (state->synctype == 18) || (state->synctype == 19)) {
      //  6 +D-STAR
      //  7 -D-STAR
      // 14 +ProVoice
      // 15 -ProVoice
      // 18 +D-STAR_HD
      // 19 -D-STAR_HD
      int dibit;
      if (symbol > state->center) {
        *state->dibit_buf_p++ = 1;
        dibit = 0;
      } else {
        *state->dibit_buf_p++ = 3;
        dibit = 1;
      }
      if ((state->synctype & 1) == 1) {
        dibit ^= 1;
      }
      return dibit;
  } else {
      //  2 +X2-TDMA (non inverted signal data frame)
      //  3 -X2-TDMA (inverted signal voice frame)
      //  4 +X2-TDMA (non inverted signal voice frame)
      //  5 -X2-TDMA (inverted signal data frame)
      //  8 +NXDN (non inverted voice frame)
      //  9 -NXDN (inverted voice frame)
      // 10 +DMR (non inverted signal data frame)
      // 11 -DMR (inverted signal voice frame)
      // 12 +DMR (non inverted signal voice frame)
      // 13 -DMR (inverted signal data frame)
      // 16 +NXDN (non inverted data frame)
      // 17 -NXDN (inverted data frame)
      int dibit;

      // Revert to the original approach: choose the symbol according to the regions delimited
      // by center, umid and lmid
      if (symbol > state->center) {
          if (symbol > state->umid) {
              dibit = 1;               // +3
          } else {
              dibit = 0;               // +1
          }
      } else {
          if (symbol < state->lmid) {
              dibit = 3;               // -3
          } else {
              dibit = 2;               // -1
          }
      }
      // store non-inverted values in dibit_buf
      *state->dibit_buf_p++ = dibit;

      if ((state->synctype & 1) == 1) { 
        dibit = invert_dibit(dibit);
      }
      state->last_dibit = dibit;
      return dibit;
  }
}

/**
 * \brief This important method reads the last analog signal value (getSymbol call) and digitizes it.
 * Depending of the ongoing transmission it in converted into a bit (0/1) or a di-bit (00/01/10/11).
 */
int
getDibit (dsd_opts* opts, dsd_state* state)
{
  int symbol, dibit;

  state->numflips = 0;
  symbol = getSymbol (opts, state, 1);

  state->sbuf[state->sidx] = symbol;

  // in c4fm min/max must only be updated during sync
  state->maxref = state->max;
  state->minref = state->min;

  // Increase sidx
  if (state->sidx == (opts->ssize - 1)) {
      state->sidx = 0;
  } else {
      state->sidx++;
  }

  if (state->dibit_buf_p > state->dibit_buf + 48000)
      state->dibit_buf_p = state->dibit_buf + 200;

  dibit = digitize (opts, state, symbol);
  return dibit;
}

void
skipDibit (dsd_opts * opts, dsd_state * state, int count)
{
  int i;

  for (i = 0; i < count; i++) {
      getDibit (opts, state);
  }
}

