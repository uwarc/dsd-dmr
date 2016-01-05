#include "dsd.h"

/*
 * NXDN SACCH interleave
 */
static const unsigned char nsW[72] = {
  0, 12, 24, 36, 48, 60,
  1, 13, 25, 37, 49, 61,
  2, 14, 26, 38, 50, 62,
  3, 15, 27, 39, 51, 63,
  4, 16, 28, 40, 52, 64,
  5, 17, 29, 41, 53, 65,
  6, 18, 30, 42, 54, 66,
  7, 19, 31, 43, 55, 67,
  8, 20, 32, 44, 56, 68,
  9, 21, 33, 45, 57, 69,
  10,22, 34, 46, 58, 70,
  11,23, 35, 47, 59, 71
};

/*
 *  pseudorandom bit sequence
 */
static const unsigned char nxdnpr[175] = {
    0, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 0, 0, 0, 2, 2,
    0, 2, 2, 2, 2, 0, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0,
    0, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 2, 0, 2, 0,
    2, 2, 0, 2, 0, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0,
    0, 2, 0, 0, 2, 0, 0, 2, 0, 2, 2, 0, 2, 2, 2, 2,
    2, 2, 0, 0, 2, 0, 0, 2, 2, 0, 2, 0, 2, 0, 0, 2,
    2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0,
    0, 2, 2, 0, 0, 2, 0, 2, 0, 0, 0, 2, 2, 0, 2, 0,
    0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0, 0, 2,
    0, 2, 2, 0, 0, 0, 2, 2, 2, 0, 2, 0, 2, 2, 0, 0,
    2, 0, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 2
};

unsigned int
processNXDNVoice (dsd_opts * opts, dsd_state * state)
{
  unsigned int i, j, dibit, total_errs = 0;
  uint64_t l1 = 0;
  unsigned char sacch[72];
  //unsigned int sacch_out;
  unsigned char ambe_dibits[36];
  const unsigned char *pr = nxdnpr;

  for (i = 0; i < 30; i++) {
    dibit = getDibit (opts, state);
    sacch[nsW[2*i]]   = ((*pr++ ^ (dibit & 2)) >> 1); // bit 1
    sacch[nsW[2*i+1]] = ((dibit & 1) >> 0);           // bit 0
  }
  /* For the sake of simplicity, handle the re-insertion of punctured bits here.
   * This makes the actual Viterbi decoder code a LOT simpler.
   */
  for (i = 30; i < 36; i++) {
    sacch[nsW[2*i]]   = 0;
    sacch[nsW[2*i+1]] = 0;
  }
  //sacch_out = nxdn_conv_sacch_decode(sacch);
#if 0
  for(i = 0; i < 60; i++) {
    l1 <<= 1;
    l1 |= sacch[i];
  }
  printf("NXDN: SACCH: 0x%016lx\n", l1); 
#endif 

  for (j = 0; j < 4; j++) {
    for (i = 0; i < 36; i++) { 
      dibit = getDibit (opts, state);
      ambe_dibits[i] = *pr++ ^ dibit;
    }
    processAMBEFrame (opts, state, ambe_dibits);
    total_errs += state->errs2;
  }

  return total_errs;
}

