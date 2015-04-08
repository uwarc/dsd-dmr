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
  unsigned int i, j, l1, l2, dibit, total_errs = 0;
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
  l1 = get_uint(sacch, 30);
  l2 = get_uint(sacch, 30);
  printf("NXDN: SACCH: 0x%08x%08x\n", l1, l2); 
 
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

