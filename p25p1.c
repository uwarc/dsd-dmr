#include "dsd.h"
#include "p25p1_const.h"

static void hexdump_p25_packet(unsigned char payload[20][6], unsigned int payload_offset, unsigned int payload_len,
                               char *packetdump)
{
    static const char hex[]="0123456789abcdef";
    unsigned int i, j = 0, l;
    for (i = 0; i < payload_len; i++) {
        l = get_uint(payload[i+payload_offset], 6);
        packetdump[j++] = hex[((l >> 4) & 0x0f)];
        packetdump[j++] = hex[((l >> 0) & 0x0f)];
        packetdump[j++] = ' ';
    }
    packetdump[j] = '\0';
}

static unsigned int Hamming15113Gen[11] = {
    0x400f, 0x200e, 0x100d, 0x080c, 0x040b, 0x020a, 0x0109, 0x0087, 0x0046, 0x0025, 0x0013
};

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

static unsigned int mbe_eccImbe7200x4400C0 (char imbe_fr[23])
{
  int j;
  unsigned int errs, gin = 0, gout;

  for (j = 22; j >= 0; j--) {
      gin <<= 1;
      gin |= imbe_fr[j];
  }

  errs = mbe_golay2312 (gin, &gout);
  for (j = 11; j >= 0; j--) {
      imbe_fr[j+11] = (gout & 0x0800) >> 11;
      gout <<= 1;
  }

  return (errs);
}

static void mbe_demodulateImbe7200x4400Data (char imbe[8][23])
{
  int i, j = 0, k;
  unsigned short pr[115], foo = 0;

  // create pseudo-random modulator
  for (i = 0; i < 12; i++) {
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

  for (i = 0; i < 4; i++) {
      if (i > 0) {
          gin = 0;
          for (j = 22; j >= 0; j++) {
              gin <<= 1;
              gin |= imbe_fr[i][j];
          }
          errs += mbe_golay2312 (gin, &gout);
          for (j = 0; j < 12; j++) {
            *imbe++ = ((gout >> j) & 1);
          }
      } else {
          for (j = 11; j >= 0; j--) {
              *imbe++ = imbe_fr[i][j+11];
          }
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
  int j, dibit;
  char imbe_fr[8][23];
  const int *w, *x, *y, *z;

  w = iW;
  x = iX;
  y = iY;
  z = iZ;

  for (j = 0; j < 72; j++)
    {
      if (*status_count == 35)
        {
          // Skip the status symbol
          //int status = getDibit (opts, state);
          getDibit (opts, state);
          // TODO: do something useful with the status bits...
          *status_count = 1;
        }
      else
        {
          (*status_count)++;
        }
      dibit = getDibit (opts, state);
      imbe_fr[*w][*x] = (1 & (dibit >> 1)); // bit 1
      imbe_fr[*y][*z] = (1 & dibit);        // bit 0
      w++;
      x++;
      y++;
      z++;
    }

  //if (state->p25kid == 0 || opts->unmute_encrypted_p25 == 1)
    {
      {
          // Check for a non-standard c0 transmitted
          // This is explained here: https://github.com/szechyjs/dsd/issues/24
          char non_standard_word[23] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0};
          int match = 1;
          unsigned int i;
          for (i=0; i<23; i++) {
              if (imbe_fr[0][i] != non_standard_word[i]) {
                  match = 0;
                  break;
              }
          }

          if (match) {
              // Skip this particular value. If we let it pass it will be signaled as an erroneus IMBE
              printf("(Non-standard IMBE c0 detected, skipped)");
          } else {
              char imbe_d[88];
              for (i = 0; i < 88; i++) {
                imbe_d[i] = 0;
              }

              state->errs2 = mbe_eccImbe7200x4400C0 (imbe_fr[0]);
              mbe_demodulateImbe7200x4400Data (imbe_fr);
              state->errs2 += mbe_eccImbe7200x4400Data (imbe_fr, imbe_d);
              processIMBEFrame (opts, state, imbe_d);
          }
      }
    }
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

static inline void hex_to_bin(unsigned char *output, unsigned int input)
{
  output[0] = ((input >> 5) & 1);
  output[1] = ((input >> 4) & 1);
  output[2] = ((input >> 3) & 1);
  output[3] = ((input >> 2) & 1);
  output[4] = ((input >> 1) & 1);
  output[5] = ((input >> 0) & 1);
}

/**
 * Reads an hex word, its parity bits and attempts to error correct it using the Golay23 algorithm.
 */
static void
read_and_correct_hdu_hex_word (dsd_opts* opts, dsd_state* state, unsigned char* hex, int* status_count)
{
  unsigned int corrected_hexword = 0;
  unsigned char hex_and_parity[18];

  // read both the hex word and the Golay(23, 12) parity information
  read_dibit(opts, state, hex_and_parity, 9, status_count);

  // Use the Golay23 FEC to correct it.
  corrected_hexword = correct_hex_word(state, hex_and_parity, 9);

  // codeword is now hopefully fixed
  // put it back into our hex format
  hex_to_bin(hex, corrected_hexword);
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
static void
processHDU(dsd_opts* opts, dsd_state* state)
{
  int i;
  unsigned char mfid = 0;
  unsigned char algid = 0;
  unsigned short kid = 0;
  unsigned int talkgroup = 0;
  int status_count;
  unsigned char hex_data[20][6];    // Data in hex-words (6 bit words). A total of 20 hex words.
  char mi[25];

  // we skip the status dibits that occur every 36 symbols
  // the next status symbol comes in 14 dibits from here
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  // Read 20 hex words, correct them using their Golay23 parity data.
  for (i=19; i>=0; i--) {
      read_and_correct_hdu_hex_word (opts, state, hex_data[i], &status_count);
  }

  // Read the 16 parity hex word. These are used to FEC the 20 hex words using Reed-Solomon.
  skip_dibit(opts, state, 9*16, &status_count);

  // Now put the corrected data on the DSD structures
  mfid = get_uint(hex_data[12], 6);
  mfid <<= 6;
  mfid |= ((hex_data[13][0] << 1) | (hex_data[13][1] << 0));

  // The important algorithm ID. This indicates whether the data is encrypted,
  // and if so what is the encryption algorithm used.
  // A code 0x80 here means that the data is unencrypted.
  algid  = (get_uint(hex_data[13]+2, 4) << 4);
  algid |= (get_uint(hex_data[14], 4) << 0);

  // The encryption key ID
  kid = ((hex_data[14][4] << 1) | (hex_data[14][5] << 0));
  kid <<= 6;
  kid |= get_uint(hex_data[15], 6);
  kid <<= 6;
  kid |= get_uint(hex_data[16], 6);
  kid <<= 2;
  kid |= ((hex_data[17][0] << 1) | (hex_data[17][1] << 0));
  state->p25kid = kid;

  skipDibit (opts, state, 6);
  //status = getDibit (opts, state);
  //TODO: Do something useful with the status bits...

  if ((mfid == 144) || (mfid == 9)) { // FIXME: only one of these is correct.
    talkgroup  = (get_uint(hex_data[18], 6) << 6);
    talkgroup |= get_uint(hex_data[19], 6);
  } else {
    talkgroup  = (get_uint(hex_data[17]+2, 4) << 10);
    talkgroup |= (get_uint(hex_data[18], 6) << 6);
    talkgroup |= get_uint(hex_data[19], 6);
  }

  state->talkgroup = talkgroup;
  state->lasttg = talkgroup;
  if (opts->verbose > 1) {
      printf ("mfid: %u talkgroup: %u ", mfid, talkgroup);
      if (opts->p25enc == 1) {
          printf ("algid: 0x%02x kid: 0x%04x ", algid, kid);
      }
      printf ("\n");
  }

  if (opts->verbose > 2) {
    mi[24] = '\0';
    hexdump_p25_packet(hex_data, 0, 12, mi);
    printf ("mi: %s\n", mi);
  }
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
  p25_hamming15_11_3_decode(&value);
  value &= 0x3f; value_in[0] >>= 4;
  if (value != value_in[0]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[1];
  p25_hamming15_11_3_decode(&value);
  value &= 0x3f; value_in[1] >>= 4;
  if (value != value_in[1]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[2];
  p25_hamming15_11_3_decode(&value);
  value &= 0x3f; value_in[2] >>= 4;
  if (value != value_in[2]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  value = value_in[3];
  p25_hamming15_11_3_decode(&value);
  value &= 0x3f; value_in[3] >>= 4;
  if (value != value_in[3]) { state->debug_header_errors++; }
  value_out <<= 6;
  value_out |= value;

  return value_out;
}

static void
processLDU1 (dsd_opts* opts, dsd_state* state)
{
  // extracts IMBE frames from LDU frame
  int i;
  unsigned char mfid = 0, lcformat = 0;
  unsigned char lsd1 = 0, lsd2 = 0;
  //unsigned char hex_data[12][6];    // Data in hex-words (6 bit words). A total of 12 hex words.
  unsigned char hex_data[3];    // Data in hex-words (6 bit words), stored packed in groups of four, in a uint32_t
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  if (opts->errorbars == 1) {
      printf ("e:");
  }

  // IMBE 1
  process_IMBE (opts, state, &status_count);

  // IMBE 2
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 2
  hex_data[0] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  hex_data[1] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  hex_data[2] = read_and_correct_hex_word (opts, state, &status_count);

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
  {
    unsigned char lsd[8];

    read_dibit(opts, state, lsd, 4, &status_count);
    skip_dibit(opts, state, 4, &status_count);
    read_dibit(opts, state, lsd+4, 4, &status_count);
    skip_dibit(opts, state, 4, &status_count);

    for (i=0; i<4; i++) {
      lsd1 <<= 2;
      lsd2 <<= 2;
      lsd1 |= lsd[i];
      lsd2 |= lsd[i+4];
    }

    // TODO: error correction of the LSD bytes...
    // TODO: do something useful with the LSD bytes...
  }

  // IMBE 9
  process_IMBE (opts, state, &status_count);

  if (opts->errorbars == 1) {
      printf ("\n");
  }

  if (opts->p25status == 1) {
      printf ("lsd1: 0x%02x, lsd2: 0x%02x ", lsd1, lsd2);
  }

  // trailing status symbol
  {
      //int status = getDibit (opts, state);
      getDibit (opts, state);
      // TODO: do something useful with the status bits...
  }

  lcformat  = (hex_data[0] & 0xFF);
  mfid  = ((hex_data[0] >> 8) & 0xFF);
  if (opts->verbose > 0) {
      printf ("mfid: %u, lcformat: 0x%02x, lcinfo: 0x%02x 0x%06x 0x%06x\n", mfid, lcformat, (hex_data[0] >> 16), hex_data[1], hex_data[2]);
  }
}

static void
processLDU2 (dsd_opts * opts, dsd_state * state)
{
  // extracts IMBE frames from LDU frame
  int i;
  unsigned char algid = 0, lsd1 = 0, lsd2 = 0;
  unsigned short kid = 0;
  //unsigned char hex_data[16][6];    // Data in hex-words (6 bit words). A total of 16 hex words.
  unsigned char hex_data[4];    // Data in hex-words (6 bit words). A total of 16 hex words.
  int status_count;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  if (opts->errorbars == 1) {
      printf ("e:");
  }

  // IMBE 1
  process_IMBE (opts, state, &status_count);

  // IMBE 2
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 2
  hex_data[0] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  hex_data[1] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  hex_data[2] = read_and_correct_hex_word (opts, state, &status_count);

  // IMBE 5
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 5
  hex_data[3] = read_and_correct_hex_word (opts, state, &status_count);

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
  {
    unsigned char lsd[8];

    read_dibit(opts, state, lsd, 4, &status_count);
    skip_dibit(opts, state, 4, &status_count);
    read_dibit(opts, state, lsd+4, 4, &status_count);
    skip_dibit(opts, state, 4, &status_count);

    for (i=0; i<4; i++) {
      lsd1 <<= 2;
      lsd2 <<= 2;
      lsd1 |= lsd[i];
      lsd2 |= lsd[i+4];
    }

    // TODO: error correction of the LSD bytes...
    // TODO: do something useful with the LSD bytes...
  }

  // IMBE 9
  process_IMBE (opts, state, &status_count);

  if (opts->errorbars == 1) {
      printf ("\n");
  }

  if (opts->p25status == 1) {
      printf ("lsd1: 0x%02x, lsd2: 0x%02x ", lsd1, lsd2);
  }

  // trailing status symbol
  {
      //int status = getDibit (opts, state) + '0';
      getDibit (opts, state);
      // TODO: do something useful with the status bits...
  }

  algid  = (hex_data[3] & 0xFF);

  // The encryption key ID
  kid = ((hex_data[3] >> 8) & 0xFFFF);
  if (opts->p25enc == 1) {
      printf ("algid: 0x%02x kid: 0x%04x\n", algid, kid);
  }

  if (opts->verbose > 2) {
    printf ("mi: 0x%04x 0x%04x 0x%04x\n", hex_data[0], hex_data[1], hex_data[2]);
  }
}

static void
processTDU (dsd_opts* opts, dsd_state* state)
{
    int status_count;

    // we skip the status dibits that occur every 36 symbols
    // the first IMBE frame starts 14 symbols before next status
    // so we start counter at 36-14-1 = 21
    status_count = 21;

    // Next 14 dibits should be zeros
    skip_dibit(opts, state, 14, &status_count);

    // Next we should find an status dibit
    if (status_count != 35) {
        printf("*** SYNC ERROR\n");
    }

    // trailing status symbol
    {
        //int status = getDibit (opts, state) + '0';
        getDibit (opts, state);
        // TODO: do something useful with the status bits...
    }
}

/**
 * Reverse the order of bits in a 12-bit word. We need this to accommodate to the expected bit order in
 * some algorithms.
 * \param dodeca The 12-bit word to reverse.
 */
static inline void
swap_hex_words_bits(unsigned char *dodeca)
{
  unsigned int j;
  for(j=0; j<6; j++) {
      unsigned char swap = dodeca[j];
      dodeca[j] = dodeca[j+6];
      dodeca[j+6] = swap;
  }
}

static void
processTDULC (dsd_opts* opts, dsd_state* state)
{
  unsigned int i, j, corrected_hexword = 0;
  unsigned char hex_and_parity[12];
  unsigned char lcformat = 0, mfid = 0;
  char lcinfo[57];
  unsigned char dodeca_data[6][12];    // Data in 12-bit words. A total of 6 words.
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
      for (i = 0; i < 12; i++) {
          dodeca_data[5-j][i] = ((corrected_hexword >> (11 - i)) & 1);
      }
  }

  for (j = 0; j < 6; j++) {
      // read both the hex word and the Golay(23, 12) parity information
      read_dibit(opts, state, hex_and_parity, 12, &status_count);

      // Use the Golay23 FEC to correct it.
      corrected_hexword = correct_hex_word(state, hex_and_parity, 12);
  }

  // Next 10 dibits should be zeros
  skip_dibit(opts, state, 10, &status_count);

  // Next we should find an status dibit
  if (status_count != 35) {
      printf("*** SYNC ERROR\n");
  }

  // trailing status symbol
  {
      //int status = getDibit (opts, state) + '0';
      getDibit (opts, state);
      // TODO: do something useful with the status bits...
  }

  // Put the corrected data into the DSD structures
  lcformat = get_uint(dodeca_data[5], 8);
  mfid  = ((get_uint(dodeca_data[5]+8, 4) << 4) | (get_uint(dodeca_data[4], 4) << 0));

  {
    static const char hex[]="0123456789abcdef";
    unsigned int l = get_uint(dodeca_data[4]+4, 8);
    lcinfo[0] = hex[((l >> 4) & 0x0f)];
    lcinfo[1] = hex[((l >> 0) & 0x0f)];
    lcinfo[2] = ' ';
    l = get_uint(dodeca_data[3], 8);
    lcinfo[3] = hex[((l >> 4) & 0x0f)];
    lcinfo[4] = hex[((l >> 0) & 0x0f)];
    lcinfo[5] = ' ';
    l = get_uint(dodeca_data[3]+8, 4);
    lcinfo[6] = hex[((l >> 0) & 0x0f)];
    l = get_uint(dodeca_data[2], 4);
    lcinfo[7] = hex[((l >> 0) & 0x0f)];
    lcinfo[8] = ' ';
    l = get_uint(dodeca_data[2]+4, 8);
    lcinfo[9] = hex[((l >> 4) & 0x0f)];
    lcinfo[10] = hex[((l >> 0) & 0x0f)];
    lcinfo[11] = ' ';
    l = get_uint(dodeca_data[1], 8);
    lcinfo[12] = hex[((l >> 4) & 0x0f)];
    lcinfo[13] = hex[((l >> 0) & 0x0f)];
    lcinfo[14] = ' ';
    l = get_uint(dodeca_data[1]+8, 4);
    lcinfo[15] = hex[((l >> 0) & 0x0f)];
    l = get_uint(dodeca_data[0], 4);
    lcinfo[16] = hex[((l >> 0) & 0x0f)];
    lcinfo[17] = ' ';
    l = get_uint(dodeca_data[0]+4, 8);
    lcinfo[18] = hex[((l >> 4) & 0x0f)];
    lcinfo[19] = hex[((l >> 0) & 0x0f)];
    lcinfo[20] = '\0';
  }

  if (opts->verbose > 0) {
      printf ("mfid: %u, lcformat: 0x%02x, lcinfo: %s\n", mfid, lcformat, lcinfo);
  }
}

static void print_p25_frameinfo (dsd_opts * opts, dsd_state * state, char *p25frametype)
{
  int level = (int) state->max / 164;
  printf ("inlvl: %2i%% ", level);
  if (state->nac != 0) {
      printf ("nac: %4X ", state->nac);
  }

  if (opts->verbose > 1) {
      printf ("src: %8u ", state->last_radio_id);
  }
  printf ("tg: %5u %s", state->lasttg, p25frametype);
}

void process_p25_frame(dsd_opts *opts, dsd_state *state, unsigned char duid)
{
  if (duid == 0) {
      // Header Data Unit
      print_p25_frameinfo (opts, state, "HDU\n");
      state->lastp25type = 2;
      processHDU (opts, state);
  } else if (duid == 5) { // 11 -> 0101 = 5
      // Logical Link Data Unit 1
      print_p25_frameinfo (opts, state, "LDU1 ");
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      state->lastp25type = 1;
      state->numtdulc = 0;
      processLDU1 (opts, state);
  } else if (duid == 10) { // 22 -> 1010 = 10
      // Logical Link Data Unit 2
      if (state->lastp25type != 1) {
          print_p25_frameinfo (opts, state, "Ignoring LDU2 not preceeded by LDU1\n");
          state->lastp25type = 0;
      } else {
          print_p25_frameinfo (opts, state, "LDU2 ");
          if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
              openMbeOutFile (opts, state);
          }
          state->lastp25type = 2;
          state->numtdulc = 0;
          processLDU2 (opts, state);
      }
  } else if (duid == 15) { // 33 -> 1111 = 15
      // Terminator with subsequent Link Control
      print_p25_frameinfo (opts, state, "TDULC\n");
      if (opts->mbe_out_dir[0] != 0) {
          closeMbeOutFile (opts, state);
      }
      mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);
      state->lasttg = 0;
      state->last_radio_id = 0;
      state->lastp25type = 0;
      state->err_str[0] = 0;
      state->numtdulc++;
      processTDULC (opts, state);
      state->err_str[0] = 0;
  } else if (duid == 3) {
      // Terminator without subsequent Link Control
      print_p25_frameinfo (opts, state, "TDU\n");
      if (opts->mbe_out_dir[0] != 0) {
          closeMbeOutFile (opts, state);
      }
      mbe_initMbeParms (&state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced);
      state->lasttg = 0;
      state->last_radio_id = 0;
      state->lastp25type = 0;
      state->err_str[0] = 0;
      processTDU (opts, state);
  } else if (duid == 7) { // 13 -> 0111 = 7
      print_p25_frameinfo (opts, state, "TSDU\n");
      state->lasttg = 0;
      state->last_radio_id = 0;
      state->lastp25type = 3;
      // Now processing NID
      skipDibit (opts, state, 328-25);
  } else if (duid == 12) { // 30 -> 1100 = 12
      print_p25_frameinfo (opts, state, "PDU\n");
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      state->lastp25type = 4;
  // try to guess based on previous frame if unknown type
  } else if (state->lastp25type == 1) { 
      print_p25_frameinfo (opts, state, "(LDU2) ");
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      //state->lastp25type = 0;
      // Guess that the state is LDU2
      state->lastp25type = 2;
      state->numtdulc = 0;
      processLDU2 (opts, state);
  } else if (state->lastp25type == 2) {
      print_p25_frameinfo (opts, state, "(LDU1) ");
      if ((opts->mbe_out_dir[0] != 0) && (opts->mbe_out_fd == -1)) {
          openMbeOutFile (opts, state);
      }
      //state->lastp25type = 0;
      // Guess that the state is LDU1
      state->lastp25type = 1;
      state->numtdulc = 0;
      processLDU1 (opts, state);
  } else if (state->lastp25type == 3) {
      print_p25_frameinfo (opts, state, "(TSDU)\n");
      //state->lastp25type = 0;
      // Guess that the state is TSDU
      state->lastp25type = 3;
      // Now processing NID
      skipDibit (opts, state, 328-25);
  } else if (state->lastp25type == 4) {
      print_p25_frameinfo (opts, state, "(PDU)\n");
      state->lastp25type = 0;
  } else { 
      state->lastp25type = 0;
      print_p25_frameinfo (opts, state, "Unknown DUID: ");
      printf ("0x%x\n", duid);
  }
}

