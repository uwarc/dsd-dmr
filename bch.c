#include <stdint.h>
#include <stdio.h>

/*
 * Copyright 2010, KA1RBI 
 */
static const unsigned char bchGFexp[64] = {
	1, 2, 4, 8, 16, 32, 3, 6, 12, 24, 48, 35, 5, 10, 20, 40,
	19, 38, 15, 30, 60, 59, 53, 41, 17, 34, 7, 14, 28, 56, 51, 37,
	9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
	13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33, 0
};

static const signed char bchGFlog[64] = {
	-1, 0, 1, 6, 2, 12, 7, 26, 3, 32, 13, 35, 8, 48, 27, 18,
	4, 24, 33, 16, 14, 52, 36, 54, 9, 45, 49, 38, 28, 41, 19, 56,
	5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
	10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58
};

int bchDec(uint64_t cw, uint16_t *cw_decoded)
{
  unsigned char Codeword[63];
  uint16_t cw_out = 0;
  int elp[24][ 22], S[23];
  int D[23], L[24], uLu[24];
  int root[11], locn[11], reg[12];
  int i,j,U,q,count;
  unsigned char SynError = 0;
  int CantDecode = 0;

  for (i = 0; i < 63; i++) {
      Codeword[i] = ((cw >> (62-i)) & 1);
      //Codeword[i] = ((cw >> i) & 1);
  }

  for(i = 1; i <= 22; i++) {
      S[i] = 0;
      // FOR j = 0 TO 62
      for(j = 0; j < 63; j++) {
         if(Codeword[j]) {
            S[i] ^= bchGFexp[(i * j) % 63];
         }
      }
      if(S[i]) {
         SynError = 1;
      }
      S[i] = bchGFlog[S[i]];
      // printf("S[%d] %d\n", i, S[i]);
  }

  if(SynError) { //if there are errors, try to correct them
      L[0] = 0; uLu[0] = -1; D[0] = 0;    elp[0][ 0] = 0;
      L[1] = 0; uLu[1] = 0;  D[1] = S[1]; elp[1][ 0] = 1;
      //FOR i = 1 TO 21
      for(i = 1; i <= 21; i++) {
         elp[0][ i] = -1; elp[1][ i] = 0;
      }
      U = 0;

      do {
         U = U + 1;
         if( D[U] == -1) {
            L[U + 1] = L[U];
            // FOR i = 0 TO L[U]
            for(i = 0; i <= L[U]; i++) {
               elp[U + 1][ i] = elp[U][ i]; elp[U][ i] = bchGFlog[elp[U][ i]];
            }
         } else {
            //search for words with greatest uLu(q) for which d(q)!=0
            q = U - 1;
            while((D[q] == -1) &&(q > 0)) { q = q - 1; }
            //have found first non-zero d(q)
            if(q > 0) {
               j = q;
               do {
                  j = j - 1; if((D[j] != -1) &&(uLu[q] < uLu[j])) { q = j; }
               } while( j > 0);
            }

            //store degree of new elp polynomial
            if(L[U] > L[q] + U - q) {
               L[U + 1] = L[U];
            } else {
               L[U + 1] = L[q] + U - q;
            }

            ///* form new elp(x) */
            // FOR i = 0 TO 21
            for(i = 0; i <= 21; i++) {
               elp[U + 1][ i] = 0;
            }
            // FOR i = 0 TO L(q)
            for(i = 0; i <= L[q]; i++) {
               if( elp[q][ i] != -1) {
                  elp[U + 1][ i + U - q] = bchGFexp[(D[U] + 63 - D[q] + elp[q][ i]) % 63];
               }
            }
            // FOR i = 0 TO L(U)
            for(i = 0; i <= L[U]; i++) {
               elp[U + 1][ i] = elp[U + 1][ i] ^ elp[U][ i];
               elp[U][ i] = bchGFlog[elp[U][ i]];
            }
         }
         uLu[U + 1] = U - L[U + 1];

         //form(u+1)th discrepancy
         if(U < 22) {
            //no discrepancy computed on last iteration
            if( S[U + 1] != -1) { D[U + 1] = bchGFexp[S[U + 1]]; } else { D[U + 1] = 0; }
            // FOR i = 1 TO L(U + 1)
            for(i = 1; i <= L[U + 1]; i++) {
               if((S[U + 1 - i] != -1) &&(elp[U + 1][ i] != 0)) {
                  D[U + 1] = D[U + 1] ^ bchGFexp[(S[U + 1 - i] + bchGFlog[elp[U + 1][ i]]) % 63];
               }
            }
            //put d(u+1) into index form */
            D[U + 1] = bchGFlog[D[U + 1]];
         }
      } while((U < 22) &&(L[U + 1] <= 11));

      U = U + 1;
      if(L[U] <= 11) { // /* Can correct errors */
         //put elp into index form
         // FOR i = 0 TO L[U]
         for(i = 0; i <= L[U]; i++) {
            elp[U][ i] = bchGFlog[elp[U][ i]];
         }

         //Chien search: find roots of the error location polynomial
         // FOR i = 1 TO L(U)
         for(i = 1; i <= L[U]; i++) {
            reg[i] = elp[U][ i];
         }
         count = 0;
         // FOR i = 1 TO 63
         for(i = 1; i <= 63; i++) {
            q = 1;
            //FOR j = 1 TO L(U)
            for(j = 1; j <= L[U]; j++) {
               if( reg[j] != -1) {
                  reg[j] =(reg[j] + j) % 63; q = q ^ bchGFexp[reg[j]];
               }
            }
            if(q == 0) { //store root and error location number indices
               root[count] = i; locn[count] = 63 - i; count = count + 1;
            }
         }
         if(count == L[U]) {
            //no. roots = degree of elp hence <= t errors
            //FOR i = 0 TO L[U] - 1
            for(i = 0; i <= L[U]-1; i++) {
               Codeword[locn[i]] = Codeword[locn[i]] ^ 1;
            }
            CantDecode = count;
         } else { //elp has degree >t hence cannot solve
            CantDecode = -1;
         }
      } else {
         CantDecode = -2;
      }
  }

  for (i = 0; i < 16; i++) {
    cw_out <<= 1;
    cw_out |= Codeword[i];
  }
  *cw_decoded = cw_out;
  return CantDecode;
}

static uint64_t bchEncMatrix[16] = {
   0x8000cd930bdd3b2a,
   0x4000ab5a8e33a6be,
   0x2000983e4cc4e874,
   0x10004c1f2662743a,
   0x0800eb9c98ec0136,
   0x0400b85d47ab3bb0,
   0x02005c2ea3d59dd8,
   0x01002e1751eaceec,
   0x0080170ba8f56776,
   0x0040c616dfa78890,
   0x0020630b6fd3c448,
   0x00103185b7e9e224,
   0x000818c2dbf4f112,
   0x0004c1f2662743a2,
   0x0002ad6a38ce9afb,
   0x00019b2617ba7657
};

uint64_t bchEnc(uint16_t in_word)
{
  uint8_t i;
  uint16_t temp_word;
  uint64_t code_word_out = 0;
  for(i=0; i<16; i++) {
    temp_word = (in_word & (1U << (15 - i)));
    if(temp_word >= 1) {
      code_word_out ^= bchEncMatrix[i];
    }
  }
  return code_word_out;
}

