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
#include "dmr_const.h"

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
processDMRvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from DMR frame
  int i, j, k, dibit;
  int *dibit_p;
  char ambe_fr[4][24];
  char ambe_fr2[4][24];
  char ambe_fr3[4][24];
  const int *w, *x, *y, *z;
  char sync[25];
  unsigned char cachdata[13];
  unsigned char cachbits[25];
  unsigned char cachbits2[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  int mutecurrentslot;
  int msMode;

  mutecurrentslot = 0;
  msMode = 0;

  dibit_p = state->dibit_buf_p - 144;
  for (j = 0; j < 6; j++) {
      // 2nd half of previous slot
      for (i = 0; i < 54; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
              if (opts->inverted_dmr == 1)
                  dibit = (dibit ^ 2);
          }
      }

      // CACH
      for (i = 0; i < 12; i++) {
        if (j > 0) {
          dibit = getDibit (opts, state);
        } else {
          dibit = *dibit_p++;
          if (opts->inverted_dmr == 1)
            dibit = (dibit ^ 2);
        }
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

      // current slot frame 1
      w = rW;
      x = rX;
      y = rY;
      z = rZ;
      for (i = 0; i < 36; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
              if (opts->inverted_dmr == 1)
                  dibit = (dibit ^ 2);
          }
          ambe_fr[*w][*x] = (1 & (dibit >> 1)); // bit 1
          ambe_fr[*y][*z] = (1 & dibit);        // bit 0
          w++;
          x++;
          y++;
          z++;
      }

      // current slot frame 2 first half
      w = rW;
      x = rX;
      y = rY;
      z = rZ;
      for (i = 0; i < 18; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
              if (opts->inverted_dmr == 1)
                  dibit = (dibit ^ 2);
          }
          ambe_fr2[*w][*x] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[*y][*z] = (1 & dibit);       // bit 0
          w++;
          x++;
          y++;
          z++;
      }

      // signaling data or sync
      for (i = 0; i < 24; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
              if (opts->inverted_dmr == 1)
                  dibit = (dibit ^ 2);
          }
          sync[i] = (dibit | 1) + 48;
      }
      sync[24] = 0;

      if ((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0)) {
          mutecurrentslot = 1;
          if (state->currentslot == 0)
            {
              strcpy (state->slot0light, "[slot0]");
            }
          else
            {
              strcpy (state->slot1light, "[slot1]");
            }
      } else if ((strcmp (sync, DMR_BS_VOICE_SYNC) == 0) || (strcmp (sync, DMR_MS_VOICE_SYNC) == 0)) {
          mutecurrentslot = 0;
          if (state->currentslot == 0)
            {
              strcpy (state->slot0light, "[SLOT0]");
            }
          else
            {
              strcpy (state->slot1light, "[SLOT1]");
            }
      }

      if ((strcmp (sync, DMR_MS_VOICE_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0)) {
          msMode = 1;
      }

      if ((j == 0) && (opts->errorbars == 1)) {
          int level = (int) state->max / 164;
          printf ("Sync: %s mod: %s      inlvl: %2i%% %s %s  VOICE e:",
                  state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"), level,
                  state->slot0light, state->slot1light);
      }

      // current slot frame 2 second half
      for (i = 0; i < 18; i++) {
          dibit = getDibit (opts, state);
          ambe_fr2[*w][*x] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[*y][*z] = (1 & dibit);       // bit 0
          w++;
          x++;
          y++;
          z++;
      }

      if (mutecurrentslot == 0) {
          if (state->firstframe == 1) { // we don't know if anything received before the first sync after no carrier is valid
              state->firstframe = 0;
          } else {
              processAMBEFrame (opts, state, ambe_fr);
              if (opts->errorbars == 1) {
                printf ("%s", state->err_str);
              }
              processAMBEFrame (opts, state, ambe_fr2);
              if (opts->errorbars == 1) {
                printf ("%s", state->err_str);
              }
          }
      }

      // current slot frame 3
      w = rW;
      x = rX;
      y = rY;
      z = rZ;
      for (i = 0; i < 36; i++) {
          dibit = getDibit (opts, state);
          ambe_fr3[*w][*x] = (1 & (dibit >> 1));        // bit 1
          ambe_fr3[*y][*z] = (1 & dibit);       // bit 0
          w++;
          x++;
          y++;
          z++;
      }

      if (mutecurrentslot == 0) {
          processAMBEFrame (opts, state, ambe_fr3);
          if (opts->errorbars == 1) {
            printf ("%s", state->err_str);
          }
      }

      // CACH
      for (i = 0; i < 12; i++) {
        dibit = getDibit (opts, state);
        cachbits[2*i] = (1 & (dibit >> 1));    // bit 1
        cachbits[2*i+1] = (1 & dibit);   // bit 0
      }
      cach_hdr  = ((cachbits[0] << 0) | (cachbits[4] << 1) | (cachbits[8] << 2) | (cachbits[12] << 3));
      cach_hdr |= ((cachbits[14] << 4) | (cachbits[18] << 5) | (cachbits[22] << 6));
      cach_hdr_hamming = hamming7_4_correct(cach_hdr);

      // next slot
      skipDibit (opts, state, 54);

      // signaling data or sync
      for (i = 0; i < 24; i++) {
          dibit = getDibit (opts, state);
          sync[i] = (dibit | 1) + 48;
      }
      sync[24] = 0;

      if ((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (msMode == 1)) {
          if (state->currentslot == 0)
            {
              strcpy (state->slot1light, " slot1 ");
            }
          else
            {
              strcpy (state->slot0light, " slot0 ");
            }
      } else if (strcmp (sync, DMR_BS_VOICE_SYNC) == 0) {
          if (state->currentslot == 0)
            {
              strcpy (state->slot1light, " SLOT1 ");
            }
          else
            {
              strcpy (state->slot0light, " SLOT0 ");
            }
      }

      if (j == 5) {
          // 2nd half next slot
          skipDibit (opts, state, 54);

          // CACH
          skipDibit (opts, state, 12);

          // first half current slot
          skipDibit (opts, state, 54);
      }
  }

  if (opts->errorbars == 1) {
      printf ("\n");
  }
}

