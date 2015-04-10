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

  for(i = 0; i < 12; i++) {
    payload[i] = output[i];
  }
  return errorFlag;
}

