#include "dsd.h"
#include "nxdn_const.h"

void
processNXDNData (dsd_opts * opts, dsd_state * state)
{
  int i, dibit;
  int *dibit_p;
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

  if (opts->errorbars) {
      int level = (int) state->max / 164;
      printf ("Sync: %s mod: %s      inlvl: %2i%% %s %s  DATA: ",
              state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"),
              level, state->slot0light, state->slot1light);
      switch((lich[0] << 1) | lich[1]) {
        case 0:
          printf("Trunk-C Control Ch ");
          break;
        case 1:
          printf("Trunk-C Traffic Ch ");
          break;
        case 2:
          if(lich[6])
            printf("Repeater Ch ");
          else
            printf("Mobile Direct Ch ");
          break;
        case 3:
          printf("Trunk-D Composite Ch ");
          break;
      }
      printf("\n");
  }

  skipDibit (opts, state, 174);
}

void
processNXDNVoice (dsd_opts * opts, dsd_state * state)
{
  int i, j, dibit;
  unsigned int total_errs = 0;
  char ambe_fr[4][24];
  const int *w, *x, *y, *z;
  const char *pr;

  skipDibit (opts, state, 30);

  pr = nxdnpr;
  for (j = 0; j < 4; j++) {
    w = nW;
    x = nX;
    y = nY;
    z = nZ;
    for (i = 0; i < 36; i++) { 
      dibit = getDibit (opts, state);
      ambe_fr[*w][*x] = *pr ^ (1 & (dibit >> 1));   // bit 1
      pr++;
      ambe_fr[*y][*z] = (1 & dibit);        // bit 0
      w++;
      x++;
      y++;
      z++;
    }
    processAMBEFrame (opts, state, ambe_fr);
    total_errs += state->errs2;
  }

  if (opts->errorbars) {
    int level = (int) state->max / 164;
    printf ("Sync: %s mod: %s      inlvl: %2i%% %s %s  VOICE e: %u\n",
            state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"),
            level, state->slot0light, state->slot1light, total_errs);
  }
}

