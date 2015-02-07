#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
uint64_t bchEnc(uint16_t in_word);
int bchDec(uint64_t cw, uint16_t *cw_decoded);

int main(int argc, char **argv) {
    unsigned int hdr = strtoul(argv[1], NULL, 0);
    uint16_t nac_duid_test, nac_duid_in = (hdr & 0xFFFF);
    uint64_t encoded_nac_duid = bchEnc(nac_duid_in);
    int nerrs = bchDec(encoded_nac_duid, &nac_duid_test);
    fprintf(stderr, "In: 0x%04x (NAC: 0x%X, DUID: 0x%X) -> 0x%08lx -> 0x%04x\n",
            nac_duid_in, (nac_duid_in & 0x0FFF), (nac_duid_in >> 12),
            encoded_nac_duid, nac_duid_test);
    fprintf(stderr, "nerrs: %d\n", nerrs); 
    return 0;
}

