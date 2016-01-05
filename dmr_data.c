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

static const char *ack_reasons[8] = {
    "Message_Accepted",
    "Store_Forward",
    "Reg_Accepted",
    "CallBack",
    "MS_Accepted",
    "CallBack",
    "MS_ALERTING",
    "Accepted for the Status Polling Service"
};

static const char *nack_reasons[17] = {
    "Not_Supported",
    "Perm_User_Refused",
    "Temp_User_Refused",
    "Transient_Sys_Refused",
    "NoregMSaway_Refused",
    "MSaway_Refused",
    "Div_Cause_Fail",
    "SYSbusy_Refused",
    "SYS_NotReady",
    "Call_Cancel_Refused",
    "Reg_Refused",
    "Reg_Denied or MSNot_Supported",
    "IP_Connection_failed or LineNot_Supported",
    "StackFull_Refused",
    "EuipBusy_Refused",
    "Recipient_Refused",
    "Custom_Refused"
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

static const char hex[]="0123456789abcdef";
static void hexdump_packet(unsigned char *pkt, unsigned int pktlen, unsigned char packetdump[37])
{
    unsigned int j;
    for (j = 0; j < pktlen; j++) {
        unsigned char l = pkt[j];
        packetdump[3*j  ] = hex[((l >> 4) & 0x0f)];
        packetdump[3*j+1] = hex[((l >> 0) & 0x0f)];
        packetdump[3*j+2] = ' ';
    }
    packetdump[3*pktlen] = '\0';
}

static void process_dataheader(unsigned char payload[12])
{
    // Common elements to all data header types
    unsigned int j = (payload[0] & 0x0f); // j is used as header type
    unsigned int sap = (payload[1] >> 4);
    unsigned int rxid = ((payload[2] << 16) | (payload[3] << 8) | (payload[4] << 0));
    unsigned int txid = ((payload[5] << 16) | (payload[6] << 8) | (payload[7] << 0));
    printf("%s: %s: %u SrcRId: %u %c SAP: %s BF:0x%02x Misc:0x%02x\n", headertypes[j],
           ((payload[0] & 0x80) ? "Grp" : "DestRID"), rxid, txid,
           ((payload[0] & 0x40) ? 'A' : ' '), saptypes[sap], payload[8], payload[9]);
}

static unsigned int processFlco(dsd_state *state, unsigned char payload[12], char flcostr[1024])
{
    unsigned int l = 0, i, k = 0;
    unsigned char flco = (payload[0] & 0x3f);
    unsigned int talkgroup = ((payload[3] << 16) | (payload[4] << 8) | (payload[5] << 0));

    if (flco == 0) {
        l = payload[2];
        state->talkgroup = talkgroup;
        state->radio_id = ((payload[6] << 16) | (payload[7] << 8) | (payload[8] << 0));
        k = snprintf(flcostr, 1023, "Msg: Group Voice Ch Usr, SvcOpts: %u, Talkgroup: %u, RadioId: %u",
                     l, state->talkgroup, state->radio_id);
    } else if (flco == 3) {
        state->talkgroup = talkgroup;
        state->radio_id = ((payload[6] << 16) | (payload[7] << 8) | (payload[8] << 0));
        k = snprintf(flcostr, 1023, "Msg: Unit-Unit Voice Ch Usr, Talkgroup: %u, RadioId: %u",
                     state->talkgroup, state->radio_id);
    } else if (flco == 4) { 
        state->talkgroup = talkgroup;
        l = payload[6];
        state->radio_id = ((payload[7] << 16) | (payload[8] << 8) | (payload[9] << 0));
        k = snprintf(flcostr, 1023, "Msg: Capacity+ Group Voice, Talkgroup: %u, RestCh: %u, RadioId: %u",
                     state->talkgroup, l, state->radio_id);
    } else if (flco == 48) {
        unsigned int lasttg = ((payload[2] << 16) | (talkgroup >> 8));
        state->radio_id = ((payload[5] << 16) | (payload[6] << 8) | (payload[7] << 0));
        l = (payload[8] & 0x07);
        k = snprintf(flcostr, 1023, "Msg: Terminator Data LC, Talkgroup: %u, RadioId: %u, N(s): %u",
                     lasttg, state->radio_id, l);
    } else { 
        k = snprintf(flcostr, 1023, "Unknown Standard/Capacity+ FLCO: flco: 0x%x, packet_dump: ", flco);
        for (i = 2; i < 10; i++) {
            l = payload[i];
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
    unsigned int txid = ((payload[2] << 16) | (payload[3] << 8) | (payload[4] << 0));
    unsigned int rxid = ((payload[5] << 16) | (payload[6] << 8) | (payload[7] << 0));
    unsigned int l = ((payload[8] << 8) | (payload[9] << 0));
    if (csbk_id == 1) {
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Neighbors: %u %u %u %u %u ?: 0x%02x%04x",
                     payload[2], payload[3], payload[4], payload[5], payload[6], payload[7], l);
    } else if (csbk_id == 3) {
        lcn = (payload[8] >> 4);
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Voice Goto: SourceRId:%u %s:%u LCN:%u Slot:%c ?:0x%x",
                     txid, ((payload[9] & 0x01) ? "DestRId" : "GroupRId"), rxid, lcn, (((l >> 11) & 1) ? '1' : '0'), (l & 0x07ff));
    } else if (csbk_id == 6) {
        lcn = (payload[5] >> 4);
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Data Goto: RadioId:%u LCN:%u Slot:%c ?:0x%x 0x%04x%04x",
                     txid, lcn, ((payload[5] & 0x08) ? '1' : '0'), ((rxid >> 14) & 0x1f), (rxid & 0xFFFF), l);
    } else if (csbk_id == 12) {
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ ??? (Data Related): RadioId:%u ?:0x%06x%04x",
                     txid, rxid, l);
    } else if (csbk_id == 24) {
        k = snprintf(csbkstr, 1023, "Trident MS (Motorola) - Connect+ Affiliate: SourceRId:%u GroupRId:%u ?:0x%04x",
                     txid, rxid, l);
    } else {
        k = snprintf(csbkstr, 1023, "Unknown Trident MS (Motorola) - Connect+ CSBK: CSBKO: %u, packet_dump: ", csbk_id);
        for (i = 0; i < 12; i++) {
            unsigned int l = payload[i];
            csbkstr[k++] = hex[((l >> 4) & 0x0f)];
            csbkstr[k++] = hex[((l >> 0) & 0x0f)];
            csbkstr[k++] = ' ';
        }
        csbkstr[k] = '\0';
    }
    return k;
}

void processEmb (dsd_state *state, unsigned char lcss, unsigned char emb_fr[4][32])
{
  int i, k;
  unsigned int emb_deinv_fr[8];
  unsigned char fid = 0;
  unsigned char payload[12];

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

  payload[0]  = (emb_deinv_fr[0] & 0xff);
  payload[1]  = (((emb_deinv_fr[0] >> 8) & 0x07) << 5);
  payload[1] |=   (emb_deinv_fr[1] & 0x1f);
  payload[2]  = (((emb_deinv_fr[1] >> 5) & 0x3f) << 2);
  payload[2] |= (emb_deinv_fr[2] & 0x03);
  payload[3]  = ((emb_deinv_fr[2] >> 2) & 0xff);
  payload[4]  = (((emb_deinv_fr[2] >> 10) & 0x01) << 10);
  payload[4] |= (emb_deinv_fr[3] & 0xff);
  payload[5]  = (((emb_deinv_fr[3] >> 8) & 0x07) << 5);
  payload[5] |= (emb_deinv_fr[4] & 0x1f);
  payload[6]  = (((emb_deinv_fr[4] >> 5) & 0x3f) << 2);
  payload[6] |= (emb_deinv_fr[5] & 0x03);
  payload[7]  = ((emb_deinv_fr[5] >> 2) & 0xff);
  payload[8]  = (((emb_deinv_fr[5] >> 10) & 0x01) << 10);
  payload[8] |= (emb_deinv_fr[6] & 0xff);
  payload[9]  = (((emb_deinv_fr[6] >> 8) & 0x07) << 5);
  payload[9] |= (emb_deinv_fr[7] & 0x1f);
  fid = payload[1];

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
static void ProcessBPTC(unsigned char infodata[196], unsigned char payload[12])
{
  unsigned int i, j, k;
  unsigned char payload_[97];
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
     payload_[k] = data_fr[0 * 15 + i];
  }
  for (j = 1; j < 9; j++) {
    for (i = 0; i < 11; i++, k++) {
      payload_[k] = data_fr[j * 15 + i];
    }
  }
  for (j = 0; j < 12; j++) {
    payload[j] = get_uint(payload_+8*j, 8);
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
	    for (i = 0; i < 6; i++) {
            unsigned int tmp1 = ((tribits[8*i+3] << 9) | (tribits[8*i+2] << 6) | (tribits[8*i+1] << 3) | (tribits[8*i  ] << 0));
            unsigned int tmp2 = ((tribits[8*i+7] << 9) | (tribits[8*i+6] << 6) | (tribits[8*i+5] << 3) | (tribits[8*i+4] << 0));
            out[3*i  ] = ((tmp1 >> 0) & 0xFF);
            out[3*i+1] = ((tmp1 >> 8) | (tmp2 & 0x0F));
            out[3*i+2] = ((tmp2 >> 0) & 0xFF);
	    }
    }
}

#ifndef NO_REEDSOLOMON
// rs_mask = 0x96 for Voice Header, 0x99 for TLC.
static unsigned int check_and_fix_reedsolomon_12_09_04(ReedSolomon *rs, unsigned char payload[12], unsigned char rs_mask)
{
  unsigned int i, errorFlag;
  unsigned char input[255];
  unsigned char output[255];

  for (i=0; i<((1<<8)-1); i++) {
    input[i] = 0;
  }

  for(i = 0; i < 12; i++) {
    input[i] = payload[i];
  }
  input[ 9] ^= rs_mask;
  input[10] ^= rs_mask;
  input[11] ^= rs_mask;

  /* decode recv[] */
  errorFlag = rs8_decode(rs, input, output);

#if 0
  for(i = 0; i < 12; i++) {
    payload[i] = output[i];
  }
#endif
  return errorFlag;
}
#endif

void
processDMRdata (dsd_opts * opts, dsd_state * state)
{
  int i, j;
  unsigned int dibit;
  unsigned char *dibit_p;
  unsigned char cach1bits[25];
  unsigned char cach1_hdr = 0, cach1_hdr_hamming = 0;
  unsigned char infodata[196];
  unsigned char payload[12];
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
  cach1_hdr_hamming = Hamming7_4_Correct(cach1_hdr);

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
    for (i = 0; i < 12; i++) payload[i] = 0;
    ProcessBPTC(infodata, payload);
  }

  fid = payload[1];
  j = fid_mapping[fid];
  if (j > 15) j = 15;

  if (bursttype != 9) {
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: GFSK, offs: %4u, inlvl: %2i%% %s %s CACH: 0x%x %s ",
              state->ftype, state->offset, level, state->slot0light, state->slot1light, cach1_hdr_hamming,
              slottype_to_string[bursttype]);
      if ((bursttype == 1) || (bursttype == 2)) {
#ifndef NO_REEDSOLOMON
            unsigned char rs_mask = ((bursttype == 1) ? 0x96 : 0x99);
            unsigned int nerrs = check_and_fix_reedsolomon_12_09_04(&state->ReedSolomon_12_09_04, payload, rs_mask);
#endif
            printf("fid: %s (%u)\n", fids[j], fid);
            if ((fid == 0) || (fid == 16)) { // Standard feature, MotoTRBO Capacity+
              char flcostr[1024];
              flcostr[0] = '\0';
              processFlco(state, payload, flcostr);
              printf("%s\n", flcostr);
            }
      } else if ((bursttype == 3) || (bursttype == 4)) {
            unsigned char csbk_id = (payload[0] & 0x3f);
            char csbkstr[1024];
            csbkstr[0] = '\0';
            if (fid == 6) {
                ProcessConnectPlusCSBK(payload, csbk_id, csbkstr);
                printf("%s\n", csbkstr);
            } else {
                unsigned int l1 = ((payload[2] << 8) | (payload[3] << 0));
                unsigned int txid = ((payload[4] << 16) | (payload[5] << 8) | (payload[6] << 0));
                unsigned int rxid = ((payload[7] << 16) | (payload[8] << 8) | (payload[9] << 0));
                unsigned int unrecognised_csbk = 0;
                if ((fid == 0) || (fid == 8) || (fid == 104)) { // Standard Feature Set, Hyteria (8), Hyteria (104)
                    if (csbk_id == 4) {
                        snprintf(csbkstr, 1023, "Unit-to-Unit Voice Service Request: svcopts: 0x%02x, DestRId:%u, SrcRId:%u\n",
                                 payload[2], txid, rxid);
                    } else if (csbk_id == 5) {
                        snprintf(csbkstr, 1023, "Unit-to-Unit Voice Service Answer Response: svcopts: 0x%02x, resp: 0x%02x, DestRId:%u, SrcRId:%u\n",
                                 payload[2], payload[3], txid, rxid);
                    } else if (csbk_id == 7) {
                        const char *leaderprefs[4] = { "Unknown", "Low", "Med", "High" };
                        unsigned int ct = (((txid & 1) << 1) | (rxid & 1));
                        l1 = ((((payload[2] << 8) | payload[3]) & 0x07ff) >> 1); // actually 11 bits in 500ms increments, but we ignore the lsb to get it in 1sec increments.
                        snprintf(csbkstr, 1023, "Channel Timing: Generation:%u, SyncAge:%u secs, CTO:%u, LeaderID:%u (NL:%c, Pref:%s), SrcID:%u (NL:%c, Pref:%s)\n",
                                (payload[2] >> 3), l1, ct,
                                (txid >> 4), (txid & 8)+0x30, leaderprefs[((txid >> 1) & 0x03)],
                                (rxid >> 4), (rxid & 8)+0x30, leaderprefs[((rxid >> 1) & 0x03)]);
                    } else if (csbk_id == 25) {
                        unsigned char backoff_map[16] = { 0, 1, 2, 3, 4, 5, 8, 11, 15, 20, 26, 33, 41, 50, 70, 100 };
                        const char *svctypes[4] = { "All", "Payload Ch Needed", "Payload Ch Not Needed", "Reg Only" };
                        l1 = (((payload[1] << 3) | (payload[2] >> 5)) & 0x0f);
                        snprintf(csbkstr, 1023, "Aloha %s - Service Type: %s, NRand_Wait:%u, Reg:%c, Backoff Frame Len:%u, SysId:%u, MSId:%u\n",
                                ((payload[2] & 0x01) ? "Networked" : ""), svctypes[((payload[3] >> 1) & 0x03)], l1, (payload[4] & 0x20)+0x30,
                                backoff_map[(payload[4] & 0x0f)], (txid & 0xFFFF), rxid);
                    } else if (csbk_id == 26) {
                        strcpy(csbkstr, "UDT Download Header\n");
                    } else if (csbk_id == 27) {
                        strcpy(csbkstr, "UDT Upload Header\n");
                    } else if (csbk_id == 28) {
                        snprintf(csbkstr, 1023, "Ahoy - svcopts: 0x%02x (%c), svc_kind: %u(%c) N(S):%c DestRId:%u SrcRId:%u\n", 
                                (payload[2] >> 1), ((payload[2] & 0x01)+0x30), (payload[3] & 0x0f), ((payload[3] >> 6) & 0x01)+0x30,
                                ((payload[3] >> 4) & 0x03)+0x30, txid, rxid);
                    } else if (csbk_id == 30) {
                        snprintf(csbkstr, 1023, "Random Access Service Request - svcopts: 0x%02x (%c), svc_kind: %u N(S):%c DestRId:%u SrcRId:%u\n", 
                                (payload[2] >> 1), ((payload[2] & 0x01)+0x30), (payload[3] & 0x0f),
                                ((payload[3] >> 4) & 0x03)+0x30, txid, rxid);
                    } else if (csbk_id == 31) {
                        snprintf(csbkstr, 1023, "Ackvitation - svcopts: 0x%02x (%c), svc_kind: %u(%c) N(S):%c DestRId:%u SrcRId:%u\n", 
                                (payload[2] >> 1), ((payload[2] & 0x01)+0x30), (payload[3] & 0x0f), ((payload[3] >> 6) & 0x01)+0x30,
                                ((payload[3] >> 4) & 0x03)+0x30, txid, rxid);
                    } else if ((csbk_id >= 32) && (csbk_id <= 35)) {
                        unsigned int reason_code = (payload[3] & 0x1f); // FIXME: check if correct!!!
                        snprintf(csbkstr, 1023, "Ack TSCC: %s: %s, DestRId:%u, SrcRId:%u, RespInfo:0x%02x ",
                                ((payload[3] & 0x20) ? "TS to MS" : "MS to TS"), ((payload[3] & 0x80) ? "Wait/Queued" :
                                ((payload[3] & 0x40) ? ack_reasons[reason_code] : nack_reasons[reason_code])),
                                 txid, rxid, (payload[2] >> 1));
                    } else if (csbk_id == 38) {
                        snprintf(csbkstr, 1023, "Nack Response: service type: 0x%02x, reason: 0x%02x, SrcRId:%u, DestRId:%u\n",
                                 (payload[2] & 0x3f), payload[3], txid, rxid);
                    } else if (csbk_id == 40) {
                        //c_bcast(payload, csbkstr);
                    } else if (csbk_id == 46) {
                        snprintf(csbkstr, 1023, "Clear: ToChan:%u %s:%u SrcRId:%u\n",
                                 l1, ((payload[3] & 0x01) ? "GroupRId" : "DestRId"), txid, rxid);
                    } else if (csbk_id == 47) {
                        unsigned int l2 = ((payload[3] & 0x0F) >> 1); // Known: 0 = Disable PTT, 1 = Enable PTT, 2 = Clear Untargeted.
                        snprintf(csbkstr, 1023, "Protect: Cap:%u ToChan:%u %s:%u SrcRId:%u\n",
                                 l2, l1, ((payload[3] & 0x01) ? "GroupRId" : "DestRId"), txid, rxid);
                    } else if ((csbk_id > 47) && (csbk_id < 53)) {
                        static const char *grant_types[5] = { 
                            "Private Voice", "Talkgroup Voice", "Broadcast Talkgroup Voice", "Private Data/Payload Channel", "Talkgroup Data"
                        }; 
                        snprintf(csbkstr, 1023, "%s Grant Chan:%u Slot:%u %s %s %s:%u SrcRId:%u\n", 
                                 grant_types[csbk_id - 48], l1, (payload[3] & 0x08),
                                 ((payload[3] & 0x04)?"OVCM":" "), ((payload[3] & 0x02)?"EMERGENCY":" "),
                                 (((csbk_id == 48) || (csbk_id == 51)) ? "DestRId" : "GroupRId"), txid, rxid);
                    } else if (csbk_id == 56) {
                        snprintf(csbkstr, 1023, "BS Downlink Activate / Move: BId:0x%06x RId:0x%06x\n", txid, rxid);
                    } else if (csbk_id == 0x3d) {
                        snprintf(csbkstr, 1023, "Preamble: %u %s blks, %s: %u RId: %u\n",
                                 payload[3],
                                ((payload[2] & 0x80) ? "Data" : "CSBK"),
                                ((payload[2] & 0x40) ? "TGrp" : "TGid"),
                                txid, rxid);
                    } else {
                        unrecognised_csbk = 1;
                    }
                } else if (fid == 16) {
                    if ((csbk_id == 30) || (csbk_id == 62)) {
                        char tmpStr[81];
                        unsigned int lcn = (payload[2] & 0x0f);
                        if (!txid && !rxid) {
                            strcpy(tmpStr, "Idle");
                        } else {
                            snprintf(tmpStr, 80, "%u/%u/%u/%u/%u/%u",
                                     payload[4], payload[5], payload[6], payload[7], payload[8], payload[9]);
                        }
                        snprintf(csbkstr, 1023, "Capacity+ Ch Status: FL:%c Slot:%c RestCh:%u ActiveCh: %c%c%c%c%c%c (TGs: %s)\n",
                                  (payload[2] >> 6)+0x30, ((payload[2] >> 5) & 0x01)+0x30, lcn, 
                                 ((payload[3] & 0x80) ?'1':'-'), ((payload[3] & 0x40) ?'2':'-'),
                                 ((payload[3] & 0x20) ?'3':'-'), ((payload[3] & 0x10) ?'4':'-'),
                                 ((payload[3] & 0x08) ?'5':'-'), ((payload[3] & 0x04) ?'6':'-'), tmpStr);
                    } else if ((csbk_id == 31) || (csbk_id == 32)) {
                        snprintf(csbkstr, 1023, "Capacity+ %s: DestRId:%u SrcRId:%u ?:0x%02x%02x\n",
                                 ((csbk_id == 32) ? "Call Alert ACK" : "Call Alert"), txid, rxid, payload[2], payload[3]);
                    } else if (csbk_id == 36) {
                        snprintf(csbkstr, 1023, "Capacity+ Radio Check: SrcRId:%u DestRId:%u ?:0x%02x%02x\n",
                                 txid, rxid, payload[2], payload[3]);
                    } else {
                        unrecognised_csbk = 1;
                    }
                } else {
                    unrecognised_csbk = 1;
                }
                if (unrecognised_csbk) {
                    hexdump_packet(payload, 12, packetdump);
                    snprintf(csbkstr, 1023, "lb: %c, id: %u, fid: %s (%u), data: %s\n", (payload[0] >> 7)+0x30, csbk_id, fids[j], fid, packetdump);
                }
                printf("%s", csbkstr);
            }
      } else if (bursttype == 5) {
            unsigned char csbk_id = (payload[0] & 0x3f);
            if ((csbk_id == 40) || (csbk_id == 56)) {
                printf("%s Appended MBC: CDefType: %u, Channel: %u, TXFreq: %u:%uMHz, RXFreq: %u:%uMHz\n",
                       ((csbk_id == 40) ? "C_Bcast" : "C_Move"), (payload[2] >> 4),
                       (((payload[2] & 0x03) << 10) | (payload[3] << 2) | (payload[4] >> 6)),
                       (((payload[4] << 3) | (payload[5] >> 5)) & 0x3ff), 125 * (((payload[5] & 0x1f) << 8) | payload[6]),
                       (((payload[7] << 2) | (payload[8] >> 6)) & 0x3ff), 125 * (((payload[8] & 0x1f) << 8) | payload[9]));
            } else {
                hexdump_packet(payload, 12, packetdump);
                printf("Unrecognised appended MBC: lb: %c, data: %s\n", (payload[0] >> 7)+0x30, packetdump);
            }
      } else if (bursttype == 6) {
            process_dataheader(payload);
      } else if (bursttype == 7) {
            hexdump_packet(payload, 12, packetdump);
            printf("%s\n", packetdump);
      } else if (bursttype == 8) {
            unsigned char payload34[19];
            ProcessRate34Data(infodata, payload34);
            hexdump_packet(payload34, 18, packetdump);
            printf("%s\n", packetdump);
      } else {
        printf("\n");
      } 
  }
}

