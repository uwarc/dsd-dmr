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
int bchDec(uint64_t cw, uint16_t *cw_decoded);

static unsigned char
get_p25_nac_and_duid(dsd_opts *opts, dsd_state *state)
{
  // Read the NAC, 12 bits
  uint64_t bch_code = 0;
  unsigned short nac = 0, new_nac = 0;
  unsigned char duid = 0, new_duid = 0;
  unsigned int i, dibit;
  int check_result;

  for (i = 0; i < 6; i++) {
      dibit = getDibit (opts, state);
      nac <<= 2;
      nac |= dibit;
      bch_code <<= 2;
      bch_code |= dibit;
  }

  // Read the DUID, 4 bits
  dibit = getDibit (opts, state);
  duid  = (dibit << 2);
  bch_code <<= 2;
  bch_code |= dibit;

  dibit = getDibit (opts, state);
  duid |= (dibit << 0);
  bch_code <<= 2;
  bch_code |= dibit;

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
  //parity = (dibit & 1);     // bit 0

  // Check if the NID is correct
  // Ideas taken from http://op25.osmocom.org/trac/wiki.png/browser/op25/gr-op25/lib/decoder_ff_impl.cc
  // See also p25_training_guide.pdf page 48.
  // See also tia-102-baaa-a-project_25-fdma-common_air_interface.pdf page 40.
  check_result = bchDec(bch_code, &new_nac);
  if (check_result >= 0) {
      new_duid = (new_nac >> 12);
      new_nac &= 0x0FFF;
      if (new_nac != nac) {
          // NAC fixed by error correction
          printf("old_nac: 0x%03x -> nac: 0x%03x\n", nac, new_nac);
          nac = new_nac;
          state->debug_header_errors++;
      }
      if (new_duid != duid) {
          // DUID fixed by error correction
          printf("old_duid: 0x%x -> duid: 0x%x\n", duid, new_duid);
          duid = new_duid;
          state->debug_header_errors++;
      }
#if 0
      // Check the parity
      if (parity_table[new_duid] != parity) {
          //printf("Error in parity detected?");
      }
#endif
  } else {
      // Check of NID failed and unable to recover its value
      //printf("NID error\n");
      //duid = -1;
      state->debug_header_critical_errors++;
  }
  state->nac = nac;
  return duid;
}

void
processFrame (dsd_opts * opts, dsd_state * state)
{
  if (state->rf_mod == 1) {
    state->maxref = (state->max * 0.80f);
    state->minref = (state->min * 0.80f);
  } else {
    state->maxref = state->max;
    state->minref = state->min;
  }

  state->lasttg = 0;
  state->last_radio_id = 0;
  state->nac = 0;

  if ((state->synctype == 8) || (state->synctype == 9)) {
      state->rf_mod = 2;
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      processNXDNVoice (opts, state);
      return;
  } else if ((state->synctype == 16) || (state->synctype == 17)) {
      state->rf_mod = 2;
      closeMbeOutFile (opts, state);
      state->err_str[0] = 0;
      processNXDNData (opts, state);
      return;
  } else if ((state->synctype == 6) || (state->synctype == 7)) {
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      processDSTAR (opts, state);
      return;
  } else if ((state->synctype == 18) || (state->synctype == 19)) {
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      processDSTAR_HD (opts, state);
      return;
  } else if ((state->synctype >= 10) && (state->synctype <= 13)) {
      state->lasttg = 0;
      if ((state->synctype == 11) || (state->synctype == 12)) {
          if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
              openMbeOutFile (opts, state);
          }
          processDMRvoice (opts, state);
      } else {
          state->err_str[0] = 0;
          processDMRdata (opts, state);
      }
      return;
#if 0
  } else if ((state->synctype >= 2) && (state->synctype <= 5)) {
      if ((state->synctype == 3) || (state->synctype == 4)) {
          if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
              openMbeOutFile (opts, state);
          }
          processX2TDMAvoice (opts, state);
      } else {
          state->err_str[0] = 0;
          processX2TDMAData (opts, state);
      }
      return;
#endif
  } else {
      unsigned char duid = get_p25_nac_and_duid(opts, state);
      printf("p25 NAC: 0x%03x, DUID: 0x%x\n", state->nac, duid);
      process_p25_frame (opts, state, duid);
      return;
  }
}

