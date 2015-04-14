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

static unsigned int emb_fr[4];
static unsigned int emb_fr_index = 0;

static void AssembleEmb (dsd_state *state, unsigned char lcss, unsigned int syncdata)
{
  switch(lcss) {
    case 0: //Single fragment LC or first fragment CSBK signalling (used for Reverse Channel)
      return;
      break;
    case 1: // First Fragment of LC signaling
      emb_fr_index = 0;
      break;
    case 2: // Last Fragment of LC or CSBK signaling
      if(emb_fr_index != 2) {
        emb_fr_index = 0;
	    return;
      }
      break;
    case 3:
      if(emb_fr_index >= 3)
        return;
	  break;
  }
  emb_fr[emb_fr_index++] = syncdata;
}

unsigned int
processDMRvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from DMR frame
  int i, j;
  unsigned int dibit, total_errs = 0;
  unsigned char *dibit_p;
  unsigned char ambe_dibits1[36], ambe_dibits2[36], ambe_dibits3[36];
  char sync[25];
  unsigned int syncdata = 0;
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  unsigned char emb_fr_valid = 0, mutecurrentslot = 0, invertedSync = 0;

  dibit_p = state->dibit_buf_p - 120;
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
      cach_hdr  = ((cachbits[0] << 6) | (cachbits[4] << 5) | (cachbits[8] << 4) | (cachbits[12] << 3));
      cach_hdr |= ((cachbits[14] << 2) | (cachbits[18] << 1) | (cachbits[22] << 0));
      cach_hdr_hamming = Hamming7_4_Correct(cach_hdr);

      state->currentslot = ((cach_hdr_hamming >> 2) & 1);      // bit 2 
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
          ambe_dibits1[i] = dibit;
      }

      // current slot frame 2 first half
      for (i = 0; i < 18; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
          }
          ambe_dibits2[i] = dibit;
      }

      // signaling data or sync
      if (j > 0) {
          unsigned char lcss;
          unsigned int emb_field = 0;
          syncdata = 0;
          for (i = 0; i < 4; i++) {
              dibit = getDibit (opts, state);
              emb_field <<= 2;
              emb_field |= dibit;
          }
          for (i = 0; i < 16; i++) {
              syncdata <<= 2;
              syncdata |= getDibit (opts, state);
          }
          for (i = 20; i < 24; i++) {
              dibit = getDibit (opts, state);
              emb_field <<= 2;
              emb_field |= dibit;
          }
          // Non-Sync part of the superframe
#if 0
          if (!doQR1676(&emb_field)) {
#endif
              lcss = ((emb_field >> 10) & 3);
              AssembleEmb (state, lcss, syncdata);
              if (lcss == 1) {
                emb_fr_valid = 0;
              } else {
                emb_fr_valid |= (1 << emb_fr_index);
              }
              if(emb_fr_valid == 0x0f) {
                processEmb (state, lcss, emb_fr);
              }
#if 0
          }
#endif
          if (((emb_field == 0x75F7) && (syncdata == 0x5FD7DF75)) ||
              ((emb_field == 0x7FFD) && (syncdata == 0x7D5DD57D))) {
            invertedSync++;
          }
      }

      if ((state->lastsynctype >> 1) == 2) { // Data frame
          mutecurrentslot = 1;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[slot0]");
          } else {
              strcpy (state->slot1light, "[slot1]");
          }
      } else if ((state->lastsynctype >> 1) == 6) { // Voice frame
          mutecurrentslot = 0;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[SLOT0]");
          } else {
              strcpy (state->slot1light, "[SLOT1]");
          }
      }

      // current slot frame 2 second half
      for (i = 0; i < 18; i++) {
          ambe_dibits2[i+18] = getDibit (opts, state);;
      }

      if (mutecurrentslot == 0) {
          if (state->firstframe == 1) { // we don't know if anything received before the first sync after no carrier is valid
              state->firstframe = 0;
          } else {
              processAMBEFrame (opts, state, ambe_dibits1);
              total_errs += state->errs2;
              processAMBEFrame (opts, state, ambe_dibits2);
              total_errs += state->errs2;
          }
      }

      // current slot frame 3
      for (i = 0; i < 36; i++) {
          ambe_dibits3[i] = getDibit (opts, state);
      }

      if (mutecurrentslot == 0) {
          processAMBEFrame (opts, state, ambe_dibits3);
          total_errs += state->errs2;
      }

      // CACH
      for (i = 0; i < 12; i++) {
        dibit = getDibit (opts, state);
        cachbits[2*i] = (1 & (dibit >> 1));    // bit 1
        cachbits[2*i+1] = (1 & dibit);   // bit 0
      }
      cach_hdr  = ((cachbits[0] << 0) | (cachbits[4] << 1) | (cachbits[8] << 2) | (cachbits[12] << 3));
      cach_hdr |= ((cachbits[14] << 4) | (cachbits[18] << 5) | (cachbits[22] << 6));
      cach_hdr_hamming = Hamming7_4_Correct(cach_hdr);

      // next slot
      skipDibit (opts, state, 54);

      // signaling data or sync
      for (i = 0; i < 24; i++) {
          dibit = getDibit (opts, state);
          sync[i] = (dibit | 1) + 0x30;
      }
      sync[24] = 0;

      if ((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (state->dmrMsMode == 1)) {
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

    if(invertedSync >= 2) // Things are inverted
        opts->inverted_dmr ^= 1;

    return total_errs;
}

