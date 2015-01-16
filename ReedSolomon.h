#ifndef __REEDSOLOMON_H__
#define __REEDSOLOMON_H__

/* RS code over GF(2**mm) */
/* nn=2**mm-1 -> length of codeword */
#define MAX_NN 255
#define MAX_TT 8
#define NN ((1<<MM)-1)

typedef struct _ReedSolomon {
    int alpha_to[MAX_NN + 1];
    int index_of[MAX_NN + 1];
    int gg[17];
    /* tt -> number of errors that can be corrected */
    unsigned char tt;
    /* kk=nn-2*tt */
    /* distance = nn-kk+1 = 2*tt+1 */
    unsigned int kk, n;
} ReedSolomon;

void rs8_init(ReedSolomon *rs, unsigned int generator_polinomial, unsigned char tt);
void rs8_encode(ReedSolomon *rs, const unsigned char *data, unsigned char *bb);
int rs8_decode(ReedSolomon *rs, unsigned char *input, unsigned char *output);
void rs6_init(ReedSolomon *rs, unsigned int generator_polinomial, unsigned char tt);
void rs6_encode(ReedSolomon *rs, const unsigned char *data, unsigned char *bb);
int rs6_decode(ReedSolomon *rs, unsigned char *input, unsigned char *output);

#endif
