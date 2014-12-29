#include <stdint.h>

// Hamming (13, 9, 3)
static int Hamming1393Gen[9] = {
    0x100f, 0x080e, 0x0407, 0x020a, 0x0105, 0x008b, 0x004c, 0x0026, 0x0013
};

static int Hamming1393Table[16] = {
    0x0000, 0x0001, 0x0002, 0x0013, 0x0004, 0x0105, 0x0026, 0x0407,
    0x0008, 0x0000, 0x020A, 0x008b, 0x004c, 0x0000, 0x080e, 0x100f
};

static unsigned int doHamming1393(uint8_t bitarray[13])
{
    unsigned int i, codeword = 0, ecc = 0, syndrome, mask = 0x1000;
    for(i = 0; i < 13; i++) {
        codeword <<= 1;
        codeword |= bitarray[i];
    }
    for(i = 0; i < 9; i++) {
        if((codeword & Hamming1393Gen[i]) > 0x0f)
            ecc ^= Hamming1393Gen[i];
    }
    syndrome = ecc ^ codeword;
    if(syndrome == 0)
        return 0;  // No error
    syndrome = Hamming1393Table[syndrome & 0x0f];
    syndrome ^= codeword;  // syndrome now has corrected bits
    if(syndrome == codeword) // Non correctable error
        return codeword ^ ecc; // indicate error
    for(i = 0; i < 13; i++) { 
        bitarray[i] = (syndrome & mask)?1:0;
        mask >>= 1;
    }
    return 0;
}

// Hamming (15, 11, 3)
static int Hamming15113Gen[11] = {
    0x4009, 0x200d, 0x100f, 0x080e, 0x0407, 0x020a, 0x0105, 0x008b, 0x004c, 0x0026, 0x0013
};

static int Hamming15113Table[16] = {
    0x0000, 0x0001, 0x0002, 0x0013, 0x0004, 0x0105, 0x0026, 0x0407,
    0x0008, 0x4009, 0x020A, 0x008b, 0x004C, 0x200D, 0x080E, 0x100F
};

static unsigned int doHamming15113(uint8_t bitarray[15])
{
    unsigned int i, codeword = 0, ecc = 0, syndrome, mask = 0x4000;
    for(i = 0; i < 15; i++) {
        codeword <<= 1;
        codeword |= bitarray[i];
    }
    for(i = 0; i < 11; i++) {
        if((codeword & Hamming15113Gen[i]) > 0xf)
            ecc ^= Hamming15113Gen[i];
    }
    syndrome = ecc ^ codeword;
    if(syndrome == 0)
        return 0;  // No error
    syndrome = Hamming15113Table[syndrome & 0x0f];
    syndrome ^= codeword;  // syndrome now has corrected bits
    if(syndrome == codeword) // Non correctable error
        return codeword ^ ecc; // indicate error
    for(i = 0; i < 15; i++) {
        bitarray[i] = (syndrome & mask)?1:0;
        mask >>= 1;
    }
    return 0;
}

// deinterleaved_data[a] = rawdata[(a*181)%196];
int processBPTC(unsigned char infodata[196], unsigned char payload[97])
{
  unsigned int i, j, k;
  unsigned char data_fr[196];
  for(i = 1; i < 197; i++) {
    data_fr[i-1] = infodata[((i*181)%196)];
  }

  for (i = 0; i < 3; i++)
     data_fr[0 * 15 + i] = 0; // Zero reserved bits
  for (i = 0; i < 15; i++) {
    unsigned char data_col[13];
    for(j = 0; j < 13; j++) {
      data_col[j] = data_fr[j * 15 + i];
    }
    if(doHamming1393(data_col) != 0) {
      return -1;
    }
  }
  for (j = 0; j < 9; j++) {
    if(doHamming15113(&data_fr[j * 15]) != 0) {
      return -2;
    }
  }
  for (i = 3, k = 0; i < 11; i++, k++) {
     payload[k] = data_fr[0 * 15 + i];
  }
  for (j = 1; j < 9; j++) {
    for (i = 0; i < 11; i++, k++) {
      payload[k] = data_fr[j * 15 + i];
    }
  }
  return 0;
}

