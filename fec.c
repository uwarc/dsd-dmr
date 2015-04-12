#include <stdint.h>
#include "fec.h"
#include "Golay.h"

static unsigned int GolayGenerator[12] = {
  0x63a, 0x31d, 0x7b4, 0x3da, 0x1ed, 0x6cc, 0x366, 0x1b3, 0x6e3, 0x54b, 0x49f, 0x475
};

static unsigned int Hamming15113Gen[11] = {
    0x4009, 0x200d, 0x100f, 0x080e, 0x0407, 0x020a, 0x0105, 0x008b, 0x004c, 0x0026, 0x0013
};

static unsigned int Hamming15113Table[16] = {
    0x0000, 0x0001, 0x0002, 0x0013, 0x0004, 0x0105, 0x0026, 0x0407,
    0x0008, 0x4009, 0x020A, 0x008b, 0x004C, 0x200D, 0x080E, 0x100F
};

static unsigned int p25_Hamming1064Gen[6] = {
    0x20e, 0x10d, 0x08b, 0x047, 0x023, 0x01c
};

static unsigned int p25_Hamming15113Gen[11] = {
    0x400f, 0x200e, 0x100d, 0x080c, 0x040b, 0x020a, 0x0109, 0x0087, 0x0046, 0x0025, 0x0013
};

static unsigned int Cyclic1685Gen[8] = {
    0x804e, 0x4027, 0x208f, 0x10db, 0x08f1, 0x04e4, 0x0272, 0x0139
};

static unsigned char Hamming7_4_enctab[16] =
{
    0x00, 0x0b, 0x16, 0x1d, 0x27, 0x2c, 0x31, 0x3a,
    0x45, 0x4e, 0x53, 0x58, 0x62, 0x69, 0x74, 0x7f
};

static unsigned char Hamming7_4_dectab[64] =
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

unsigned char Hamming7_4_Correct(unsigned char value)
{
    unsigned char c = Hamming7_4_dectab[value >> 1];
    if (value & 1) {
        c &= 0x0F;
    } else {
        c >>= 4;
    }
    return c;
}

unsigned char Hamming7_4_Encode(unsigned char value)
{
    return Hamming7_4_enctab[value & 0x0f];
}

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

unsigned int Hamming15_11_3_Encode(unsigned int input)
{
    unsigned int i, codeword_out = 0;
    for(i = 0; i < 11; ++i) {
        if(input & (1 << (10 - i))) {
            codeword_out ^= Hamming15113Gen[i];
        }
    }
    return codeword_out;
}

void p25_Hamming10_6_4_Correct(unsigned int *codeword)
{
  unsigned int i, block = *codeword, ecc = 0, syndrome;

  for(i = 0; i < 6; i++) {
      if((block & p25_Hamming1064Gen[i]) > 0xf)
          ecc ^= p25_Hamming1064Gen[i];
  }
  syndrome = ecc ^ block;

  if (syndrome > 0) {
      block ^= (1U << (syndrome - 1));
  }

  *codeword = (block >> 4);
}

unsigned int p25_Hamming10_6_4_Encode(unsigned int input)
{
    unsigned int i, codeword_out = 0;
    for(i = 0; i < 6; ++i) {
        if(input & (1 << (5 - i))) {
            codeword_out ^= p25_Hamming1064Gen[i];
        }
    }
    return codeword_out;
}

void p25_Hamming15_11_3_Correct(unsigned int *codeword)
{
  unsigned int i, block = *codeword, ecc = 0, syndrome;

  for(i = 0; i < 11; i++) {
      if((block & p25_Hamming15113Gen[i]) > 0xf)
          ecc ^= p25_Hamming15113Gen[i];
  }
  syndrome = ecc ^ block;

  if (syndrome > 0) {
      block ^= (1U << (syndrome - 1));
  }

  *codeword = (block >> 4);
}

unsigned int p25_Hamming15_11_3_Encode(unsigned int input)
{
    unsigned int i, codeword_out = 0;
    for(i = 0; i < 11; ++i) {
        if(input & (1 << (10 - i))) {
            codeword_out ^= p25_Hamming15113Gen[i];
        }
    }
    return codeword_out;
}

void p25_lsd_cyclic1685_Correct(unsigned int *codeword)
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

unsigned int p25_lsd_cyclic1685_Encode(unsigned int input)
{
    unsigned int i, codeword_out = 0;
    for(i = 0; i < 8; i++) {
        if (input & (1 << (7 - i))) {
            codeword_out ^= Cyclic1685Gen[i];
        }
    }
    return codeword_out;
}

/* Trellis encoder state transitions, composed with constellation to dibit pair mappings.
 * \see Table 7-2/7-3 FDMA CAI Specification.
 */
static const uint8_t p25_trellis_1_2_next_words[4][4] = {
    { 0x2, 0xc, 0x1, 0xf },
    { 0xe, 0x0, 0xd, 0x3 },
    { 0x9, 0x7, 0xa, 0x4 },
    { 0x5, 0xb, 0x6, 0x8 }
};

void p25_trellis_1_2_encode(uint8_t *data_in, unsigned int data_len, uint8_t *out)
{
    unsigned int i;
    uint8_t state = 0;

   // perform trellis encoding
   for(i = 0; i < data_len; ++i) {
      uint8_t d = (data_in[i] & 0x03);
      out[i] = p25_trellis_1_2_next_words[state][d];
      state = d;
   }
   out[data_len] = p25_trellis_1_2_next_words[state][0];
}

unsigned int p25_trellis_1_2_decode(uint8_t *in, uint32_t in_sz, uint8_t *out)
{
   uint8_t state = 0;
   unsigned int i, j;

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
         n = BIT_COUNT[codeword ^ p25_trellis_1_2_next_words[state][j]];
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

void Golay23_Correct(unsigned int *block)
{
  unsigned int i, syndrome = 0;
  unsigned int mask, block_l = *block;

  mask = 0x400000l;
  for (i = 0; i < 12; i++) {
      if ((block_l & mask) != 0) {
          syndrome ^= GolayGenerator[i];
      }
      mask = mask >> 1;
  }

  syndrome ^= (block_l & 0x07ff);
  *block = ((block_l >> 11) ^ GolayMatrix[syndrome]);
}

/* This function calculates [23,12] Golay codewords.
   The format of the returned int is [checkbits(11),data(12)]. */
unsigned int Golay23_Encode(unsigned int cw)
{
  unsigned int i, c;
  cw&=0xfffl;
  c=cw; /* save original codeword */
  for (i=0; i<12; i++){ /* examine each data bit */
      if (cw & 1)        /* test data bit */
        cw^=0xC75;        /* XOR polynomial */
      cw>>=1;            /* shift intermediate result */
  }
  return((cw<<12)|c);    /* assemble codeword */
}

#if 0
#define MM 6
#include "ReedSolomon.c"
#undef MM
#define MM 8
#include "ReedSolomon.c"
#endif

