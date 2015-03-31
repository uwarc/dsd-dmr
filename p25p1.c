#include "dsd.h"
#include "p25p1_const.h"

static unsigned char mfid_mapping[64] = {
     0,  1,  2, 0,  3,  0,  4, 0,
     5,  0,  6, 0,  7,  8,  9, 0,
    10,  0, 11, 0, 12, 13, 14, 0,
    15, 16, 17, 0, 18, 19, 20, 21,
    22, 23,  0, 0, 24, 0,  0,  0,
    25, 26,  0, 0, 27, 0,  0,  0,
    28,  0, 29, 0, 30, 0, 31,  0,
    32,  0,  0, 0, 33, 0, 34, 35
};

static const char *mfids[36] = {
   "Standard (pre-2001)",
   "Standard (post-2001)",
   "Aselsan Inc.",
   "Relm / BK Radio",
   "EADS Public Safety Inc.",
   "Cycomm",
   "Efratom Time and Frequency Products, Inc",
   "Com-Net Ericsson",
   "Etherstack",
   "Datron",
   "Icom",
   "Garmin",
   "GTE",
   "IFR Systems",
   "INIT Innovations in Transportation, Inc",
   "GEC-Marconi",
   "Harris Corp.",
   "Kenwood Communications",
   "Glenayre Electronics",
   "Japan Radio Co.",
   "Kokusai",
   "Maxon",
   "Midland",
   "Daniels Electronics Ltd.",
   "Motorola",
   "Thales",
   "M/A-COM",
   "Raytheon",
   "SEA",
   "Securicor",
   "ADI",
   "Tait Electronics",
   "Teletec",
   "Transcrypt International",
   "Vertex Standard",
   "Zetron, Inc"
};

static unsigned int Hamming1064Gen[6] = {
    0x20e, 0x10d, 0x08b, 0x047, 0x023, 0x01c
};

static unsigned int Hamming15113Gen[11] = {
    0x400f, 0x200e, 0x100d, 0x080c, 0x040b, 0x020a, 0x0109, 0x0087, 0x0046, 0x0025, 0x0013
};

void p25_hamming10_6_4_decode(unsigned int *codeword)
{
  unsigned int i, block = *codeword, ecc = 0, syndrome;

  for(i = 0; i < 6; i++) {
      if((block & Hamming1064Gen[i]) > 0xf)
          ecc ^= Hamming1064Gen[i];
  }
  syndrome = ecc ^ block;

  if (syndrome > 0) {
      block ^= (1U << (syndrome - 1));
  }

  *codeword = (block >> 4);
}

void p25_hamming15_11_3_decode(unsigned int *codeword)
{
  unsigned int i, block = *codeword, ecc = 0, syndrome;

  for(i = 0; i < 11; i++) {
      if((block & Hamming15113Gen[i]) > 0xf)
          ecc ^= Hamming15113Gen[i];
  }
  syndrome = ecc ^ block;

  if (syndrome > 0) {
      block ^= (1U << (syndrome - 1));
  }

  *codeword = (block >> 4);
}

static unsigned int Cyclic1685Gen[8] = {
    0x804e, 0x4027, 0x208f, 0x10db, 0x08f1, 0x04e4, 0x0272, 0x0139
};

void p25_lsd_cyclic1685_decode(unsigned int *codeword)
{
  unsigned int i, block = *codeword, ecc = 0, syndrome;
  for(i = 0; i < 8; i++) {
      if((block & Cyclic1685Gen[i]) > 0xff)
          ecc ^= Cyclic1685Gen[i];
  }
  syndrome = ecc ^ block;
  if (syndrome > 0) {
      block ^= (1U << (syndrome - 1));
  }
  *codeword = (block >> 8);
}

void
read_dibit (dsd_opts* opts, dsd_state* state, unsigned char* output, unsigned int count, int* status_count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        if (*status_count == 35) {
            // Status bits now
            // TODO: do something useful with the status bits...
            //int status = getDibit (opts, state);
            getDibit (opts, state);
            *status_count = 1;
        } else {
            (*status_count)++;
        }

        output[i] = getDibit(opts, state);
    }
}

void
skip_dibit(dsd_opts* opts, dsd_state* state, unsigned int count, int* status_count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        if (*status_count == 35) {
            // Status bits now
            // TODO: do something useful with the status bits...
            //int status = getDibit (opts, state);
            getDibit (opts, state);
            *status_count = 1;
        } else {
            (*status_count)++;
        }

        getDibit(opts, state);
    }
}

static unsigned int mbe_golay2312 (unsigned int in, unsigned int *out)
{
  unsigned int i, errs = 0, block = in;

  Golay23_Correct (&block);
  *out = block;

  in >>= 11;
  in ^= block;
  for (i = 0; i < 12; i++) {
      if ((in >> i) & 1) {
          errs++;
      }
  }

  return (errs);
}

static void mbe_demodulateImbe7200x4400Data (char imbe[8][23])
{
  int i, j = 0, k;
  unsigned short pr[115], foo = 0;

  // create pseudo-random modulator
  for (i = 11; i >= 0; i--) {
      foo <<= 1;
      foo |= imbe[0][11+i];
  }

  pr[0] = (16 * foo);
  for (i = 1; i < 115; i++) {
      pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) >> 16));
  }
  for (i = 1; i < 115; i++) {
      pr[i] >>= 15;
  }

  // demodulate imbe with pr
  k = 1;
  for (i = 1; i < 4; i++) {
      for (j = 22; j >= 0; j--)
        imbe[i][j] ^= pr[k++];
  }
  for (i = 4; i < 7; i++) {
      for (j = 14; j >= 0; j--)
        imbe[i][j] ^= pr[k++];
  }
}

static int mbe_eccImbe7200x4400Data (char imbe_fr[8][23], char *imbe_d)
{
  int i, j = 0, errs = 0;
  unsigned int hin, gin, gout, block;
  char *imbe = imbe_d;

  for (j = 11; j >= 0; j--) {
    *imbe++ = imbe_fr[0][j+11];
  }

  for (i = 1; i < 4; i++) {
      gin = 0;
      for (j = 22; j >= 0; j--) {
          gin <<= 1;
          gin |= imbe_fr[i][j];
      }
      errs += mbe_golay2312 (gin, &gout);
      for (j = 0; j < 12; j++) {
          *imbe++ = ((gout >> j) & 1);
      }
  }
  for (i = 4; i < 7; i++) {
      hin = 0;
      for (j = 0; j < 15; j++) {
          hin <<= 1;
          hin |= imbe_fr[i][14-j];
      }
      block = hin;
      p25_hamming15_11_3_decode(&block);
      errs += ((hin >> 4) != block);
      for (j = 14; j >= 4; j--) {
          *imbe++ = ((block & 0x0400) >> 10);
          block = block << 1;
      }
  }
  for (j = 6; j >= 0; j--) {
      *imbe++ = imbe_fr[7][j];
  }

  return (errs);
}

void
process_IMBE (dsd_opts* opts, dsd_state* state, int* status_count)
{
  unsigned int i, dibit;
  unsigned char imbe_dibits[72];
  char imbe_fr[8][23];
  const int *w, *x, *y, *z;

  w = iW;
  x = iX;
  y = iY;
  z = iZ;

  read_dibit(opts, state, imbe_dibits, 72, status_count);

  for (i = 0; i < 72; i++) {
      dibit = imbe_dibits[i];
      imbe_fr[*w][*x] = (1 & (dibit >> 1)); // bit 1
      imbe_fr[*y][*z] = (1 & dibit);        // bit 0
      w++;
      x++;
      y++;
      z++;
  }

  //if (state->p25kid == 0)
  {
    // Check for a non-standard c0 transmitted. This is explained here: https://github.com/szechyjs/dsd/issues/24
    unsigned int nsw = 0x00038000;
    unsigned int gin = 0, gout;
    int j;
    for (j = 22; j >= 0; j--) {
        gin <<= 1;
        gin |= imbe_fr[0][j];
    }

    if (gin == nsw) {
        // Skip this particular value. If we let it pass it will be signaled as an erroneus IMBE
        printf("(Non-standard IMBE c0 detected, skipped)\n");
    } else {
        char imbe_d[88];
        state->errs2 = mbe_golay2312 (gin, &gout);
        for (j = 11; j >= 0; j--) {
            imbe_fr[0][j+11] = (gout & 0x0800) >> 11;
            gout <<= 1;
        }
        for (j = 0; j < 88; j++) {
            imbe_d[j] = 0;
        }
        mbe_demodulateImbe7200x4400Data (imbe_fr);
        state->errs2 += mbe_eccImbe7200x4400Data (imbe_fr, imbe_d);
        processIMBEFrame (opts, state, imbe_d);
    }
  }
}

/**
 * Corrects a hex (12 bit) word using the Golay 23 FEC.
 */
static unsigned int
correct_hex_word (dsd_state* state, unsigned char *hex_and_parity, unsigned int codeword_bits)
{
  unsigned int i, golay_codeword = 0, golay_codeword_old = 0, fixed_errors = 0;

  // codeword now contains:
  // bits 0-10: golay (23,12) parity bits
  // bits 11-22: hex bits

  for (i = 0; i < codeword_bits; i++) {
    golay_codeword <<= 2;
    golay_codeword |= hex_and_parity[i];
  }
  golay_codeword >>= 1;
  golay_codeword_old = (golay_codeword >> 11);
  Golay23_Correct(&golay_codeword);

  if (golay_codeword_old != golay_codeword) {
    fixed_errors++;
  }

  state->debug_header_errors += fixed_errors;
  return golay_codeword;
}

void update_p25_error_stats (dsd_state* state, unsigned int nbits, unsigned int n_errs)
{
    state->p25_bit_count += nbits;
    state->p25_bit_error_count += n_errs;
    while ((!(state->p25_bit_count & 1)) && (!(state->p25_bit_error_count & 1))) {
        state->p25_bit_count >>= 1;
        state->p25_bit_error_count >>= 1;
    }
}

float get_p25_ber_estimate (dsd_state* state)
{
    float ber = 0.0f;
    if (state->p25_bit_count > 0) {
        ber = (((float)state->p25_bit_count) * 100.0f / ((float)state->p25_bit_error_count));
    }
    return ber;
}

/**
 * The important method that processes a full P25 HD unit.
 */
static unsigned int 
processHDU(dsd_opts* opts, dsd_state* state)
{
  int i;
  unsigned char mfid = 0;
  unsigned char algid = 0;
  unsigned int talkgroup = 0;
  int status_count;
  unsigned char hex_and_parity[18];
  unsigned char hex_data[20];    // Data in hex-words (6 bit words). A total of 20 hex words.

  // we skip the status dibits that occur every 36 symbols
  // the next status symbol comes in 14 dibits from here
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  // Read 20 hex words, correct them using their Golay23 parity data.
  for (i = 0; i < 20; i++) {
      // read both the hex word and the Golay(23, 12) parity information
      read_dibit(opts, state, hex_and_parity, 9, &status_count);
      // Use the Golay23 FEC to correct it.
      hex_data[i] = correct_hex_word(state, hex_and_parity, 9);
  }

  // Skip the 16 parity hex word.
  skip_dibit(opts, state, 9*16, &status_count);

  // Now put the corrected data on the DSD structures
  mfid = ((hex_data[12] << 2) | (hex_data[13] & 3));

  // The important algorithm ID. This indicates whether the data is encrypted,
  // and if so what is the encryption algorithm used.
  // A code 0x80 here means that the data is unencrypted.
  algid  = (((hex_data[13] >> 2) << 4) | (hex_data[14] & 0x0f));
  state->p25enc = (algid != 0x80);

  skipDibit (opts, state, 6);

  if ((mfid == 144) || (mfid == 9)) { // FIXME: only one of these is correct.
    talkgroup  = ((hex_data[18] << 6) | hex_data[19]);
  } else {
    talkgroup  = (((hex_data[17] >> 2) << 12) | (hex_data[18] << 6) | (hex_data[19]));
  }

  state->talkgroup = talkgroup;
  return mfid;
}

unsigned int
read_and_correct_hex_word (dsd_opts* opts, dsd_state* state, int* status_count)
{
  unsigned char hex_and_parity[40];
  unsigned int i, value_in[4], value = 0, value_out = 0;

  // Read the hexword and parity
  read_dibit(opts, state, hex_and_parity, 20, status_count);

  // Use Hamming to error correct the hex word
  // in the bitset 9 is the left-most and 0 is the right-most
  value_in[3] = value_in[2] = value_in[1] = value_in[0] = 0;  
  for (i=0; i<5; i++) {
      value_in[0] <<= 2;
      value_in[0] |= hex_and_parity[i];
      value_in[1] <<= 2;
      value_in[1] |= hex_and_parity[i+5];
      value_in[2] <<= 2;
      value_in[2] |= hex_and_parity[i+10];
      value_in[3] <<= 2;
      value_in[3] |= hex_and_parity[i+15];
  }
  value = value_in[0];
  p25_hamming10_6_4_decode(&value);
  value_in[0] >>= 4;
  if (value != value_in[0]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[1];
  p25_hamming10_6_4_decode(&value);
  value_in[1] >>= 4;
  if (value != value_in[1]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[2];
  p25_hamming10_6_4_decode(&value);
  value_in[2] >>= 4;
  if (value != value_in[2]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[3];
  p25_hamming10_6_4_decode(&value);
  value_in[3] >>= 4;
  if (value != value_in[3]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  return value_out;
}

static void
processLDU1 (dsd_opts* opts, dsd_state* state, char *outstr, unsigned int outlen)
{
  // extracts IMBE frames from LDU frame
  unsigned int i, lsd1 = 0, lsd2 = 0;
  unsigned int lcinfo[3];    // Data in hex-words (6 bit words), stored packed in groups of four, in a uint32_t
  unsigned char lsd[16];
  unsigned char mfid;
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  // IMBE 1
  process_IMBE (opts, state, &status_count);

  // IMBE 2
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 2
  lcinfo[0] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  lcinfo[1] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  lcinfo[2] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 5
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 5
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 6
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 6
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 7
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 7
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 8
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 8: LSD (low speed data)
  read_dibit(opts, state, lsd, 16, &status_count);

  for (i=0; i<8; i++) {
    lsd1 <<= 2;
    lsd1 |= lsd[i];
    lsd2 <<= 2;
    lsd2 |= lsd[i+8];
  }

  p25_lsd_cyclic1685_decode(&lsd1);
  p25_lsd_cyclic1685_decode(&lsd2);
  // TODO: do something useful with the LSD bytes...

  // IMBE 9
  process_IMBE (opts, state, &status_count);

  // trailing status symbol
  getDibit (opts, state);

  state->talkgroup = (lcinfo[1] & 0xFFFF);
  state->radio_id = lcinfo[2];

  mfid  = ((lcinfo[0] >> 10) & 0xFF);
  snprintf(outstr, outlen, "LDU1: e: %u, mfid: %s (%u), talkgroup: %u, src: %u, lsd: 0x%02x/0x%02x",
          state->errs2, mfids[mfid_mapping[mfid&0x3f]], mfid, state->talkgroup, state->radio_id, lsd1, lsd2);
  decode_p25_lcf(lcinfo);
}

static unsigned int 
processLDU2 (dsd_opts * opts, dsd_state * state)
{
  // extracts IMBE frames from LDU frame
  unsigned char lsd[16];
  unsigned int i, lsd1 = 0, lsd2 = 0;
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  // IMBE 1
  process_IMBE (opts, state, &status_count);

  // IMBE 2
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 2
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 5
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 5
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 6
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 6
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 7
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 7
  skip_dibit (opts, state, 20, &status_count);

  // IMBE 8
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 8: LSD (low speed data)
  read_dibit(opts, state, lsd, 16, &status_count);

  for (i=0; i<8; i++) {
    lsd1 <<= 2;
    lsd1 |= lsd[i];
    lsd2 <<= 2;
    lsd2 |= lsd[i+8];
  }

  p25_lsd_cyclic1685_decode(&lsd1);
  p25_lsd_cyclic1685_decode(&lsd2);
  //state->p25lsd1 = lsd1;
  //state->p25lsd2 = lsd2;
  // TODO: do something useful with the LSD bytes...

  // IMBE 9
  process_IMBE (opts, state, &status_count);

  // trailing status symbol
  getDibit (opts, state);

  return state->errs2;
}

static void
processTDU (dsd_opts* opts, dsd_state* state)
{
    // we skip the status dibits that occur every 36 symbols
    // the first IMBE frame starts 14 symbols before next status
    // so we start counter at 36-14-1 = 21

    // Next 14 dibits should be zeros
    skipDibit(opts, state, 14);

    // Next we should find an status dibit
    getDibit (opts, state);
}

static unsigned char 
processTDULC (dsd_opts* opts, dsd_state* state)
{
  unsigned int j, corrected_hexword = 0;
  unsigned char hex_and_parity[12];
  unsigned int lcinfo[3];
  unsigned int dodeca_data[6];    // Data in 12-bit words. A total of 6 words.
  unsigned char mfid;
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  for (j = 0; j < 6; j++) {
      // read both the hex word and the Golay(23, 12) parity information
      read_dibit(opts, state, hex_and_parity, 12, &status_count);

      // Use the Golay23 FEC to correct it.
      corrected_hexword = correct_hex_word(state, hex_and_parity, 12);

      // codeword is now hopefully fixed
      // put it back into our hex format
      dodeca_data[5-j] = corrected_hexword;
  }

  skip_dibit(opts, state, 72, &status_count);

  // Next 10 dibits should be zeros
  skip_dibit(opts, state, 10, &status_count);

  // trailing status symbol
  getDibit (opts, state);

  // Put the corrected data into the DSD structures
  lcinfo[0]  = (dodeca_data[5]);
  lcinfo[0] |= (dodeca_data[4] << 12);
  lcinfo[1]  = (dodeca_data[3]);
  lcinfo[1] |= (dodeca_data[2] << 12);
  lcinfo[2]  = (dodeca_data[1]);
  lcinfo[2] |= (dodeca_data[0] << 12);

  mfid  = ((lcinfo[0] >> 10) & 0xFF);
  decode_p25_lcf(lcinfo);
  return mfid;
}

static unsigned int
trellis_1_2_decode(uint8_t *in, uint32_t in_sz, uint8_t *out)
{
   uint8_t state = 0;
   unsigned int i, j;

    /* constellation to dibit pair mapping
    */
    static const uint8_t NEXT_WORDS[4][4] = {
        { 0x2, 0xc, 0x1, 0xf },
        { 0xe, 0x0, 0xd, 0x3 },
        { 0x9, 0x7, 0xa, 0x4 },
        { 0x5, 0xb, 0x6, 0x8 }
    };

   /* bit counts
    */
   static const uint8_t BIT_COUNT[] = {
      0, 1, 1, 2,
      1, 2, 2, 3,
      1, 2, 2, 3,
      2, 3, 3, 4
   };

   // perform trellis decoding
   in_sz--;
   for(i = 0; i < in_sz; ++i) {
      uint8_t codeword = (in[i] & 0x0f);
      // find dibit with minimum Hamming distance
      uint8_t m = 0;
      uint8_t ns = UINT8_MAX;
      uint8_t hd = UINT8_MAX;
      for(j = 0; j < 4; j++) {
         uint8_t n;
         n = BIT_COUNT[codeword ^ NEXT_WORDS[state][j]];
         if(n < hd) {
            m = 1;
            hd = n;
            ns = j;
         } else if(n == hd) {
            ++m;
         }
      }
      if(m != 1) {
        return i;
      }
      state = ns;
      out[i] = state;
   }
   return 0;
}

/* Symbol interleaving, derived from CAI specification table 7.4
 */
static const size_t INTERLEAVING[] = {
    0, 13, 25, 37, 
    1, 14, 26, 38, 
    2, 15, 27, 39, 
    3, 16, 28, 40, 
    4, 17, 29, 41, 
    5, 18, 30, 42, 
    6, 19, 31, 43, 
    7, 20, 32, 44, 
    8, 21, 33, 45, 
    9, 22, 34, 46, 
   10, 23, 35, 47, 
   11, 24, 36, 48, 
   12 
};

static void
processTSBK(unsigned char raw_dibits[98], unsigned char out[12], int *status_count)
{
  unsigned char trellis_buffer[49];
  unsigned int i, opcode = 0, err = 0;

  for (i = 0; i < 49; i++) {
    unsigned int k = INTERLEAVING[i];
    unsigned char t = ((raw_dibits[2*k] << 2) | (raw_dibits[2*k+1] << 0));
    trellis_buffer[i] = t;
  }

  for (i = 0; i < 49; i++) {
    raw_dibits[49] = 0;
  }

  err = trellis_1_2_decode(trellis_buffer, 49, raw_dibits); /* raw_dibits actually has decoded dibits here! */

  for(i = 0; i < 12; ++i) {
    out[i] = ((raw_dibits[4*i] << 6) | (raw_dibits[4*i+1] << 4) | (raw_dibits[4*i+2] << 2) | (raw_dibits[4*i+3] << 0));
  }

  opcode = (out[0] & 0x3f);
  if (err) {
    printf("TSBK: mfid: 0x%02x, lb: %u, opcode: 0x%02x, err: trellis decode failed, offset: %u\n", out[1], (out[0] >> 7), opcode, err);
  } else {
    printf("TSBK: mfid: 0x%02x, lb: %u, opcode: 0x%02x\n", out[1], (out[0] >> 7), opcode);
  }
}

static void
processTSDU(dsd_opts* opts, dsd_state* state)
{
  unsigned char last_block = 0;
  unsigned char raw_dibits[98];
  unsigned char out[12];
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the next status symbol comes in 14 dibits from here
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  while (!last_block) {
    read_dibit(opts, state, raw_dibits, 98, &status_count);
    processTSBK(raw_dibits, out, &status_count);
    last_block = (out[0] >> 7);
  }

  // trailing status symbol
  getDibit (opts, state);
}

void process_p25_frame(dsd_opts *opts, dsd_state *state, char *tmpStr, unsigned int tmpLen)
{
  unsigned char duid = state->duid;
  unsigned int mfid = 0, errs2 = 0;

  if ((duid == 5) || ((duid == 10) && (state->lastp25type == 1))) {
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          state->errs2 = 0;
          openMbeOutFile (opts, state);
      }
  }

  if (duid == 0) {
      // Header Data Unit
      state->lastp25type = 2;
      mfid = processHDU (opts, state);
      snprintf(tmpStr, 1023, "HDU: mfid: %s (%u), talkgroup: %u",
               mfids[mfid_mapping[(mfid>>2)&0x3f]], mfid, state->talkgroup);
  } else if (duid == 5) { // 11 -> 0101 = 5
      // Logical Link Data Unit 1
      state->lastp25type = 1;
      processLDU1 (opts, state, tmpStr, 1023);
  } else if (duid == 10) { // 22 -> 1010 = 10
      // Logical Link Data Unit 2
      if (state->lastp25type != 1) {
          snprintf(tmpStr, 1023, "Ignoring LDU2 not preceeded by LDU1");
          state->lastp25type = 0;
      } else {
          state->lastp25type = 2;
          errs2 = processLDU2 (opts, state);
          snprintf(tmpStr, 1023, "LDU2: e: %u", errs2);
      }
  } else if ((duid == 3) || (duid == 15)) {
      if (opts->mbe_out_dir[0] != 0) {
          closeMbeOutFile (opts, state);
          state->errs2 = 0;
      }
      mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);
      state->talkgroup = 0;
      state->lastp25type = 0;
      if (duid == 3) {
        // Terminator without subsequent Link Control
        processTDU (opts, state);
        snprintf(tmpStr, 1023, "TDU");
      } else {
        // Terminator with subsequent Link Control
        unsigned char mfid = processTDULC (opts, state);
        snprintf(tmpStr, 1023, "TDULC: mfid: %s (%u)", mfids[mfid_mapping[mfid&0x3f]], mfid);
      }
  } else if (duid == 7) { // 13 -> 0111 = 7
      state->talkgroup = 0;
      state->lastp25type = 3;
      processTSDU (opts, state);
      snprintf(tmpStr, 1023, "TSDU");
  // try to guess based on previous frame if unknown type
  } else if (state->lastp25type == 1) { 
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      //state->lastp25type = 0;
      // Guess that the state is LDU2
      state->lastp25type = 2;
      errs2 = processLDU2 (opts, state);
      snprintf(tmpStr, 1023, "(LDU2): e: %u", errs2);
  } else if (state->lastp25type == 2) {
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      //state->lastp25type = 0;
      // Guess that the state is LDU1
      state->lastp25type = 1;
      //printf("(LDU1)\n");
      processLDU1 (opts, state, tmpStr, 1023);
  } else if (state->lastp25type == 3) {
      //state->lastp25type = 0;
      // Guess that the state is TSDU
      state->lastp25type = 3;
      processTSDU (opts, state);
      //skipDibit (opts, state, 328-25);
      snprintf(tmpStr, 1023, "(TSDU)");
  } else { 
      state->lastp25type = 0;
      snprintf(tmpStr, 1023, "Unknown DUID");
  }
}

