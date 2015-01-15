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

static unsigned int p25_hamming15_11_3_decode(unsigned int *codeword)
{
  unsigned int i, errs = 0, block = *codeword, ecc = 0, syndrome;

  for(i = 0; i < 11; i++) {
      if((block & Hamming15113Gen[i]) > 0xf)
          ecc ^= Hamming15113Gen[i];
  }
  syndrome = ecc ^ block;

  if (syndrome > 0) {
      errs++;
      block ^= (1U << (syndrome - 1));
  }

  *codeword = (block >> 4);
  return (errs);
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
  int i, j, errs = 0;
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
      errs += p25_hamming15_11_3_decode(&hin);
      block = hin;
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

    for (i = 0; i < count; i += 2) {
        int dibit;

        if (*status_count == 35) {
            // Status bits now
            // TODO: do something useful with the status bits...
            //int status = getDibit (opts, state);
            getDibit (opts, state);
            *status_count = 1;
        } else {
            (*status_count)++;
        }

        dibit = getDibit(opts, state);
        output[i] = (1 & (dibit >> 1));      // bit 1
        output[i+1] = (1 & dibit);             // bit 0
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

/**
 * Reads an hex word, its parity bits and attempts to error correct it using the Golay23 algorithm.
 */
static void
read_and_correct_hdu_hex_word (dsd_opts* opts, dsd_state* state, unsigned char* hex, int* status_count)
{
  unsigned int corrected_hexword = 0;
  unsigned char hex_and_parity[9];

  // read both the hex word and the Golay(23, 12) parity information
  read_dibit(opts, state, hex_and_parity, 18, status_count);

  // Use the Golay23 FEC to correct it.
  corrected_hexword = correct_hex_word(state, hex_and_parity, 9);

  // codeword is now hopefully fixed
  // put it back into our hex format
  hex[0] = ((corrected_hexword >> 5) & 1);
  hex[1] = ((corrected_hexword >> 4) & 1);
  hex[2] = ((corrected_hexword >> 3) & 1);
  hex[3] = ((corrected_hexword >> 2) & 1);
  hex[4] = ((corrected_hexword >> 1) & 1);
  hex[5] = ((corrected_hexword >> 0) & 1);
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
  //unsigned int irrecoverable_errors = 0;
  int status_count;
  unsigned char hex_data[20][6];    // Data in hex-words (6 bit words). A total of 20 hex words.
  unsigned char hex_parity[16][6];  // Parity of the data, again in hex-word format. A total of 16 parity hex words.
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
  for (i=15; i>=0; i--) {
      read_and_correct_hdu_hex_word (opts, state, hex_parity[i], &status_count);
  }

  // Use the Reed-Solomon algorithm to correct the data. hex_data is modified in place
#ifdef USE_REEDSOLOMON
  irrecoverable_errors = check_and_fix_redsolomon_36_20_17((char*)hex_data, (char*)hex_parity);
  state->debug_header_critical_errors += irrecoverable_errors;
#endif

  // Now put the corrected data on the DSD structures
  mfid = get_uint(hex_data[7], 6);
  mfid <<= 6;
  mfid |= ((hex_data[6][0] << 1) | (hex_data[6][1] << 0));

  // The important algorithm ID. This indicates whether the data is encrypted,
  // and if so what is the encryption algorithm used.
  // A code 0x80 here means that the data is unencrypted.
  algid  = (get_uint(hex_data[6]+2, 4) << 4);
  algid |= (get_uint(hex_data[5], 4) << 0);

  // The encryption key ID
  kid = ((hex_data[5][4] << 1) | (hex_data[5][5] << 0));
  kid <<= 6;
  kid |= get_uint(hex_data[4], 6);
  kid <<= 6;
  kid |= get_uint(hex_data[3], 6);
  kid <<= 2;
  kid |= ((hex_data[2][0] << 1) | (hex_data[2][1] << 0));
  state->p25kid = kid;

  skipDibit (opts, state, 6);
  //status = getDibit (opts, state);
  //TODO: Do something useful with the status bits...

  if ((mfid == 144) || (mfid == 9)) { // FIXME: only one of these is correct.
    talkgroup  = (get_uint(hex_data[1], 6) << 6);
    talkgroup |= get_uint(hex_data[0], 6);
  } else {
    talkgroup  = (get_uint(hex_data[2]+2, 4) << 10);
    talkgroup |= (get_uint(hex_data[1], 6) << 6);
    talkgroup |= get_uint(hex_data[0], 6);
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
    hexdump_p25_packet(hex_data, 8, 12, mi);
    printf ("mi: %s\n", mi);
  }
}

void
read_and_correct_hex_word (dsd_opts* opts, dsd_state* state, unsigned char* hex, int* status_count)
{
  unsigned char parity[4];
  unsigned int i, value = 0, error_count;

  // Read the hex word
  read_dibit(opts, state, hex, 6, status_count);

  // Read the parity
  read_dibit(opts, state, parity, 4, status_count);

  // Use Hamming to error correct the hex word
  // in the bitset 9 is the left-most and 0 is the right-most
  for (i=0; i<6; i++) {
      value <<= 1;
      value |= hex[i];
  }
  for (i=0; i<4; i++) {
      value <<= 1;
      value |= parity[i];
  }
  error_count = p25_hamming15_11_3_decode(&value);
  value &= 0x3f;

  hex[5] = ((value >> 0) & 1);
  hex[4] = ((value >> 1) & 1);
  hex[3] = ((value >> 2) & 1);
  hex[2] = ((value >> 3) & 1);
  hex[1] = ((value >> 4) & 1);
  hex[0] = ((value >> 5) & 1);

  if (error_count > 0) {
      state->debug_header_errors++;
  }
}

static void
processLDU1 (dsd_opts* opts, dsd_state* state)
{
  // extracts IMBE frames from LDU frame
  int i;
  unsigned char mfid = 0, lcformat = 0;
  unsigned char lsd1 = 0, lsd2 = 0;
  char lcinfo[21];
  unsigned char hex_data[12][6];    // Data in hex-words (6 bit words). A total of 12 hex words.
  unsigned char hex_parity[12][6];  // Parity of the data, again in hex-word format. A total of 12 parity hex words.
  int status_count;
  //int irrecoverable_errors;

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
  read_and_correct_hex_word (opts, state, &(hex_data[11][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[10][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 9][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 8][0]), &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  read_and_correct_hex_word (opts, state, &(hex_data[ 7][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 6][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 5][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 4][0]), &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  read_and_correct_hex_word (opts, state, &(hex_data[ 3][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 2][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 1][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 0][0]), &status_count);

  // IMBE 5
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 5
  read_and_correct_hex_word (opts, state, &(hex_parity[11][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[10][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 9][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 8][0]), &status_count);

  // IMBE 6
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 6
  read_and_correct_hex_word (opts, state, &(hex_parity[ 7][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 6][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 5][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 4][0]), &status_count);

  // IMBE 7
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 7
  read_and_correct_hex_word (opts, state, &(hex_parity[ 3][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 2][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 1][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 0][0]), &status_count);

  // IMBE 8
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 8: LSD (low speed data)
  {
    unsigned char lsd[8], cyclic_parity[8];

    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, lsd+i, 2, &status_count);
    }
    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, cyclic_parity+i, 2, &status_count);
    }
    lsd1 = get_uint(lsd, 8);

    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, lsd+i, 2, &status_count);
    }
    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, cyclic_parity+i, 2, &status_count);
    }
    lsd2 = get_uint(lsd, 8);

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

  // Error correct the hex_data using Reed-Solomon hex_parity
#ifdef USE_REEDSOLOMON
  irrecoverable_errors = check_and_fix_reedsolomon_24_12_13((char*)hex_data, (char*)hex_parity);
  if (irrecoverable_errors == 1) {
      state->debug_header_critical_errors++;
  }
#endif

  lcformat  = (get_uint(hex_data[11], 6) << 6);
  lcformat |= ((hex_data[10][0] << 1) | (hex_data[10][1] << 0));
  mfid  = ((get_uint(hex_data[10]+2, 4) << 4) | (get_uint(hex_data[9], 4) << 0));
  hexdump_p25_packet(hex_data, 0, 9, lcinfo);
  lcinfo[18] = '0';
  lcinfo[19] = (((hex_data[9][5] << 1) | (hex_data[9][4] << 0)) + 0x30);
  lcinfo[20] = '\0';
  if (opts->verbose > 0) {
      printf ("mfid: %u, lcformat: 0x%02x, lcinfo: %s\n", mfid, lcformat, lcinfo);
  }
}

static void
processLDU2 (dsd_opts * opts, dsd_state * state)
{
  // extracts IMBE frames from LDU frame
  int i;
  unsigned char algid = 0, lsd1 = 0, lsd2 = 0;
  unsigned short kid = 0;
  char mi[25];
  unsigned char hex_data[16][6];    // Data in hex-words (6 bit words). A total of 16 hex words.
  unsigned char hex_parity[8][6];   // Parity of the data, again in hex-word format. A total of 12 parity hex words.
  int status_count;
  //int irrecoverable_errors = 0;

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
  read_and_correct_hex_word (opts, state, &(hex_data[15][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[14][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[13][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[12][0]), &status_count);

  // IMBE 3
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 3
  read_and_correct_hex_word (opts, state, &(hex_data[11][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[10][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 9][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 8][0]), &status_count);

  // IMBE 4
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 4
  read_and_correct_hex_word (opts, state, &(hex_data[ 7][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 6][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 5][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 4][0]), &status_count);

  // IMBE 5
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 5
  read_and_correct_hex_word (opts, state, &(hex_data[ 3][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 2][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 1][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_data[ 0][0]), &status_count);

  // IMBE 6
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 6
  read_and_correct_hex_word (opts, state, &(hex_parity[ 7][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 6][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 5][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 4][0]), &status_count);

  // IMBE 7
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 7
  read_and_correct_hex_word (opts, state, &(hex_parity[ 3][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 2][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 1][0]), &status_count);
  read_and_correct_hex_word (opts, state, &(hex_parity[ 0][0]), &status_count);

  // IMBE 8
  process_IMBE (opts, state, &status_count);

  // Read data after IMBE 8: LSD (low speed data)
  {
    unsigned char lsd[8], cyclic_parity[8];

    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, lsd+i, 2, &status_count);
    }
    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, cyclic_parity+i, 2, &status_count);
    }
    lsd1 = get_uint(lsd, 8);

    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, lsd+i, 2, &status_count);
    }
    for (i=0; i<=6; i+=2) {
        read_dibit(opts, state, cyclic_parity+i, 2, &status_count);
    }
    lsd2 = get_uint(lsd, 8);

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

  // Error correct the hex_data using Reed-Solomon hex_parity
#ifdef USE_REEDSOLOMON
  irrecoverable_errors = check_and_fix_reedsolomon_24_16_9((char*)hex_data, (char*)hex_parity);
  if (irrecoverable_errors == 1) {
      state->debug_header_critical_errors++;
  }
#endif

  algid  = (get_uint(hex_data[3], 6) << 6);
  algid |= (get_uint(hex_data[2], 2) << 0);

  // The encryption key ID
  kid = get_uint(hex_data[2]+2, 4);
  kid <<= 6;
  kid |= get_uint(hex_data[1], 6);
  kid <<= 6;
  kid |= get_uint(hex_data[0], 6);
  if (opts->p25enc == 1) {
      printf ("algid: 0x%02x kid: 0x%04x\n", algid, kid);
  }

  if (opts->verbose > 2) {
    mi[24] = '\0';
    hexdump_p25_packet(hex_data, 4, 16, mi);
    printf ("mi: %s\n", mi);
  }
}

static void
processTDU (dsd_opts* opts, dsd_state* state)
{
    unsigned char zeroes[29];
    int status_count;

    // we skip the status dibits that occur every 36 symbols
    // the first IMBE frame starts 14 symbols before next status
    // so we start counter at 36-14-1 = 21
    status_count = 21;

    // Next 14 dibits should be zeros
    read_dibit(opts, state, zeroes, 28, &status_count);

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
  unsigned char zeroes[21];
  int status_count;
  //unsigned char dodeca_parity[6][12];  // Reed-Solomon parity of the data. A total of 6 parity 12-bit words.
  //int irrecoverable_errors = 0;

  // we skip the status dibits that occur every 36 symbols
  // the first IMBE frame starts 14 symbols before next status
  // so we start counter at 36-14-1 = 21
  status_count = 21;

  for (j = 0; j < 6; j++) {
      // read both the hex word and the Golay(23, 12) parity information
      read_dibit(opts, state, hex_and_parity, 24, &status_count);

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
      read_dibit(opts, state, hex_and_parity, 24, &status_count);

      // Use the Golay23 FEC to correct it.
      corrected_hexword = correct_hex_word(state, hex_and_parity, 12);

      // codeword is now hopefully fixed
      // put it back into our hex format
#ifdef USE_REEDSOLOMON
      for (i = 0; i < 12; i++) {
          dodeca_parity[5-j][i] = ((corrected_hexword >> (11 - i)) & 1);
      }
#endif
  }

  // Swap the two 6-bit words to accommodate for the expected word order of the Reed-Solomon decoding
#ifdef USE_REEDSOLOMON
  for(i=0; i<6; i++) {
      swap_hex_words_bits((unsigned char*)dodeca_data + i*12);
      swap_hex_words_bits((unsigned char*)dodeca_parity + i*12);
  }

  // Error correct the hex_data using Reed-Solomon hex_parity
  irrecoverable_errors = check_and_fix_reedsolomon_24_12_13((char*)dodeca_data, (char*)dodeca_parity);

  // Recover the original order
  for(i=0; i<6; i++) {
      swap_hex_words_bits((unsigned char*)dodeca_data + i*12);
  }

  if (irrecoverable_errors == 1) {
      state->debug_header_critical_errors++;
      // We can correct (13-1)/2 = 6 errors. If we failed, it means that there were more than 6 errors in
      // these 12+12 words. But take into account that each hex word was already error corrected with
      // Golay 24, which can correct 3 bits on each sequence of (12+12) bits. We could say that there were
      // 7 errors of 4 bits.
      // update_error_stats(&state->p25_heuristics, 12*6+12*6, 7*4);
  }
#endif

  // Next 10 dibits should be zeros
  read_dibit(opts, state, zeroes, 20, &status_count);

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

