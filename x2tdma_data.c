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

static unsigned char hamming7_4_decode[64] =
{
    0x00, 0x03, 0x05, 0xE7,     /* 0x00 to 0x07 */
    0x09, 0xEB, 0xED, 0xEE,     /* 0x08 to 0x0F */
    0x03, 0x33, 0x4D, 0x63,     /* 0x10 to 0x17 */
    0x8D, 0xA3, 0xDD, 0xED,     /* 0x18 to 0x1F */
    0x05, 0x2B, 0x55, 0x65,     /* 0x20 to 0x27 */
    0x8B, 0xBB, 0xC5, 0xEB,     /* 0x28 to 0x2F */
    0x81, 0x63, 0x65, 0x66,     /* 0x30 to 0x37 */
    0x88, 0x8B, 0x8D, 0x6F,     /* 0x38 to 0x3F */
    0x09, 0x27, 0x47, 0x77,     /* 0x40 to 0x47 */
    0x99, 0xA9, 0xC9, 0xE7,     /* 0x48 to 0x4F */
    0x41, 0xA3, 0x44, 0x47,     /* 0x50 to 0x57 */
    0xA9, 0xAA, 0x4D, 0xAF,     /* 0x58 to 0x5F */
    0x21, 0x22, 0xC5, 0x27,     /* 0x60 to 0x67 */
    0xC9, 0x2B, 0xCC, 0xCF,     /* 0x68 to 0x6F */
    0x11, 0x21, 0x41, 0x6F,     /* 0x70 to 0x77 */
    0x81, 0xAF, 0xCF, 0xFF      /* 0x78 to 0x7F */
};

static unsigned char hamming7_4_correct(unsigned char value)
{
    unsigned char c = hamming7_4_decode[value >> 1];
    if (value & 1) {
        c &= 0x0F;
    } else {
        c >>= 4;
    }
    return c;
}

void
processX2TDMAData (dsd_opts * opts, dsd_state * state)
{
  int i, dibit;
  int *dibit_p;
  char sync[25];
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  unsigned int golay_codeword = 0;
  unsigned int bursttype = 0;
  unsigned int print_burst = 1;

  dibit_p = state->dibit_buf_p - 24;

  // CACH
  for (i = 0; i < 12; i++) {
      dibit = getDibit (opts, state);
      cachbits[2*i] = (1 & (dibit >> 1));    // bit 1
      cachbits[2*i+1] = (1 & dibit);   // bit 0
  }
  cach_hdr  = ((cachbits[0] << 0) | (cachbits[4] << 1) | (cachbits[8] << 2) | (cachbits[12] << 3));
  cach_hdr |= ((cachbits[14] << 4) | (cachbits[18] << 5) | (cachbits[22] << 6));
  cach_hdr_hamming = hamming7_4_correct(cach_hdr);

  state->currentslot = ((cach_hdr_hamming >> 1) & 1);      // bit 1
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
  skipDibit (opts, state, 49);

  // slot type
  golay_codeword  = getDibit (opts, state);
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);

  dibit = getDibit (opts, state);
  bursttype = dibit;

  dibit = getDibit (opts, state);
  bursttype = ((bursttype << 2) | dibit);
  golay_codeword = ((golay_codeword << 4)|bursttype);

  // parity bit
  golay_codeword <<= 2;
  golay_codeword |= getDibit (opts, state);

  // signaling data or sync
  for (i = 0; i < 24; i++) {
      dibit = *dibit_p++;
      if (opts->inverted_x2tdma == 1)
          dibit = (dibit ^ 2);
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
  //skipDibit (opts, state, 115);

  // current slot second half only!
  skipDibit (opts, state, 49);

  if (print_burst) { 
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: QPSK offs: %u      inlvl: %2i%% %s %s CACH: 0x%x ",
              state->ftype, state->offset, level,
              state->slot0light, state->slot1light, cach_hdr_hamming);
      if (bursttype > 9) {
          printf ("Unknown burst type: %u\n", bursttype);
      } else {
          printf ("%s\n", slottype_to_string[bursttype]);
      }
  }
}

