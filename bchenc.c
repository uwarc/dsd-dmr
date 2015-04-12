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
    fprintf(stderr, "In: 0x%04x -> 0x%08lx -> 0x%04x (nerrs: %d)\n",
            nac_duid_in, encoded_nac_duid, nac_duid_test, nerrs);
    return 0;
}

