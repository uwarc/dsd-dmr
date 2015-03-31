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
      "PI Header", // 0000
      "VOICE Header:", // 0001
      "TLC:", // 0010
      "CSBK:", // 0011
      "MBC Header:", // 0100
      "MBC:", // 0101
      "DATA Header:", // 0110
      "RATE 1/2 DATA:", // 0111
      "RATE 3/4 DATA:", // 1000
      "Slot idle", // 1001
      "Rate 1 DATA", // 1010
      "Unknown/Bad (11)", // 1011
      "Unknown/Bad (12)", // 1100
      "Unknown/Bad (13)", // 1101
      "Unknown/Bad (14)", // 1110
      "Unknown/Bad (15)"  // 1111
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

static char *headertypes[16] = {
    "UDT ", "Resp", "UDat", "CDat", "Hdr4", "Hdr5", "Hdr6", "Hdr7",
    "Hdr8", "Hdr9", "Hdr10", "Hdr11", "Hdr12", "DSDa", "RSDa", "Prop"
};

static char *saptypes[16] = {
    "UDT", "1", "TCP HC", "UDP HC", "IP Pkt", "ARP Pkt", "6", "7",
    "8", "Prop Pkt", "Short Data", "11", "12", "13", "14", "15"
}; 

static unsigned char TrellisDibitDeinterleave[49] = 
{
    0,4,8,12,16,20,24,28,32,36,40,44,48,
    1,5,9,13,17,21,25,29,33,37,41,45,
    2,6,10,14,18,22,26,30,34,38,42,46,
    3,7,11,15,19,23,27,31,35,39,43,47,
};

static unsigned char TrellisDibitToConstellationPoint[16] = {
    11, 12, 0, 7, 14, 9, 5, 2, 10, 13, 1, 6, 15, 8, 4, 3
}; 

static unsigned char STATETABLE[]={
    0,8,4,12,2,10,6,14,
    4,12,2,10,6,14,0,8,
    1,9,5,13,3,11,7,15,
    5,13,3,11,7,15,1,9,
    3,11,7,15,1,9,5,13,
    7,15,1,9,5,13,3,11,
    2,10,6,14,0,8,4,12,
    6,14,0,8,4,12,2,10
};

static unsigned char cach_deinterleave[24] = {
     0,  7,  8,  9,  1, 10, 11, 12,
     2, 13, 14, 15,  3, 16,  4, 17,
    18, 19,  5, 20, 21, 22,  6, 23
}; 

static unsigned char hamming7_4_encode[16] =
{
    0x00, 0x0b, 0x16, 0x1d, 0x27, 0x2c, 0x31, 0x3a,
    0x45, 0x4e, 0x53, 0x58, 0x62, 0x69, 0x74, 0x7f
};

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

    *block = (codeword >> 4);
}

static const char hex[]="0123456789abcdef";
static void hexdump_packet(unsigned char payload[64], unsigned char packetdump[31])
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
    unsigned int sap = get_uint(payload+8, 4);
    printf("%s: %s: %u SrcRId: %u %c SAP: %s BF:0x%02x Misc:0x%02x\n", headertypes[j],
           ((payload[0])?"Grp":"DestRID"), get_uint(payload+16, 24), get_uint(payload+40, 24),
           ((payload[1]) ? 'A' : ' '), saptypes[sap], get_uint(payload+64, 8), get_uint(payload+72, 8));
}

static unsigned int processFlco(dsd_state *state, unsigned char payload[97], char flcostr[1024])
{
  unsigned int l = 0, i, k = 0, lasttg = 0, radio_id = 0;
  unsigned char flco = get_uint(payload+2, 6);

  if (flco == 0) {
    l = get_uint(payload+16, 8);
    state->talkgroup = get_uint(payload+24, 24);
    state->radio_id = get_uint(payload+48, 24);
    k = snprintf(flcostr, 1023, "Msg: Group Voice Ch Usr, SvcOpts: %u, Talkgroup: %u, RadioId: %u",
                 l, state->talkgroup, state->radio_id);
  } else if (flco == 3) {
    state->talkgroup = get_uint(payload+24, 24);
    state->radio_id = get_uint(payload+48, 24);
    k = snprintf(flcostr, 1023, "Msg: Unit-Unit Voice Ch Usr, Talkgroup: %u, RadioId: %u",
                 state->talkgroup, state->radio_id);
  } else if (flco == 4) { 
    state->talkgroup = get_uint(payload+24, 24);
    l = get_uint(payload+48, 24);
    state->radio_id = get_uint(payload+56, 24);
    k = snprintf(flcostr, 1023, "Msg: Capacity+ Group Voice, Talkgroup: %u, RestCh: %u, RadioId: %u",
                 state->talkgroup, l, state->radio_id);
  } else if (flco == 48) {
    lasttg = get_uint(payload+16, 24);
    radio_id = get_uint(payload+40, 24);
    l = ((payload[69] << 2) | (payload[70] << 1) | (payload[71] << 0));
    k = snprintf(flcostr, 1023, "Msg: Terminator Data LC, Talkgroup: %u, RadioId: %u, N(s): %u",
                 lasttg, radio_id, l);
  } else { 
    k = snprintf(flcostr, 1023, "Unknown Standard/Capacity+ FLCO: flco: 0x%x, packet_dump: ", flco);
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

static unsigned int ProcessConnectPlusCSBK(unsigned char *payload, unsigned int csbk_id, char csbkstr[1024])
{
    // The information to decode these packets was kindly provided by inigo88 on the Radioreference forums
    // see http://forums.radioreference.com/digital-voice-decoding-software/213131-understanding-connect-plus-trunking-6.html#post1866950
    //                 67 890123 45 678901 23 456789 01 234567 89 012345 6789 0123 4567 8901 2345 6789
    // CSBKO=1 + FID=6 00 000001 00 000011 00 000100 00 000101 00 000110 0000 0000 0000 0000 0000 1110
    //                         1         3         4         5         6                          
    // bits 40 - 64 have an unknown purpose
    //
    unsigned int i, k = 0, lcn;
    unsigned int txid = get_uint(payload, 24);
    unsigned int rxid = get_uint(payload+24, 24);
    if (csbk_id == 1) {
        unsigned int l = get_uint(payload+40, 24);
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Neighbors: %u %u %u %u %u ?: 0x%06x\n",
                     get_uint(payload, 8), get_uint(payload+8, 8), get_uint(payload+16, 8),
                     get_uint(payload+24, 8), get_uint(payload+32, 8), l);
    } else if (csbk_id == 3) {
        lcn = get_uint(payload+48, 4);
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Voice Goto: SourceRId:%u %s:%u LCN:%u Slot:%c ?:0x%x\n",
                     txid, (payload[63] ? "DestRId" : "GroupRId"), rxid,
                     lcn, (payload[52] ? '1' : '0'), get_uint(payload+53, 11));
    } else if (csbk_id == 6) {
        lcn = get_uint(payload+24, 4);
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Data Goto: RadioId:%u LCN:%u Slot:%c ?:0x%x 0x%08x\n",
                     txid, lcn, (payload[28] ? '1' : '0'),
                     get_uint(payload+29, 5), get_uint(payload+32, 32));
    } else if (csbk_id == 12) {
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ ??? (Data Related): RadioId:%u ?:0x%06x%04x\n",
                     txid, rxid, get_uint(payload+48, 16));  
    } else if (csbk_id == 24) {
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Affiliate: SourceRId:%u GroupRId:%u ?:0x%04x\n",
                     txid, rxid, get_uint(payload+48, 16));  
    } else {
        k = snprintf(csbkstr, 1023, "Unknown Trident MS (Motorola) - Connect+ CSBK: CSBKO: %u, packet_dump: ", csbk_id);
        for (i = 0; i < 8; i++) {
            unsigned int l = get_uint(payload+i*8, 8);
            csbkstr[k++] = hex[((l >> 4) & 0x0f)];
            csbkstr[k++] = hex[((l >> 0) & 0x0f)];
            csbkstr[k++] = ' ';
        }
        csbkstr[k++] = '\n';
        csbkstr[k] = '\0';
    }
    return k;
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
  }
}

static unsigned int tribitExtract (unsigned char tribit[49], unsigned char constellation[49]){
	unsigned int a, b, rowStart, lastState=0;
    unsigned char match;

	for (a=0;a<49;a++) {
		// The lastState variable decides which row of STATETABLE we should use
		rowStart=lastState*8;
		match=0;
		for (b=0;b<8;b++) {
			// Check if this constellation point matches an element of this row of STATETABLE
			if (constellation[a] == STATETABLE[rowStart + b]) {
				// Yes it does
				match=1;
				lastState=b;
				tribit[a]=lastState;
			}
		}

		// If no match found then we have a problem
		if (!match){
			printf("TRIBIT EXTRACT no match %u !!!\n", a);
            return 0;
		}
	}
    return 1;
}

// deinterleaved_data[a] = rawdata[(a*181)%196];
static void ProcessBPTC(unsigned char infodata[196], unsigned char payload[97])
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
    codeword &= 0x1ff;
    for (j = 0; j < 9; j++) {
      data_fr[j * 15 + i] = ((codeword >> (8 - j)) & 1);
    }
  }
  for (j = 0; j < 9; j++) {
    unsigned int codeword = 0;
    for (i = 0; i < 15; i++) {
        codeword <<= 1;
        codeword |= data_fr[j * 15 + i];
    }
    Hamming15_11_3_Correct(&codeword);
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

static void ProcessRate34Data(unsigned char infodata[196], unsigned char out[18]){
    unsigned char ConstellationPoints[49];
    unsigned char tribits[49];
    unsigned int a, i = 0, trellisDibit, deinterleave, have_tribits = 0;

	for (a=0;a<196;a=a+4) {
        trellisDibit = ((infodata[a] << 3) | (infodata[a+1] << 2) | (infodata[a+2] << 1) | (infodata[a+3] << 0));
		deinterleave = TrellisDibitDeinterleave[i++];
        ConstellationPoints[deinterleave] = TrellisDibitToConstellationPoint[trellisDibit];
	}
	have_tribits = tribitExtract(tribits, ConstellationPoints);

    if (have_tribits) {
        // lcm(3,8) == 24, so we convert 8 tribits to 3 packed binary bytes at a time.
	    for (a=0;a<48;a=a+8) {
            unsigned int tmp = ((tribits[a+2] << 6) | (tribits[a+1] << 3) | (tribits[a] << 0));
            tmp |= ((tribits[a+5] << 15) | (tribits[a+4] << 12) | (tribits[a+3] << 9));
            tmp |= ((tribits[a+7] << 12) | (tribits[a+6] << 18));
            out[3*i  ] = ((tmp >> 0) & 0xFF);
            out[3*i+1] = ((tmp >> 8) & 0xFF);
            out[3*i+2] = ((tmp >> 16) & 0xFF);
            i++;
	    }
    }
}

void
processDMRdata (dsd_opts * opts, dsd_state * state)
{
  int i, j;
  unsigned int dibit;
  unsigned char *dibit_p;
  unsigned char cach1bits[25];
  unsigned char cach1_hdr = 0, cach1_hdr_hamming = 0;
  unsigned char infodata[196];
  unsigned char payload[114];
  unsigned char packetdump[81];
  unsigned int golay_codeword = 0;
  unsigned int bursttype = 0;
  unsigned char fid = 0;

  if (state->firstframe == 1) { // we don't know if anything received before the first sync after no carrier is valid
    //skipDibit(opts, state, 120);
    state->firstframe = 0;
  }

  dibit_p = state->dibit_buf_p - 66;

  // CACH
  for (i = 0; i < 12; i++) {
      dibit = *dibit_p++;
      cach1bits[cach_deinterleave[2*i]] = (1 & (dibit >> 1));    // bit 1
      cach1bits[cach_deinterleave[2*i+1]] = (1 & dibit);   // bit 0
  }
  cach1_hdr  = ((cach1bits[0] << 6) | (cach1bits[1] << 5) | (cach1bits[2] << 4) | (cach1bits[3] << 3));
  cach1_hdr |= ((cach1bits[4] << 2) | (cach1bits[5] << 1) | (cach1bits[6] << 0));
  cach1_hdr_hamming = hamming7_4_correct(cach1_hdr);

  state->currentslot = ((cach1_hdr_hamming >> 2) & 1);      // bit 2 
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
      dibit = *dibit_p++;
      infodata[2*i] = (1 & (dibit >> 1)); // bit 1
      infodata[2*i+1] = (1 & dibit);        // bit 0
  }

  // slot type
  golay_codeword  = *dibit_p++;
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;

  dibit = *dibit_p++;
  bursttype = dibit;

  dibit = *dibit_p++;
  bursttype = ((bursttype << 2) | dibit);
  golay_codeword = ((golay_codeword << 4)|bursttype);

  // parity bit
  golay_codeword <<= 2;
  golay_codeword |= *dibit_p++;

  // signaling data or sync
  if ((state->lastsynctype & ~1) == 4) {
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

  golay_codeword &= 0x0f;
  bursttype ^= golay_codeword;
  if (bursttype & 1) state->debug_header_errors++;
  if (bursttype & 2) state->debug_header_errors++;
  if (bursttype & 4) state->debug_header_errors++;
  if (bursttype & 8) state->debug_header_errors++;

  bursttype = golay_codeword;
  if ((bursttype == 1) || (bursttype == 2)) {
    closeMbeOutFile (opts, state);
    state->errs2 = 0;
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

  if (bursttype <= 7) {
    for (i = 0; i < 96; i++) payload[i] = 0;
    ProcessBPTC(infodata, payload);
  }

  for (i = 0; i < 8; i++) {
    fid <<= 1;
    fid |= payload[8+i];
  }
  j = fid_mapping[fid];
  if (j > 15) j = 15;

  if (bursttype != 9) {
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: GFSK, offs: %4u, inlvl: %2i%% %s %s CACH: 0x%x %s ",
              state->ftype, state->offset, level, state->slot0light, state->slot1light, cach1_hdr_hamming,
              slottype_to_string[bursttype]);
      if ((bursttype == 1) || (bursttype == 2)) {
            printf("fid: %s (%u)\n", fids[j], fid);
            if ((fid == 0) || (fid == 16)) { // Standard feature, MotoTRBO Capacity+
              char flcostr[1024];
              flcostr[0] = '\0';
              processFlco(state, payload, flcostr);
              printf("%s\n", flcostr);
            }
      } else if ((bursttype == 3) || (bursttype == 4)) {
            unsigned char csbk_id = get_uint(payload+2, 6);
            unsigned int txid = get_uint(payload+32, 24);
            unsigned int rxid = get_uint(payload+56, 24);
            char csbkstr[1024];
            csbkstr[0] = '\0';
            if (csbk_id == 0x3d) {
              unsigned char nblks = get_uint(payload+24, 8);
              printf("Preamble: %u %s blks, %s: %u RId: %u\n", nblks,
                     ((payload[16]) ? "Data" : "CSBK"),
                     ((payload[17]) ? "TGrp" : "TGid"),
                     txid, rxid);
            } else {
              if (fid == 6) {
                ProcessConnectPlusCSBK(payload+16, csbk_id, csbkstr);
                printf("%s", csbkstr);
              } else if (fid == 16) {
                unsigned int l1 = get_uint(payload+16, 16);
                if ((csbk_id == 30) || (csbk_id == 62)) {
                    char tmpStr[81];
                    unsigned int lcn = get_uint(payload+20, 4);
                    if (!txid && !rxid) {
                        strcpy(tmpStr, "Idle");
                    } else {
                        snprintf(tmpStr, 80, "TGs: %u/%u/%u/%u/%u/%u",
                                ((txid >> 16) & 0xFF), ((txid >> 8) & 0xFF), ((txid >> 0) & 0xFF),
                                ((rxid >> 16) & 0xFF), ((rxid >> 8) & 0xFF), ((rxid >> 0) & 0xFF));
                    }
                    printf("Capacity+ Ch Status: FL:%c Slot:%c RestCh:%u ActiveCh: %c%c%c%c%c%c (%s)\n",
                            ((payload[16] << 1) | payload[17])+0x30, payload[18]+0x30, lcn, 
                            (payload[24] ?'1':'-'), (payload[25] ?'2':'-'), (payload[26] ?'3':'-'), (payload[27] ?'4':'-'),
                            (payload[28] ?'5':'-'), (payload[29] ?'6':'-'), tmpStr);
                } else if ((csbk_id == 31) || (csbk_id == 32)) {
                    printf("Capacity+ %s: DestRId:%u SrcRId:%u ?:0x%04x\n",
                             ((csbk_id == 32) ? "Call Alert ACK" : "Call Alert"), txid, rxid, l1);
                } else if (csbk_id == 36) {
                    printf("Capacity+ Radio Check: SrcRId:%u DestRId:%u ?:0x%04x\n",
                             txid, rxid, l1);
                } else {
                  printf("lb: %c, id: %u, fid: %s (%u)\n", payload[0]+0x30, csbk_id, fids[j], fid);
                }
              } else {
                printf("lb: %c, id: %u, fid: %s (%u)\n", payload[0]+0x30, csbk_id, fids[j], fid);
              }
            }
      } else if (bursttype == 5) {
            unsigned char csbk_id = get_uint(payload+2, 6);
            if ((csbk_id == 40) || (csbk_id == 56)) {
                printf("%s Appended MBC: CDefType: %u, Channel: %u, TXFreq: %u:%uMHz, RXFreq: %u:%uMHz\n",
                       ((csbk_id == 40) ? "C_Bcast" : "C_Move"), get_uint(payload+16, 4), get_uint(payload+22, 12),
                       get_uint(payload+33, 10), 125 * get_uint(payload+43, 13),
                       get_uint(payload+56, 10), 125 * get_uint(payload+66, 13));
            } else {
                hexdump_packet(payload, packetdump);
                printf("Unrecognised appended MBC: lb: %c, data: %s\n", payload[0]+0x30, packetdump);
            }
      } else if (bursttype == 6) {
            process_dataheader(payload);
      } else if (bursttype == 7) {
            hexdump_packet(payload, packetdump);
            printf("%s\n", packetdump);
      } else if (bursttype == 8) {
            unsigned char payload34[12];
            ProcessRate34Data(infodata, payload34);
            for (j = 0; j < 12; j++) {
                unsigned char l = payload34[j];
                packetdump[3*j  ] = hex[((l >> 4) & 0x0f)];
                packetdump[3*j+1] = hex[((l >> 0) & 0x0f)];
                packetdump[3*j+2] = ' ';
            }
            packetdump[36] = '\0';
            printf("%s\n", packetdump);
      } else {
        printf("\n");
      } 
  }
}

