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
#include "x2tdma_const.h"

static unsigned char hamming7_4_decode[64] =
{
    0x00, 0x01, 0x08, 0x24, 0x01, 0x11, 0x53, 0x91, 
    0x06, 0x2A, 0x23, 0x22, 0xB3, 0x71, 0x33, 0x23, 
    0x06, 0xC4, 0x54, 0x44, 0x5D, 0x71, 0x55, 0x54, 
    0x66, 0x76, 0xE6, 0x24, 0x76, 0x77, 0x53, 0x7F, 
    0x08, 0xCA, 0x88, 0x98, 0xBD, 0x91, 0x98, 0x99, 
    0xBA, 0xAA, 0xE8, 0x2A, 0xBB, 0xBA, 0xB3, 0x9F, 
    0xCD, 0xCC, 0xE8, 0xC4, 0xDD, 0xCD, 0x5D, 0x9F, 
    0xE6, 0xCA, 0xEE, 0xEF, 0xBD, 0x7F, 0xEF, 0xFF, 
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

unsigned int
processX2TDMAvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from X2TDMA frame
  int i, j, k, dibit;
  unsigned char *dibit_p;
  unsigned int total_errs = 0;
  char ambe_fr[4][24];
  char ambe_fr2[4][24];
  char ambe_fr3[4][24];
  char sync[25];
  unsigned char syncdata[24];
  unsigned char infodata[82];
  unsigned int lcinfo[3];    // Data in hex-words (6 bit words), stored packed in groups of four, in a uint32_t
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  unsigned char eeei = 0, aiei = 0;
  int mutecurrentslot = 0;

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
      cach_hdr_hamming = hamming7_4_correct(cach_hdr);

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
          ambe_fr[aW[i]][aX[i]] = (1 & (dibit >> 1)); // bit 1
          ambe_fr[aY[i]][aZ[i]] = (1 & dibit);        // bit 0
      }

      // current slot frame 2 first half
      for (i = 0; i < 18; i++) {
          if (j > 0) {
              dibit = getDibit (opts, state);
          } else {
              dibit = *dibit_p++;
          }
          ambe_fr2[aW[i]][aX[i]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[aY[i]][aZ[i]] = (1 & dibit);       // bit 0
      }

      // signaling data or sync
      if (j > 0) {
          for (i = 0; i < 24; i++) {
              dibit = getDibit (opts, state);
              syncdata[i] = dibit;
          }
      }

      if ((state->lastsynctype & ~1) == 2) {
          mutecurrentslot = 1;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[slot0]");
          } else {
              strcpy (state->slot1light, "[slot1]");
          }
      } else if ((state->lastsynctype & ~1) == 14) {
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

      // current slot frame 2 second half
      for (i = 0; i < 18; i++) {
          dibit = getDibit (opts, state);
          ambe_fr2[aW[i+18]][aX[i+18]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr2[aY[i+18]][aZ[i+18]] = (1 & dibit);       // bit 0
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
          ambe_fr3[aW[i]][aX[i]] = (1 & (dibit >> 1));        // bit 1
          ambe_fr3[aY[i]][aZ[i]] = (1 & dibit);       // bit 0
      }

      if (mutecurrentslot == 0) {
          processAMBEFrame (opts, state, ambe_fr3);
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
      cach_hdr_hamming = hamming7_4_correct(cach_hdr);

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
    return total_errs;
}

