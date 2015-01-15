/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Note: D-STAR support is fairly complete at this point.
 * The ambe3600x2450 decoder is similar butnot compatible with D-STAR voice frames. 
 * The dstar interleave pattern is different as well.
 * GMSK modulation optimizations will also required to get a usable bit error
 */

#include "dsd.h"
#include "dstar_const.h"
#include "descramble.h"

void processDSTAR(dsd_opts * opts, dsd_state * state) {
	// extracts AMBE frames from D-STAR voice frame
	int i, j, dibit;
	char ambe_fr[4][24];
	int total_errs = 0, sync_missed = 0, framecount;
	unsigned char slowdata[4];
	unsigned int bitbuffer = 0;
	const int *w, *x;

	if (state->synctype == 18) {
		framecount = 0;
		state->synctype = 6;
	} else if (state->synctype == 19) {
		framecount = 0;
		state->synctype = 7;
	} else {
		framecount = 1; //just saw a sync frame; there should be 20 not 21 till the next
	}

	while (sync_missed < 3) {
		memset(ambe_fr, 0, 96);
		// voice frame
	    w = dW;
	    x = dX;

		for (i = 0; i < 72; i++) {
			dibit = getDibit(opts, state);
			bitbuffer <<= 1;
			if (dibit == 1) {
				bitbuffer |= 0x01;
			}
			if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
				// we're slipping bits
				printf("sync in voice after i=%d, restarting\n", i);
				//ugh just start over
				i = 0;
			    w = dW;
			    x = dX;
				framecount = 1;
				continue;
			}
			ambe_fr[*w][*x] = (1 & dibit);
			w++;
			x++;
		}
		processAMBEFrame(opts, state, ambe_fr);
        total_errs += state->errs2;

		//  data frame - 24 bits
		for (i = 73; i < 97; i++) {
			dibit = getDibit(opts, state);
			bitbuffer <<= 1;
			if (dibit == 1) {
				bitbuffer |= 0x01;
			}
			if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
				// looking if we're slipping bits
				if (i != 96) {
					printf("sync after i=%d\n", i);
					i = 96;
				}
			}
		}

		slowdata[0] = (bitbuffer >> 16) & 0x000000FF;
		slowdata[1] = (bitbuffer >> 8) & 0x000000FF;
		slowdata[2] = (bitbuffer) & 0x000000FF;
		slowdata[3] = 0;

		if ((bitbuffer & 0x00FFFFFF) == 0x00AAB468) {
			//We got sync!
			//printf("Sync on framecount = %d\n", framecount);
			sync_missed = 0;
		} else if ((bitbuffer & 0x00FFFFFF) == 0xAAAAAA) {
			//End of transmission
			printf("End of transmission\n");
			goto end;
		} else if (framecount % 21 == 0) {
			printf("Missed sync on framecount = %d, value = %x/%x/%x\n",
					framecount, slowdata[0], slowdata[1], slowdata[2]);
			sync_missed++;
		} else if (framecount != 0 && (bitbuffer & 0x00FFFFFF) != 0x000000) {
			slowdata[0] ^= 0x70;
			slowdata[1] ^= 0x4f;
			slowdata[2] ^= 0x93;
			//printf("unscrambled- %s",slowdata);
		} else if (framecount == 0) {
			//printf("never scrambled-%s\n",slowdata);
		}
		framecount++;
	}

end:
    if (opts->errorbars) {
        int level = (int) state->max / 164;
        printf ("Sync: %s mod: %s      inlvl: %2i%% %s %s  VOICE e: %u\n",
                state->ftype, ((state->rf_mod == 2) ? "GFSK" : "C4FM"),
                level, state->slot0light, state->slot1light, total_errs);
    }
}

#define SCRAMBLER_TABLE_BITS_LENGTH 720
static const unsigned char SCRAMBLER_TABLE_BITS[SCRAMBLER_TABLE_BITS_LENGTH+1] = {
	0,0,0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,
	0,0,1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,
	1,1,0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,
	1,1,1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,
	0,0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,
	0,1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,
	1,0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,
	1,1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,
	0,0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,
	1,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,
	0,1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,
	1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,
	0,1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,
	0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,
	1,0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,1,
	1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
	1,1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,
	0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,1,
	0,1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,1,1,1,1,
	1,0,1,0,0,1,0,1,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,0,1,
	1,1,0,1,1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,
	1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,1,0,0,0,0,0,1,1,0,0,1,1,0,1,0,
	1,0,0,1,1,1,0,0,1,1,1,1,0,1,1,0
};

void processDSTAR_HD(dsd_opts * opts, dsd_state * state) {
	int radioheaderbuffer[660];
	int radioheaderbuffer2[660];
	unsigned int i, j, m_count = 0, bitcount = 0;
	unsigned char bit2octet[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
	unsigned char radioheader[41];

	for (j = 0; j < 660; j++) {
	    radioheaderbuffer[j] = getDibit(opts, state);
	}

    for (i=0; i < 660; i++) {
	    radioheaderbuffer2[i] = radioheaderbuffer[i] ^ SCRAMBLER_TABLE_BITS[m_count++];
	    if (m_count >= SCRAMBLER_TABLE_BITS_LENGTH) {
		    m_count = 0U;
	    }
    }

    for (i = 0; i < 660; i++) {
        radioheaderbuffer[j] = radioheaderbuffer2[i];
        j += 24;
        if (j >= 672) {
            j -= 671;
        } else if (j >= 660) {
            j -= 647;
        }
    }

	// Note: These routines contain GPLed code. Remove if you object to that.
	// Due to this, they are in a separate source file.
	FECdecoder(radioheaderbuffer, radioheaderbuffer2);

	// note we receive 330 bits, but we only use 328 of them (41 octets)
	// bits 329 and 330 are unused
    for (i = 0; i < 41; i++) {
        radioheader[i] = 0;
    }
	for (i = 0; i < 328; i++) {
		if (radioheaderbuffer2[i]) {
			radioheader[(bitcount >> 3)] |= bit2octet[(bitcount & 7)];
		}
		bitcount++;
	}

	// print header
#ifdef DEBUG_DSTAR_HD
	printf("\nDSTAR HEADER: ");
	printf("FLAG1: %02X - FLAG2: %02X - FLAG3: %02X\n", radioheader[0],
			radioheader[1], radioheader[2]);
	printf("RPT 2: %c%c%c%c%c%c%c%c ", radioheader[3], radioheader[4],
			radioheader[5], radioheader[6], radioheader[7], radioheader[8],
			radioheader[9], radioheader[10]);
	printf("RPT 1: %c%c%c%c%c%c%c%c ", radioheader[11], radioheader[12],
			radioheader[13], radioheader[14], radioheader[15], radioheader[16],
			radioheader[17], radioheader[18]);
	printf("YOUR: %c%c%c%c%c%c%c%c ", radioheader[19], radioheader[20],
			radioheader[21], radioheader[22], radioheader[23], radioheader[24],
			radioheader[25], radioheader[26]);
	printf("MY: %c%c%c%c%c%c%c%c/%c%c%c%c\n", radioheader[27],
			radioheader[28], radioheader[29], radioheader[30], radioheader[31],
			radioheader[32], radioheader[33], radioheader[34], radioheader[35],
			radioheader[36], radioheader[37], radioheader[38]);
#endif

	//We officially have sync now, so just pass on to the above routine:
	processDSTAR(opts, state);
}

