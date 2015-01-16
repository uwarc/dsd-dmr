#include <stdint.h>
#include "ReedSolomon.h"
#include "dsd.h"

// Uncomment for very verbose trace messages
//#define CHECK_LDU_DEBUG

static inline void hex_to_bin(unsigned char *output, unsigned int input)
{
  output[0] = ((input >> 5) & 1);
  output[1] = ((input >> 4) & 1);
  output[2] = ((input >> 3) & 1);
  output[3] = ((input >> 2) & 1);
  output[4] = ((input >> 1) & 1);
  output[5] = ((input >> 0) & 1);
}

void ReedSolomon_36_20_17_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity)
{
    unsigned char input[47];
    unsigned char output[63];
    unsigned int i;

    // Put the 20 hex words of data
    for(i=0; i<20; i++) {
        input[i] = get_uint(hex_data + i*6, 6);
    }

    // Fill up with zeros to complete the 47 expected hex words of data
    for(i=20; i<47; i++) {
        input[i] = 0;
    }

    // Now we can call encode on the base class
    rs6_encode(rs, input, output);

    // Convert it back to binary form and put it into the parity
    for(i=0; i<16; i++) {
        hex_to_bin(out_hex_parity + i*6, output[i]);
    }
}

int ReedSolomon_36_20_17_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity)
{
    unsigned char input[63];
    unsigned char output[63];
    unsigned int i, irrecoverable_errors;

    // First put the parity data, 16 hex words
    for(i=0; i<16; i++) {
        input[i] = get_uint(hex_parity + i*6, 6);
    }

    // Then the 20 hex words of data
    for(i=16; i<16+20; i++) {
        input[i] = get_uint(hex_data + (i-16)*6, 6);
    }

    // Fill up with zeros to complete the 47 expected hex words of data
    for(i=16+20; i<63; i++) {
        input[i] = 0;
    }

    // Now we can call decode on the base class
    irrecoverable_errors = rs6_decode(rs, input, output);

    // Convert it back to binary and put it into hex_data. If decode failed we should have
    // the input unchanged.
    for(i=16; i<16+20; i++) {
        hex_to_bin(hex_data + (i-16)*6, output[i]);
    }

    return irrecoverable_errors;
}

void ReedSolomon_24_16_09_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity)
{
    unsigned char input[63];
    unsigned char output[63];
    unsigned int i;

    // Put the 16 hex words of data
    for(i=0; i<16; i++) {
        input[i] = get_uint(hex_data + i*6, 6);
    }

    // Fill up with zeros to complete the 55 expected hex words of data
    for(i=16; i<55; i++) {
        input[i] = 0;
    }

    // Now we can call encode on the base class
    rs6_encode(rs, input, output);

    // Convert it back to binary form and put it into the parity
    for(i=0; i<8; i++) {
        hex_to_bin(out_hex_parity + i*6, output[i]);
    }
}

int ReedSolomon_24_16_09_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity)
{
    unsigned char input[63];
    unsigned char output[63];
    unsigned int i, irrecoverable_errors;

    // First put the parity data, 8 hex words
    for(i=0; i<8; i++) {
        input[i] = get_uint(hex_parity + i*6, 6);
    }

    // Then the 16 hex words of data
    for(i=8; i<8+16; i++) {
        input[i] = get_uint(hex_data + (i-8)*6, 6);
    }

    // Fill up with zeros to complete the 55 expected hex words of data
    for(i=8+16; i<63; i++) {
        input[i] = 0;
    }

    // Now we can call decode on the base class
    irrecoverable_errors = rs6_decode(rs, input, output);

    // Convert it back to binary and put it into hex_data. If decode failed we should have
    // the input unchanged.
    for(i=8; i<8+16; i++) {
        hex_to_bin(hex_data + (i-8)*6, output[i]);
    }

    return irrecoverable_errors;
}

int ReedSolomon_24_12_13_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity)
{
    unsigned char input[63];
    unsigned char output[63];
    unsigned int i, irrecoverable_errors;

    // First put the parity data, 12 hex words
    for(i=0; i<12; i++) {
        input[i] = get_uint(hex_parity + i*6, 6);
    }

    // Then the 12 hex words of data
    for(i=12; i<12+12; i++) {
        input[i] = get_uint(hex_data + (i-12)*6, 6);
    }

    // Fill up with zeros to complete the 51 expected hex words of data
    for(i=12+12; i<63; i++) {
        input[i] = 0;
    }

    // Now we can call decode on the base class
    irrecoverable_errors = rs6_decode(rs, input, output);

    // Convert it back to binary and put it into hex_data. If decode failed we should have
    // the input unchanged.
    for(i=12; i<12+12; i++) {
        hex_to_bin(hex_data + (i-12)*6, output[i]);
    }

    return irrecoverable_errors;
}

void ReedSolomon_24_12_13_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity)
{
    unsigned char input[63];
    unsigned char output[63];
    unsigned int i;

    // Put the 12 hex words of data
    for(i=0; i<12; i++) {
        input[i] = get_uint(hex_data + i*6, 6);
    }

    // Fill up with zeros to complete the 51 expected hex words of data
    for(i=12; i<51; i++) {
        input[i] = 0;
    }

    // Now we can call encode on the base class
    rs6_encode(rs, input, output);

    // Convert it back to binary form and put it into the parity
    for(i=0; i<12; i++) {
        hex_to_bin(out_hex_parity + i*6, output[i]);
    }
}

