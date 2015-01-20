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

static unsigned char fid_mapping[256] = {
    1, 0, 0, 0, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0,
    7, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char *fids[16] = {
      "Unknown",
      "Standard",
      "Flyde Micro",
      "PROD-EL SPA",
      "Motorola Connect+",
      "RADIODATA GmbH",
      "Hyteria (8)",
      "Motorola Capacity+",
      "EMC S.p.A (19)",
      "EMC S.p.A (28)",
      "Radio Activity Srl (51)",
      "Radio Activity Srl (60)",
      "Tait Electronics",
      "Hyteria (104)",
      "Vertex Standard"
};

static char *serviceopts[10] = {
        "Unknown",
        "EMERGENCY ", // 0
        "Privacy ",   // 1
        "Unknown 1 ", // 2
        "Unknown 2 ", // 3
        "Broadcast ", // 4
        "OVCM ",      // 5
        "Priority:2 ", // 6
        "Priority:1 ", // 7
        "Priority:3 "  // 6+7
};

static char *headertypes[16] = {
    "UDT ",
    "Resp",
    "UDat",
    "CDat",
    "Hdr4",
    "Hdr5",
    "Hdr6",
    "Hdr7",
    "Hdr8",
    "Hdr9",
    "Hdr10",
    "Hdr11",
    "Hdr12",
    "DSDa",
    "RSDa",
    "Prop"
};

static unsigned char cach_deinterleave[24] = {
     0,  7,  8,  9,  1, 10, 11, 12,
     2, 13, 14, 15,  3, 16,  4, 17,
    18, 19,  5, 20, 21, 22,  6, 23
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

// Hamming (15, 11, 3)
static unsigned int Hamming15113Gen[11] = {
    0x4009, 0x200d, 0x100f, 0x080e, 0x0407, 0x020a, 0x0105, 0x008b, 0x004c, 0x0026, 0x0013
};

static unsigned int Hamming15113Table[16] = {
    0x0000, 0x0001, 0x0002, 0x0013, 0x0004, 0x0105, 0x0026, 0x0407,
    0x0008, 0x4009, 0x020A, 0x008b, 0x004C, 0x200D, 0x080E, 0x100F
};

void Hamming15_11_3_Correct(unsigned int *block)
{
    unsigned int i, codeword = *block, ecc = 0, syndrome;
    for(i = 0; i < 11; i++) {
        if((codeword & Hamming15113Gen[i]) > 0xf)
            ecc ^= Hamming15113Gen[i];
    }
    syndrome = ecc ^ codeword;

    if (syndrome != 0) {
      codeword ^= Hamming15113Table[syndrome & 0x0f];
    }

    *block = codeword;
    //*block = (codeword >> 4);
}

static const char hex[]="0123456789abcdef";
static void hexdump_packet(unsigned char payload[96], unsigned char packetdump[31])
{
    unsigned int i, j, l = 0;
    for (i = 0, j = 0; i < 80; i++) {
        l <<= 1;
        l |= payload[i];

        if((i&7) == 7) {
            packetdump[j++] = hex[((l >> 4) & 0x0f)];
            packetdump[j++] = hex[((l >> 0) & 0x0f)];
            packetdump[j++] = ' ';
            l = 0;
        }
    }
    packetdump[j] = '\0';
}

static void process_dataheader(unsigned char payload[96])
{
    // Common elements to all data header types
    unsigned int j = get_uint(payload+4, 4); // j is used as header type
    printf("DATA Header: %s: %s:%u SrcRId: %u %c SAP:%u Misc:0x%04x\n", headertypes[j],
           ((payload[0])?"Grp":"DestRID"), get_uint(payload+16, 24), get_uint(payload+40, 24),
           ((payload[1]) ? 'A' : ' '), get_uint(payload+8, 4), get_uint(payload+64, 16));
}

static unsigned int processFlco(dsd_state *state, unsigned char payload[97], char flcostr[1024])
{
  char flco[7];
  unsigned int l = 0, i, k = 0;
  unsigned int radio_id = 0;

  for (i = 0; i < 6; i++) {
    flco[i] = payload[2+i] + 48;
  }
  flco[6] = '\0';

  if (strcmp (flco, "000000") == 0) {
    l = get_uint(payload+16, 8);
    state->talkgroup = get_uint(payload+24, 24);
    state->radio_id = get_uint(payload+48, 24);
    k = snprintf(flcostr, 1023, "Msg: Group Voice Ch Usr, SvcOpts: %u, Talkgroup: %u, RadioId: %u", l, state->talkgroup, state->radio_id);
  } else if (strcmp (flco, "000011") == 0) {
    state->talkgroup = get_uint(payload+24, 24);
    state->radio_id = get_uint(payload+48, 24);
    k = snprintf(flcostr, 1023, "Msg: Unit-Unit Voice Ch Usr, Talkgroup: %u, RadioId: %u", state->talkgroup, state->radio_id);
  } else if (strcmp (flco, "000100") == 0) { 
    state->talkgroup = get_uint(payload+24, 24);
    l = get_uint(payload+48, 24);
    state->radio_id = get_uint(payload+56, 24);
    k = snprintf(flcostr, 1023, "Msg: Capacity+ Group Voice, Talkgroup: %u, RestCh: %u, RadioId: %u", state->talkgroup, l, state->radio_id);
  } else if (strcmp (flco, "110000") == 0) {
    state->lasttg = get_uint(payload+16, 24);
    radio_id = get_uint(payload+40, 24);
    l = ((payload[69] << 2) | (payload[70] << 1) | (payload[71] << 0));
    k = snprintf(flcostr, 1023, "Msg: Terminator Data LC, Talkgroup: %u, RadioId: %u, N(s): %u", state->lasttg, radio_id, l);
  } else { 
    k = snprintf(flcostr, 1023, "Unknown Standard/Capacity+ FLCO: flco: %s, packet_dump: ", flco);
    for (i = 2; i < 12; i++) {
        l = get_uint(payload+i*8, 8);
        flcostr[k++] = hex[((l >> 4) & 0x0f)];
        flcostr[k++] = hex[((l >> 0) & 0x0f)];
        flcostr[k++] = ' ';
    }
    flcostr[k] = '\0';
  }
  return k;
}

// deinterleaved_data[a] = rawdata[(a*181)%196];
static void processBPTC(unsigned char infodata[196], unsigned char payload[97])
{
  unsigned int i, j, k;
  unsigned char data_fr[196];
  for(i = 1; i < 197; i++) {
    data_fr[i-1] = infodata[((i*181)%196)];
  }

  for (i = 0; i < 3; i++)
     data_fr[0 * 15 + i] = 0; // Zero reserved bits
  for (i = 0; i < 15; i++) {
    unsigned int codeword = 0;
    for(j = 0; j < 13; j++) {
      codeword <<= 1;
      codeword |= data_fr[j * 15 + i];
    }
    Hamming15_11_3_Correct(&codeword);
    codeword &= 0x1fff;
    for (j = 0; j < 13; j++) {
      data_fr[j * 15 + i] = ((codeword >> (12 - j)) & 1);
    }
  }
  for (j = 0; j < 9; j++) {
    unsigned int codeword = 0;
    for (i = 0; i < 15; i++) {
        codeword <<= 1;
        codeword |= data_fr[j * 15 + i];
    }
    Hamming15_11_3_Correct(&codeword);
    codeword >>= 4;
    for (i = 0; i < 11; i++) {
        data_fr[j * 15 + 10 - i] = ((codeword >> i) & 1);
    }
  }
  for (i = 3, k = 0; i < 11; i++, k++) {
     payload[k] = data_fr[0 * 15 + i];
  }
  for (j = 1; j < 9; j++) {
    for (i = 0; i < 11; i++, k++) {
      payload[k] = data_fr[j * 15 + i];
    }
  }
}

void processEmb (dsd_state *state, unsigned char lcss, unsigned char emb_fr[4][32])
{
  int i, k;
  unsigned int emb_deinv_fr[8];
  unsigned char fid = 0;
  unsigned char payload[97];

  printf("\nDMR Embedded Signalling present in voice packet: LCSS: %u\n", lcss);

  // Deinterleave
  for (i = 0; i < 8; i++) {
    emb_deinv_fr[i] = 0;
  }
  for (k = 0; k < 15; k++) {
    for (i = 0; i < 8; i++) {
      emb_deinv_fr[i] <<= 1;
      emb_deinv_fr[i] |= emb_fr[((8*k+i) >> 5)][((8*k+i) & 31)];
    }
  }
  for(i = 0; i < 8; i++) {
    unsigned int hamming_codeword = emb_deinv_fr[i];
    Hamming15_11_3_Correct(&hamming_codeword);
    emb_deinv_fr[i] = hamming_codeword;
  }
  fid  = (((emb_deinv_fr[0] >> 11) & 0x07) << 5);
  fid |= (((emb_deinv_fr[1] >>  4) & 0x1f) << 0);

  for (i = 0; i < 6; i++) {
    payload[5-i+2] = ((emb_deinv_fr[0] >> (6+i)) & 0x01);
  }

  for (i = 0; i < 8; i++) {
    payload[i+8] = ((fid >> (7-i)) & 1);
  }

  for (i = 0; i < 6; i++) {
    payload[5-i+16] = ((emb_deinv_fr[1] >> (9+i)) & 0x01);
  }
  for (i = 0; i < 11; i++) {
    k = (14 - i);
	payload[i+21] = ((emb_deinv_fr[2] >> k) & 0x01);
	payload[i+31] = ((emb_deinv_fr[3] >> k) & 0x01);
	payload[i+41] = ((emb_deinv_fr[4] >> k) & 0x01);
	payload[i+51] = ((emb_deinv_fr[5] >> k) & 0x01);
	payload[i+61] = ((emb_deinv_fr[6] >> k) & 0x01);
	payload[i+71] = ((emb_deinv_fr[7] >> k) & 0x01);
  }
  if ((fid == 0) || (fid == 16)) { // Standard feature, MotoTRBO Capacity+
    char flcostr[1024];
    flcostr[0] = '\0';
    processFlco(state, payload, flcostr);
    printf("Embedded Signalling: fid: %s (%u): %s\n", fids[fid_mapping[fid]], fid, flcostr);
  } else {
    unsigned char packetdump[81];
    hexdump_packet(payload, packetdump);
    printf("Unknown FLCO in embedded signalling: %s\n", packetdump);
  }
}

#ifndef NO_REEDSOLOMON
// rs_mask = 0x96 for Voice Header, 0x99 for TLC.
static unsigned int check_and_fix_reedsolomon_12_09_04(ReedSolomon *rs, unsigned char payload[97], unsigned char rs_mask)
{
  unsigned int i, errorFlag;
  unsigned char input[255];
  unsigned char output[255];

  for (i=0; i<255; i++) {
    input[i] = 0;
    output[i] = 0;
  }

  for(i = 0; i < 12; i++) {
    input[i] = get_uint(payload+8*i, 8);
  }
  input[ 9] ^= rs_mask;
  input[10] ^= rs_mask;
  input[11] ^= rs_mask;

  /* decode recv[] */
  errorFlag = rs8_decode(rs, input, output);

  for(i = 0; i < 12; i++) {
    payload[8*i  ] = ((output[i] & 128)? 1 : 0);
    payload[8*i+1] = ((output[i] &  64)? 1 : 0);
    payload[8*i+2] = ((output[i] &  32)? 1 : 0);
    payload[8*i+3] = ((output[i] &  16)? 1 : 0);
    payload[8*i+4] = ((output[i] &   8)? 1 : 0);
    payload[8*i+5] = ((output[i] &   4)? 1 : 0);
    payload[8*i+6] = ((output[i] &   2)? 1 : 0);
    payload[8*i+7] = ((output[i] &   1)? 1 : 0);
  }
  return errorFlag;
}
#endif

void
processDMRdata (dsd_opts * opts, dsd_state * state)
{
  int i, j, dibit;
  int *dibit_p;
  char sync[25];
  unsigned char cach1bits[25];
  unsigned char cach1_hdr = 0, cach1_hdr_hamming = 0;
  unsigned char infodata[196];
  unsigned char payload[114];
  unsigned char packetdump[81];
  unsigned int golay_codeword = 0;
  unsigned int bursttype = 0;
  unsigned char fid = 0;
  unsigned int print_burst = 1;

  if (state->firstframe == 1) { // we don't know if anything received before the first sync after no carrier is valid
    //skipDibit(opts, state, 120);
    state->firstframe = 0;
  }

  dibit_p = state->dibit_buf_p - 90;

  // CACH
  for (i = 0; i < 12; i++) {
      //dibit = getDibit (opts, state);
      dibit = *dibit_p++;
      cach1bits[cach_deinterleave[2*i]] = (1 & (dibit >> 1));    // bit 1
      cach1bits[cach_deinterleave[2*i+1]] = (1 & dibit);   // bit 0
  }
  cach1_hdr  = ((cach1bits[0] << 0) | (cach1bits[1] << 1) | (cach1bits[2] << 2) | (cach1bits[3] << 3));
  cach1_hdr |= ((cach1bits[4] << 4) | (cach1bits[5] << 5) | (cach1bits[6] << 6));
  cach1_hdr_hamming = hamming7_4_correct(cach1_hdr);

  state->currentslot = ((cach1_hdr_hamming >> 1) & 1);      // bit 1
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
  for (i = 0; i < 49; i++) {
      //dibit = getDibit (opts, state);
      dibit = *dibit_p++;
      infodata[2*i] = (1 & (dibit >> 1)); // bit 1
      infodata[2*i+1] = (1 & dibit);        // bit 0
  }

  // slot type
  //golay_codeword  = getDibit (opts, state);
  //golay_codeword <<= 2;
  //golay_codeword |= getDibit (opts, state);
  golay_codeword  = *dibit_p++;
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;

  //dibit = getDibit (opts, state);
  dibit = *dibit_p++;
  bursttype = dibit;

  //dibit = getDibit (opts, state);
  dibit = *dibit_p++;
  bursttype = ((bursttype << 2) | dibit);
  golay_codeword = ((golay_codeword << 4)|bursttype);

  // parity bit
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;
  //golay_codeword |= getDibit (opts, state);

  // signaling data or sync
  for (i = 0; i < 24; i++) {
      dibit = *dibit_p++;
      sync[i] = (dibit | 1) + 48;
  }
  sync[24] = 0;

  if ((strcmp (sync, DMR_BS_DATA_SYNC) == 0) || (strcmp (sync, DMR_MS_DATA_SYNC) == 0)) {
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

  if ((bursttype == 1) || (bursttype == 2)) {
    closeMbeOutFile (opts, state);
  }

  // 2nd half next slot
  for (i = 49; i < 98; i++) {
      dibit = getDibit (opts, state);
      infodata[2*i] = (1 & (dibit >> 1)); // bit 1
      infodata[2*i+1] = (1 & dibit);        // bit 0
  }

  // CACH
  skipDibit(opts, state, 12);

  // first half current slot
  skipDibit (opts, state, 54);

  if ((bursttype == 0) || (bursttype == 1) || (bursttype == 2) || (bursttype == 3) || (bursttype == 6) || (bursttype == 7)) {
    for (i = 0; i < 96; i++) payload[i] = 0;
    processBPTC(infodata, payload);
  }

  for (i = 0; i < 8; i++) {
    fid <<= 1;
    fid |= payload[8+i];
  }
  j = fid_mapping[fid];
  if (j > 15) j = 15;

  if (print_burst == 1) {
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: %s offs: %u      inlvl: %2i%% %s %s CACH: 0x%x ",
              state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"), state->offset, level,
              state->slot0light, state->slot1light, cach1_hdr_hamming);
      if (bursttype > 9) {
          printf ("Unknown burst type: %u\n", bursttype);
      } else {
          if (bursttype == 0) {
            hexdump_packet(payload, packetdump);
            printf("PI Header: %s\n", packetdump);
          } else if ((bursttype == 1) || (bursttype == 2)) {
#ifndef NO_REEDSOLOMON
            unsigned char rs_mask = ((bursttype == 1) ? 0x96 : 0x99);
            unsigned int nerrs = check_and_fix_reedsolomon_12_09_04(&state->ReedSolomon_12_09_04, payload, rs_mask);
            state->debug_header_errors += nerrs;
#endif
            printf("%s: fid: %s (%u) \n", ((bursttype == 1) ? "VOICE Header" : "TLC"), fids[j], fid);
            if ((fid == 0) || (fid == 16)) { // Standard feature, MotoTRBO Capacity+
              char flcostr[1024];
              flcostr[0] = '\0';
              processFlco(state, payload, flcostr);
              printf("%s\n", flcostr);
            } else {
              hexdump_packet(payload, packetdump);
              printf("Unknown FLCO in voice header/TLC: %s\n", packetdump);
            }
          } else if (bursttype == 3) {
            unsigned char csbk_id = get_uint(payload+2, 6);
            printf("fid: %s (%u) ", fids[j], fid);
            if (csbk_id == 0x3d) {
              unsigned char nblks = get_uint(payload+24, 8);
              unsigned int txid = get_uint(payload+32, 24);
              unsigned int rxid = get_uint(payload+56, 24);
              printf("Preamble: %u %s blks, %s: %u RId: %u\n", nblks,
                     ((payload[0] == '1')?"Data":"CSBK"),
                     ((payload[1] == '1') ? "TGrp" : "TGid"),
                     txid, rxid);
            } else {
              printf("CSBK: id: 0x%x\n", csbk_id);
            }
          } else if (bursttype == 6) {
            process_dataheader(payload);
          } else if (bursttype == 7) {
            hexdump_packet(payload, packetdump);
            printf("RATE 1/2 DATA: %s\n", packetdump);
          } else {
            printf ("%s\n", slottype_to_string[bursttype]);
          }
      }
  }
}

