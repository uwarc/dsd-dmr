#include "dsd.h"
#include "nxdn_const.h"

void
processNXDNData (dsd_opts * opts, dsd_state * state)
{
  unsigned int i, dibit;
  unsigned char *dibit_p;
  char lich[9];
  char lich_scram[9] = { 0, 0, 1, 0, 0, 1, 1, 1, 0 };
  dibit_p = state->dibit_buf_p - 8;

  for (i = 0; i < 8; i++) {
      dibit = *dibit_p++;
      if(lich_scram[i] ^ (state->lastsynctype & 0x1)) {
          dibit = (dibit ^ 2);
      }
      lich[i] = 1 & (dibit >> 1);
  }

  closeMbeOutFile (opts, state);
  state->errs2 = 0;

  if (opts->errorbars) {
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: GFSK      inlvl: %2i%% %s %s  DATA: %s Ch\n",
              state->ftype, level, state->slot0light, state->slot1light,
              (lich[1] ? (lich[0] ? "Trunk-C Traffic" : "Trunk-D Composite") 
                       : (lich[0] ? "Trunk-C Control" : (lich[6] ? "Repeater" : "Mobile Direct"))));
  }

  skipDibit (opts, state, 174);
}

/*
 *  pseudorandom bit sequence
 */
const char nxdnpr[145] = {
    1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0,
    1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1,
    1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0,
    0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0,
    0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1,
    0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1
};

unsigned int
processNXDNVoice (dsd_opts * opts, dsd_state * state)
{
  int i, j;
  unsigned int dibit, total_errs = 0;
  char ambe_fr[4][24];
  const char *pr;

  skipDibit (opts, state, 30);

  pr = nxdnpr;
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

