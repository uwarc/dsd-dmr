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

static unsigned char emb_fr[4][32];
static unsigned int emb_fr_index = 0;
static unsigned int emb_fr_valid = 0;

static void AssembleEmb (dsd_state *state, unsigned char lcss, unsigned char syncdata[16])
{
  int i, dibit;

  switch(lcss) {
    case 0: //Single fragment LC or first fragment CSBK signalling (used for Reverse Channel)
      return;
      break;
    case 1: // First Fragment of LC signaling
      emb_fr_index = 0;
      emb_fr_valid = 0;
      break;
    case 2: // Last Fragment of LC or CSBK signaling
      if(++emb_fr_index != 3)
	    return;
      break;
    case 3:
      if(++emb_fr_index >= 4)
	    return;
	  break;
  }
  for(i = 0; i < 16; i++) {
      dibit = syncdata[i+4];
      emb_fr[emb_fr_index][2*i] = ((dibit >> 1) & 1);
      emb_fr[emb_fr_index][2*i+1] = (dibit & 1);
  }
  emb_fr_valid |= (1 << emb_fr_index);
}

void
processDMRvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from DMR frame
  int i, j, dibit;
  int *dibit_p;
  unsigned int total_errs = 0;
  char ambe_fr[4][24];
  char ambe_fr2[4][24];
  char ambe_fr3[4][24];
  char sync[25];
  unsigned char syncdata[16];
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  int mutecurrentslot = 0;
  unsigned char msMode = 0;

  dibit_p = state->dibit_buf_p - 144;
  for (j = 0; j < 6; j++) {
      // 2nd half of previous slot
      for (i = 0; i < 54; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
          }
      }

      // CACH
      for (i = 0; i < 12; i++) {
        if (j > 0) {
          dibit = getDibit (opts, state);
        } else {
          dibit = *dibit_p++;
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
      for (i = 0; i < 36; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
          }
          ambe_fr[rW[i]][rX[i]] = (1 & (dibit >> 1)); // bit 1
          ambe_fr[rY[i]][rZ[i]] = (1 & dibit);        // bit 0
      }

      // current slot frame 2 first half
      for (i = 0; i < 18; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
          }
          ambe_fr2[rW[i]][rX[i]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[rY[i]][rZ[i]] = (1 & dibit);       // bit 0
      }

      // signaling data or sync
      if (j > 0) {
          unsigned char lcss;
          unsigned int emb_field = 0;
          for (i = 0; i < 4; i++) {
              dibit = getDibit (opts, state);
              sync[i] = (dibit | 1) + 48;
              emb_field <<= 2;
              emb_field |= dibit;
          }
          for (i = 0; i < 16; i++) {
              dibit = getDibit (opts, state);
              sync[i+4] = (dibit | 1) + 48;
              syncdata[i] = dibit;
          }
          for (i = 20; i < 24; i++) {
              dibit = getDibit (opts, state);
              sync[i] = (dibit | 1) + 48;
              emb_field <<= 2;
              emb_field |= dibit;
          }
          // Non-Sync part of the superframe
#if 0
#if 0
          if (!doQR1676(&emb_field)) {
#endif
              lcss = ((emb_field >> 10) & 3);
              AssembleEmb (state, lcss, syncdata);
              if(emb_fr_valid == 0x0f) {
                processEmb (state, lcss, emb_fr);
              }
#if 0
          }
#endif
#endif
      } else { 
          for (i = 0; i < 24; i++) {
              dibit = *dibit_p++;
              sync[i] = (dibit | 1) + 48;
          }
      }
      sync[24] = 0;

      if ((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0)) {
          mutecurrentslot = 1;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[slot0]");
          } else {
              strcpy (state->slot1light, "[slot1]");
          }
      } else if ((strcmp (sync, DMR_BS_VOICE_SYNC) == 0) || (strcmp (sync, DMR_MS_VOICE_SYNC) == 0)) {
          mutecurrentslot = 0;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[SLOT0]");
          } else {
              strcpy (state->slot1light, "[SLOT1]");
          }
      }

      if ((strcmp (sync, DMR_MS_VOICE_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0)) {
          msMode = 1;
      }

      // current slot frame 2 second half
      for (i = 0; i < 18; i++) {
          dibit = getDibit (opts, state);
          ambe_fr2[rW[i+18]][rX[i+18]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[rY[i+18]][rZ[i+18]] = (1 & dibit);       // bit 0
      }

      if (mutecurrentslot == 0) {
          if (state->firstframe == 1) { // we don't know if anything received before the first sync after no carrier is valid
              state->firstframe = 0;
          } else {
              processAMBEFrame (opts, state, ambe_fr);
              total_errs += state->errs2;
              processAMBEFrame (opts, state, ambe_fr2);
              total_errs += state->errs2;
          }
      }

      // current slot frame 3
      for (i = 0; i < 36; i++) {
          dibit = getDibit (opts, state);
          ambe_fr3[rW[i]][rX[i]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr3[rY[i]][rZ[i]] = (1 & dibit);       // bit 0
      }

      if (mutecurrentslot == 0) {
          processAMBEFrame (opts, state, ambe_fr3);
          total_errs += state->errs2;
      }

      if ((j == 0) && (opts->errorbars == 1)) {
          int level = (int) state->max / 164;
          printf ("Sync: %s mod: %s      inlvl: %2i%% %s %s  VOICE e: %u\n",
                  state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"), level,
                  state->slot0light, state->slot1light, total_errs);
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
          if (state->currentslot == 0) {
              strcpy (state->slot1light, " slot1 ");
          } else {
              strcpy (state->slot0light, " slot0 ");
          }
      } else if (strcmp (sync, DMR_BS_VOICE_SYNC) == 0) {
          if (state->currentslot == 0) {
              strcpy (state->slot1light, " SLOT1 ");
          } else {
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
}

