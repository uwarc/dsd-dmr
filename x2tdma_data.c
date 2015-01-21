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

static const char *slottype_to_string[16] = {
      "PI Header    ", // 0000
      "VOICE Header ", // 0001
      "TLC          ", // 0010
      "CSBK         ", // 0011
      "MBC Header   ", // 0100
      "MBC          ", // 0101
      "DATA Header  ", // 0110
      "RATE 1/2 DATA", // 0111
      "RATE 3/4 DATA", // 1000
      "Slot idle    ", // 1001
      "Rate 1 DATA  ", // 1010
      "Unknown/Bad  ", // 1011
      "Unknown/Bad  ", // 1100
      "Unknown/Bad  ", // 1101
      "Unknown/Bad  ", // 1110
      "Unknown/Bad  "  // 1111
};

void
processX2TDMAData (dsd_opts * opts, dsd_state * state)
{
  int i, dibit;
  int *dibit_p;
  char sync[25];
  unsigned char cachbits[25];
  unsigned int golay_codeword = 0;
  unsigned int bursttype = 0;
  unsigned int print_burst = 1;

  dibit_p = state->dibit_buf_p - 90;

  // CACH
  for (i = 0; i < 12; i++) {
      dibit = *dibit_p++;
      cachbits[2*i] = (1 & (dibit >> 1));    // bit 1
      cachbits[2*i+1] = (1 & dibit);   // bit 0
  }

  state->currentslot = (cachbits[4] & 1);      // bit 1
  if (state->currentslot == 0) {
      state->slot0light[0] = '[';
      state->slot0light[6] = ']';
      state->slot1light[0] = ' ';
      state->slot1light[6] = ' ';
  } else {
      state->slot1light[0] = '[';
      state->slot1light[6] = ']';
      state->slot0light[0] = ' ';
      state->slot0light[6] = ' ';
  }

  // current slot
  dibit_p += 49;

  // slot type
  golay_codeword  = *dibit_p++;
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;

  bursttype = *dibit_p++;

  dibit = *dibit_p++;
  bursttype = ((bursttype << 2) | dibit);
  golay_codeword = ((golay_codeword << 4)|bursttype);

  // parity bit
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;

  // signaling data or sync
  for (i = 0; i < 24; i++) {
      dibit = *dibit_p++;
      sync[i] = (dibit | 1) + 48;
  }
  sync[24] = 0;

  if ((strcmp (sync, X2TDMA_BS_DATA_SYNC) == 0) || (strcmp (sync, X2TDMA_BS_DATA_SYNC) == 0)) {
      if (state->currentslot == 0) {
          strcpy (state->slot0light, "[slot0]");
      } else {
          strcpy (state->slot1light, "[slot1]");
      }
  }

  // repeat of slottype
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);
  golay_codeword >>= 1;
  Golay23_Correct(&golay_codeword);
  bursttype = (golay_codeword & 0x0f);

  if ((bursttype == 8) || (bursttype == 9)) {
    print_burst = 0;
  }

  // current slot second half, cach, next slot 1st half
  skipDibit (opts, state, 115);

  if (print_burst) { 
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: QPSK offs: %u      inlvl: %2i%% %s %s ",
              state->ftype, state->offset, level,
              state->slot0light, state->slot1light);
      if (bursttype > 9) {
          printf ("Unknown burst type: %u\n", bursttype);
      } else {
          printf ("%s\n", slottype_to_string[bursttype]);
      }
  }
}

