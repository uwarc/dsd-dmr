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

static const char *p25frametypes[16] = {
    "HDU",
    "????",
    "????",
    "TDU",
    "????",
    "LDU1",
    "????",
    "TSDU",
    "????",
    "VSELP",
    "LDU2",
    "????",
    "PDU",
    "????",
    "????",
    "TDULC"
};

static unsigned char
get_p25_nac_and_duid(dsd_opts *opts, dsd_state *state)
{
  // Read the NAC and DUID, 16 bits
  uint64_t bch_code = 0;
  unsigned short nid = 0, new_nid = 0;
  unsigned int i, dibit;
  int check_result;

  for (i = 0; i < 8; i++) {
      dibit = getDibit (opts, state);
      nid <<= 2;
      nid |= dibit;
      bch_code <<= 2;
      bch_code |= dibit;
  }

  // Read the BCH data for error correction of NAC and DUID
  for (i = 0; i < 3; i++) {
      dibit = getDibit (opts, state);
      bch_code <<= 2;
      bch_code |= dibit;
  }

  // Intermission: read the status dibit
  getDibit (opts, state);

  // ... continue reading the BCH error correction data
  for (i = 0; i < 20; i++) {
      dibit = getDibit (opts, state);
      bch_code <<= 2;
      bch_code |= dibit;
  }

  // Read the parity bit
  dibit = getDibit (opts, state);
  bch_code <<= 1;
  bch_code |= (dibit >> 1);      // bit 1
  bch_code <<= 1;

  // Check if the NID is correct
  // Ideas taken from http://op25.osmocom.org/trac/wiki.png/browser/op25/gr-op25/lib/decoder_ff_impl.cc
  // See also p25_training_guide.pdf page 48.
  // See also tia-102-baaa-a-project_25-fdma-common_air_interface.pdf page 40.
  check_result = bchDec(bch_code, &new_nid);
  if (check_result >= 0) {
      if (new_nid != nid) {
          // NAC/DUID fixed by error correction
          printf("old_nid: 0x%03x -> nid: 0x%03x\n", nid, new_nid);
          nid = new_nid;
          state->debug_header_errors++;
      }
  }
  state->nac = (nid >> 4);
  state->duid = (nid & 0x0F);
  return (check_result >= 0);
}

static void
processNXDNData (dsd_opts * opts, dsd_state * state, unsigned char lich[9])
{
  unsigned int i, dibit;
  unsigned char *dibit_p;
  unsigned char lich_scram[9] = { 0, 0, 1, 0, 0, 1, 1, 1, 0 };
  dibit_p = state->dibit_buf_p - 8;

  for (i = 0; i < 8; i++) {
      dibit = *dibit_p++;
      if(lich_scram[i] ^ (state->lastsynctype & 0x1)) {
          dibit = (dibit ^ 2);
      }
      lich[i] = 1 & (dibit >> 1);
  }

  skipDibit (opts, state, 174);
}

void
processFrame (dsd_opts * opts, dsd_state * state)
{
  int level = (int) state->max / 164;
  unsigned int total_errs = 0, synctype = (state->synctype >> 1);

  state->nac = 0;
  state->duid = 0;

  if (synctype == 3) {
      unsigned char lich[9];
      processNXDNData (opts, state, lich);
      closeMbeOutFile (opts, state);
      state->errs2 = 0;
      printf ("Sync: %c%s mod: %s offs: %4u inlvl: %2i%% DATA: %s Ch\n",
              ((state->synctype & 1) ? '-' : '+'), state->ftype,
              ((state->rf_mod == 2) ? "GFSK" : "QPSK"), state->offset, level,
              (lich[1] ? (lich[0] ? "Trunk-C Traffic" : "Trunk-D Composite") 
                       : (lich[0] ? "Trunk-C Control" : (lich[6] ? "Repeater" : "Mobile Direct"))));
      return;
  } else if ((synctype == 2) || (synctype == 1)) {
      processDMRdata (opts, state);
      return;
  }

  if (synctype == 0) {
      char tmpStr[1024];
      unsigned char ret = get_p25_nac_and_duid(opts, state);
      if (ret) {
        process_p25_frame (opts, state, tmpStr, 1023);
      } else {
        snprintf(tmpStr, 1023, "Unable to correct errors in NAC/DUID (%s)", p25frametypes[state->duid]);
      }
      printf ("Sync: %c%s mod: %s offs: %4u inlvl: %2i%% p25 NAC: 0x%03x, DUID: 0x%x -> %s\n",
              ((state->synctype & 1) ? '-' : '+'), state->ftype,
              ((state->rf_mod == 2) ? "GFSK" : "QPSK"), state->offset, level,
              state->nac, state->duid, tmpStr);
      return;
  }

  if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
    openMbeOutFile (opts, state);
  }

  if (synctype == 8) {
      total_errs = processNXDNVoice (opts, state);
  } else if (synctype == 4) {
      total_errs = processDSTAR (opts, state);
  } else if (synctype == 5) {
      total_errs = processDSTAR_HD (opts, state);
  } else if (synctype == 6) {
      total_errs = processDMRvoice (opts, state);
  } else if (synctype == 7) {
      total_errs = processX2TDMAvoice (opts, state);
  }
  printf ("Sync: %c%s mod: %s, offs: %4u, inlvl: %2i%% %s %s  VOICE e: %u\n",
          ((state->synctype & 1) ? '-' : '+'), state->ftype, ((state->rf_mod == 2) ? "GFSK" : "QPSK"), state->offset, level,
          state->slot0light, state->slot1light, total_errs);
  return;
}

