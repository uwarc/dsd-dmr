#include "dsd.h"
#include "nxdn_const.h"

/*
 *  pseudorandom bit sequence
 */
const char nxdnpr[175] = {
    0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1,
    0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0,
    0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0,
    0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1,
    0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0,
    1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1
};

unsigned int
processNXDNVoice (dsd_opts * opts, dsd_state * state)
{
  unsigned int i, j, dibit, total_errs = 0;
  uint64_t l1 = 0;
  unsigned char sacch[60];
  //unsigned char sacch_out[20];
  char ambe_fr[4][24];
  const char *pr;
  pr = nxdnpr;

  for (i = 0; i < 30; i++) {
    dibit = getDibit (opts, state);
    sacch[nsW[i]] = *pr ^ (1 & (dibit >> 1)); // bit 1
    sacch[nsY[i]] = (1 & dibit);        // bit 0
    pr++;
  }
  //nxdn_conv_sacch_decode(sacch, sacch_out);
  //l = get_uint(sacch_out, 20);
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
      ambe_fr[nW[i]][nX[i]] = *pr++ ^ (1 & (dibit >> 1));   // bit 1
      ambe_fr[nY[i]][nZ[i]] = (1 & dibit);        // bit 0
    }
    processAMBEFrame (opts, state, ambe_fr);
    total_errs += state->errs2;
  }

  return total_errs;
}

