#ifndef __REEDSOLOMON_H__
#define __REEDSOLOMON_H__

#define MAX_NN 255
#define MAX_TT 8

typedef struct _ReedSolomon {
    int alpha_to[MAX_NN + 1];
    int index_of[MAX_NN + 1];
    int gg[17];
    /* RS code over GF(2**mm) */
    /* tt -> number of errors that can be corrected */
    unsigned char mm, tt;
    /* nn=2**mm-1 -> length of codeword */
    /* kk=nn-2*tt */
    /* distance = nn-kk+1 = 2*tt+1 */
    unsigned int nn, kk, n;
} ReedSolomon;

void rs_init(ReedSolomon *rs, unsigned char *generator_polinomial, unsigned char mm, unsigned char tt);
void rs_encode(ReedSolomon *rs, const unsigned char *data, unsigned char *bb);
int rs_decode(ReedSolomon *rs, unsigned char *input);

#endif
