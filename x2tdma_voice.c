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

unsigned int
processX2TDMAvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from X2TDMA frame
  int i, j, k;
  unsigned char *dibit_p;
  unsigned int dibit, total_errs = 0;
  unsigned char ambe_dibits1[36], ambe_dibits2[36], ambe_dibits3[36];
  char sync[25];
  unsigned char syncdata[24];
  unsigned char infodata[82];
  unsigned int lcinfo[3];    // Data in hex-words (6 bit words), stored packed in groups of four, in a uint32_t
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  unsigned char eeei = 0, aiei = 0;
  int mutecurrentslot = 0, invertedSync = 0;

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
          for (i = 0; i < 24; i++) {
              syncdata[i] = getDibit (opts, state);
          }
      }

      if ((state->lastsynctype >> 1) == 1) {
          mutecurrentslot = 1;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[slot0]");
          } else {
              strcpy (state->slot1light, "[slot1]");
          }
      } else if ((state->lastsynctype >> 1) == 7) {
          mutecurrentslot = 0;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[SLOT0]");
          } else {
              strcpy (state->slot1light, "[SLOT1]");
          }
      }

      if (j == 1) {
          eeei = (1 & syncdata[1]);     // bit 0
          aiei = (1 & (syncdata[2] >> 1));      // bit 1

          if ((eeei == 0) && (aiei == 0)) {
              for (k = 0; k < 4; k++) {
                infodata[ 0+k] = (1 & (syncdata[4] >> 1));      // bit 1
                infodata[11+k] = (1 & syncdata[4]); // bit 0
                infodata[22+k] = (1 & (syncdata[5] >> 1));        // bit 1
                infodata[32+k] = (1 & syncdata[5]);      // bit 0
                infodata[42+k] = (1 & (syncdata[6] >> 1));       // bit 1
                infodata[52+k] = (1 & syncdata[6]);      // bit 0
                infodata[62+k] = (1 & (syncdata[7] >> 1));       // bit 1
                infodata[72+k] = (1 & syncdata[7]);  // bit 0
              }
          }
      } else if (j == 2) {
          if ((eeei == 0) && (aiei == 0)) {
              for (k = 4; k < 8; k++) {
                infodata[ 0+k] = (1 & (syncdata[4] >> 1));      // bit 1
                infodata[11+k] = (1 & syncdata[4]); // bit 0
                infodata[22+k] = (1 & (syncdata[5] >> 1));        // bit 1
                infodata[32+k] = (1 & syncdata[5]);      // bit 0
                infodata[42+k] = (1 & (syncdata[6] >> 1));       // bit 1
                infodata[52+k] = (1 & syncdata[6]);      // bit 0
                infodata[62+k] = (1 & (syncdata[7] >> 1));       // bit 1
                infodata[72+k] = (1 & syncdata[7]);  // bit 0
              }
          }
      } else if (j == 4) {
          if ((eeei == 0) && (aiei == 0)) {
              for (k = 8; k < 10; k++) {
                infodata[ 0+k] = (1 & (syncdata[4] >> 1));      // bit 1
                infodata[11+k] = (1 & syncdata[4]); // bit 0
                infodata[22+k] = (1 & (syncdata[5] >> 1));        // bit 1
                infodata[32+k] = (1 & syncdata[5]);      // bit 0
                infodata[42+k] = (1 & (syncdata[6] >> 1));       // bit 1
                infodata[52+k] = (1 & syncdata[6]);      // bit 0
                infodata[62+k] = (1 & (syncdata[7] >> 1));       // bit 1
                infodata[72+k] = (1 & syncdata[7]);  // bit 0
              }
              infodata[10] = (1 & (syncdata[12] >> 1)); // bit 1
              infodata[21] = (1 & syncdata[12]);      // bit 0
          }
      }

      if (j > 0) {
        unsigned int syncdata1 = 0, syncdata2 = 0;
        for (i = 0; i < 12; i++) {
            syncdata1 <<= 2;
            syncdata2 <<= 2;
            syncdata1 |= syncdata[i];
            syncdata2 |= syncdata[i+12];
        }
        if (((syncdata1 == 0x005DDFFD) && (syncdata2 == 0x00DFD5F5)) ||
            ((syncdata1 == 0x0077D57F) && (syncdata2 == 0x00FF555D))) {
            invertedSync++;
        }
      }

      // current slot frame 2 second half
      for (i = 0; i < 18; i++) {
          ambe_dibits2[i+18] = getDibit (opts, state);
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

      if ((strcmp (sync, X2TDMA_BS_DATA_SYNC) == 0) || (state->dmrMsMode == 1)) {
          if (state->currentslot == 0) {
              strcpy (state->slot1light, " slot1 ");
          } else {
              strcpy (state->slot0light, " slot0 ");
          }
      } else if (strcmp (sync, X2TDMA_BS_VOICE_SYNC) == 0) {
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

    if ((eeei == 0) && (aiei == 0)) {
        lcinfo[0] = get_uint(infodata, 24);
        lcinfo[1] = get_uint(infodata+24, 24);
        lcinfo[2] = get_uint(infodata+48, 24);
        //decode_p25_lcf(lcinfo);
    }

    if(invertedSync >= 2) // Things are inverted
        opts->inverted_x2tdma ^= 1;

    return total_errs;
}

