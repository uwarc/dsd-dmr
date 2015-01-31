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
processX2TDMAvoice (dsd_opts * opts, dsd_state * state)
{
  // extracts AMBE frames from X2TDMA frame
  int i, j, dibit;
  int *dibit_p;
  unsigned int total_errs = 0;
  char ambe_fr[4][24];
  char ambe_fr2[4][24];
  char ambe_fr3[4][24];
  char sync[25];
  unsigned char syncdata[24];
  unsigned char mfid = 0, lcformat = 0;
  char lcinfo[57];
  char parity;
  unsigned char cachbits[25];
  unsigned char cach_hdr = 0, cach_hdr_hamming = 0;
  unsigned char eeei = 0, aiei = 0, burstd = 0;
  int mutecurrentslot = 0;

  lcinfo[56] = 0;

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
      } else if ((state->lastsynctype & ~1) == 4) {
          mutecurrentslot = 0;
          if (state->currentslot == 0) {
              strcpy (state->slot0light, "[SLOT0]");
          } else {
              strcpy (state->slot1light, "[SLOT1]");
          }
      }

      if (j == 1)
        {
          eeei = (1 & syncdata[1]);     // bit 0
          aiei = (1 & (syncdata[2] >> 1));      // bit 1

          if ((eeei == 0) && (aiei == 0))
            {
              mfid |= ((1 & syncdata[4]) << 4); // bit 0
              mfid |= ((1 & syncdata[8]) << 3); // bit 0
              mfid |= ((1 & syncdata[12]) << 2);        // bit 0
              mfid |= ((1 & syncdata[16]) << 1);        // bit 0
              lcformat |= ((1 & (syncdata[4] >> 1)) << 7);      // bit 1
              lcformat |= ((1 & (syncdata[8] >> 1)) << 6);      // bit 1
              lcformat |= ((1 & (syncdata[12] >> 1)) << 5);     // bit 1
              lcformat |= ((1 & (syncdata[16] >> 1)) << 4);     // bit 1
              lcinfo[6] = (1 & (syncdata[5] >> 1)) + 48;        // bit 1
              lcinfo[16] = (1 & syncdata[5]) + 48;      // bit 0
              lcinfo[26] = (1 & (syncdata[6] >> 1)) + 48;       // bit 1
              lcinfo[36] = (1 & syncdata[6]) + 48;      // bit 0
              lcinfo[46] = (1 & (syncdata[7] >> 1)) + 48;       // bit 1
              parity = (1 & syncdata[7]) + 48;  // bit 0
              lcinfo[7] = (1 & (syncdata[9] >> 1)) + 48;        // bit 1
              lcinfo[17] = (1 & syncdata[9]) + 48;      // bit 0
              lcinfo[27] = (1 & (syncdata[10] >> 1)) + 48;      // bit 1
              lcinfo[37] = (1 & syncdata[10]) + 48;     // bit 0
              lcinfo[47] = (1 & (syncdata[11] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[11]) + 48; // bit 0
              lcinfo[8] = (1 & (syncdata[13] >> 1)) + 48;       // bit 1
              lcinfo[18] = (1 & syncdata[13]) + 48;     // bit 0
              lcinfo[28] = (1 & (syncdata[14] >> 1)) + 48;      // bit 1
              lcinfo[38] = (1 & syncdata[14]) + 48;     // bit 0
              lcinfo[48] = (1 & (syncdata[15] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[15]) + 48; // bit 0
              lcinfo[9] = (1 & (syncdata[17] >> 1)) + 48;       // bit 1
              lcinfo[19] = (1 & syncdata[17]) + 48;     // bit 0
              lcinfo[29] = (1 & (syncdata[18] >> 1)) + 48;      // bit 1
              lcinfo[39] = (1 & syncdata[18]) + 48;     // bit 0
              lcinfo[49] = (1 & (syncdata[19] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[19]) + 48; // bit 0
            }
        }
      else if (j == 2)
        {
          if ((eeei == 0) && (aiei == 0))
            {
              mfid |= (1 & syncdata[4]); // bit 0
              lcformat |= ((1 & (syncdata[4] >> 1)) << 3);      // bit 1
              lcformat |= ((1 & (syncdata[8] >> 1)) << 2);      // bit 1
              lcformat |= ((1 & (syncdata[12] >> 1)) << 1);     // bit 1
              lcformat |= ((1 & (syncdata[16] >> 1)) << 0);     // bit 1
              lcinfo[10] = (1 & (syncdata[5] >> 1)) + 48;       // bit 1
              lcinfo[20] = (1 & syncdata[5]) + 48;      // bit 0
              lcinfo[30] = (1 & (syncdata[6] >> 1)) + 48;       // bit 1
              lcinfo[40] = (1 & syncdata[6]) + 48;      // bit 0
              lcinfo[50] = (1 & (syncdata[7] >> 1)) + 48;       // bit 1
              parity = (1 & syncdata[7]) + 48;  // bit 0
              lcinfo[0] = (1 & syncdata[8]) + 48;       // bit 0
              lcinfo[11] = (1 & (syncdata[9] >> 1)) + 48;       // bit 1
              lcinfo[21] = (1 & syncdata[9]) + 48;      // bit 0
              lcinfo[31] = (1 & (syncdata[10] >> 1)) + 48;      // bit 1
              lcinfo[41] = (1 & syncdata[10]) + 48;     // bit 0
              lcinfo[51] = (1 & (syncdata[11] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[11]) + 48; // bit 0
              lcinfo[1] = (1 & syncdata[12]) + 48;      // bit 0
              lcinfo[12] = (1 & (syncdata[13] >> 1)) + 48;      // bit 1
              lcinfo[22] = (1 & syncdata[13]) + 48;     // bit 0
              lcinfo[32] = (1 & (syncdata[14] >> 1)) + 48;      // bit 1
              lcinfo[42] = (1 & syncdata[14]) + 48;     // bit 0
              lcinfo[52] = (1 & (syncdata[15] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[15]) + 48; // bit 0
              lcinfo[2] = (1 & syncdata[16]) + 48;      // bit 0
              lcinfo[13] = (1 & (syncdata[17] >> 1)) + 48;      // bit 1
              lcinfo[23] = (1 & syncdata[17]) + 48;     // bit 0
              lcinfo[33] = (1 & (syncdata[18] >> 1)) + 48;      // bit 1
              lcinfo[43] = (1 & syncdata[18]) + 48;     // bit 0
              lcinfo[53] = (1 & (syncdata[19] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[19]) + 48; // bit 0
            }
        }
      else if (j == 3)
        {
          burstd = (1 & syncdata[1]);   // bit 0

          state->p25algid = ((syncdata[4] << 2)  | (syncdata[5] << 0));  
          if (burstd == 0) {
              state->p25algid <<= 4;
              state->p25algid = ((syncdata[8] << 2)  | (syncdata[9] << 0));  

              state->p25keyid = syncdata[10];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[11];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[12];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[13];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[14];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[15];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[16];
              state->p25keyid <<= 2;
              state->p25keyid = syncdata[17];
              state->p25keyid <<= 2;
            }
          else
            {
              state->p25algid = 0;
              state->p25keyid = 0;
            }
        }
      else if (j == 4)
        {
          if ((eeei == 0) && (aiei == 0))
            {
              mfid |= ((1 & (syncdata[4] >> 1)) << 7);  // bit 1
              mfid |= ((1 & (syncdata[8] >> 1)) << 6);  // bit 1
              mfid |= ((1 & (syncdata[12] >> 1)) << 5); // bit 1
              lcinfo[3] = (1 & syncdata[4]) + 48;       // bit 0
              lcinfo[14] = (1 & (syncdata[5] >> 1)) + 48;       // bit 1
              lcinfo[24] = (1 & syncdata[5]) + 48;      // bit 0
              lcinfo[34] = (1 & (syncdata[6] >> 1)) + 48;       // bit 1
              lcinfo[44] = (1 & syncdata[6]) + 48;      // bit 0
              lcinfo[54] = (1 & (syncdata[7] >> 1)) + 48;       // bit 1
              parity = (1 & syncdata[7]) + 48;  // bit 0
              lcinfo[4] = (1 & syncdata[8]) + 48;       // bit 0
              lcinfo[15] = (1 & (syncdata[9] >> 1)) + 48;       // bit 1
              lcinfo[25] = (1 & syncdata[9]) + 48;      // bit 0
              lcinfo[35] = (1 & (syncdata[10] >> 1)) + 48;      // bit 1
              lcinfo[45] = (1 & syncdata[10]) + 48;     // bit 0
              lcinfo[55] = (1 & (syncdata[11] >> 1)) + 48;      // bit 1
              parity = (1 & syncdata[11]) + 48; // bit 0
              lcinfo[5] = (1 & syncdata[12]) + 48;      // bit 0
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

      if ((j == 0) && (opts->errorbars == 1)) {
          int level = (int) state->max / 164;
          printf ("Sync: %s mod: QPSK     inlvl: %2i%% %s %s  VOICE e: %u\n",
                  state->ftype, level, state->slot0light, state->slot1light, total_errs);
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
        printf ("mfid: %u, lcformat: 0x%02x, lcinfo: %s\n", mfid, lcformat, lcinfo);
    }
    if (opts->p25enc == 1) {
        printf ("algid: 0x%04x kid: 0x%04x\n", state->p25algid, state->p25keyid);
    }
}

