#include <stdint.h>
#include "ReedSolomon.h"
#include "dsd.h"

// Uncomment for very verbose trace messages
//#define CHECK_LDU_DEBUG

void ReedSolomon_36_20_17_encode(ReedSolomon *rs, unsigned char hex_data[20], unsigned char *out_hex_parity)
{
    unsigned char input[47];
    unsigned int i;

    // Put the 20 hex words of data
    for(i=0; i<20; i++) {
        input[i] = hex_data[i];
    }

    // Fill up with zeros to complete the 47 expected hex words of data
    for(i=20; i<47; i++) {
        input[i] = 0;
    }

    // Now we can call encode on the base class
    rs6_encode(rs, input, out_hex_parity);
}

int ReedSolomon_36_20_17_decode(ReedSolomon *rs, unsigned char hex_data_and_parity[36], unsigned char *out_hex_parity)
{
    unsigned char input[63];
    unsigned int i;

    // First put the parity data, 16 hex words
    // Then the 20 hex words of data
    for(i=0; i<36; i++) {
        input[i] = hex_data_and_parity[i];
    }

    // Fill up with zeros to complete the 47 expected hex words of data
    for(i=36; i<63; i++) {
        input[i] = 0;
    }

    // Now we can call decode on the base class
    return rs6_decode(rs, input, out_hex_parity);
}

void ReedSolomon_24_12_13_encode(ReedSolomon *rs, unsigned char hex_data[12], unsigned char *out_hex_parity)
{
    unsigned char input[63];
    unsigned int i;

    // Put the 12 hex words of data
    for(i=0; i<12; i++) {
        input[i] = hex_data[i];
    }

    // Fill up with zeros to complete the 51 expected hex words of data
    for(i=12; i<51; i++) {
        input[i] = 0;
    }

    // Now we can call encode on the base class
    rs6_encode(rs, input, out_hex_parity);
}

int ReedSolomon_24_12_13_decode(ReedSolomon *rs, unsigned char hex_data_and_parity[24], unsigned char *out_hex_parity)
{
    unsigned char input[63];
    unsigned int i;

    // First put the parity data, 12 hex words
    // Then the 12 hex words of data
    for(i=0; i<24; i++) {
        input[i] = hex_data_and_parity[i];
    }

    // Fill up with zeros to complete the 51 expected hex words of data
    for(i=24; i<63; i++) {
        input[i] = 0;
    }

    // Now we can call decode on the base class
    return rs6_decode(rs, input, out_hex_parity);
}

// rs_mask = 0x96 for Voice Header, 0x99 for TLC.
unsigned int check_and_fix_reedsolomon_12_09_04(ReedSolomon *rs, unsigned char payload[12], unsigned char rs_mask)
{
  unsigned int i, errorFlag;
  unsigned char input[255];
  unsigned char output[255];

  for (i=0; i<((1<<8)-1); i++) {
    input[i] = 0;
  }

  for(i = 0; i < 12; i++) {
    input[i] = payload[i];
  }
  input[ 9] ^= rs_mask;
  input[10] ^= rs_mask;
  input[11] ^= rs_mask;

  /* decode recv[] */
  errorFlag = rs8_decode(rs, input, output);

#if 0
  output[ 9] ^= rs_mask;
  for(i = 0; i < 10; i++) {
    payload[i] = output[i];
  }
#endif
  return errorFlag;
}

unsigned int check_reedsolomon_12_09_04(ReedSolomon *rs, unsigned char *payload, unsigned char rs_mask)
{
    register int i, syn_error = 0;
    int recd[12];
    unsigned char s[4];

    for (i = 0; i < 9; i++)
       recd[i] = rs->index_of[payload[i]]; /* put recd[i] into index form (ie as powers of alpha) */
    recd[ 9] = rs->index_of[payload[ 9]^rs_mask]; /* put recd[i] into index form (ie as powers of alpha) */
    recd[10] = rs->index_of[payload[10]^rs_mask]; /* put recd[i] into index form (ie as powers of alpha) */
    recd[11] = rs->index_of[payload[11]^rs_mask]; /* put recd[i] into index form (ie as powers of alpha) */

    /* first form the syndromes */
    //s[0] = 0x69; // 0x44
    //s[1] = 0xf2; // 0xdd
    //s[2] = 0xe5; // 0x04
    //s[3] = 0x8d; // 0x92
    s[0] = 0x00; // 0x44
    s[1] = 0xa9; // 0xdd
    s[2] = 0x1b; // 0x04
    s[3] = 0x6b; // 0x92
    for (i = 0; i < 12; i++) {
        s[0] ^= rs->alpha_to[(unsigned int)(recd[i] +   i) % 255]; /* recd[j] in index form */
        s[1] ^= rs->alpha_to[(unsigned int)(recd[i] + 2*i) % 255]; /* recd[j] in index form */
        s[2] ^= rs->alpha_to[(unsigned int)(recd[i] + 3*i) % 255]; /* recd[j] in index form */
        s[3] ^= rs->alpha_to[(unsigned int)(recd[i] + 4*i) % 255]; /* recd[j] in index form */
    }

    if (s[0] != 0) syn_error++; /* set flag if non-zero syndrome => error */
    if (s[1] != 0) syn_error++; /* set flag if non-zero syndrome => error */
    if (s[2] != 0) syn_error++; /* set flag if non-zero syndrome => error */
    if (s[3] != 0) syn_error++; /* set flag if non-zero syndrome => error */
    return syn_error;
}

