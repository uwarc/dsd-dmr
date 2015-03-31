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

#include "dsd.h"
#define SCOPERATE 1

static const char *modulations[2] = { "GFSK", "QPSK" };

/* Note: okay to hardcode samples/symbol for now, but this will need changing when:
 *       p25p2 support gets added
 *       provoice support gets re-added (assuming that ever happens)
 */
void print_datascope(dsd_state *state, int *lbuf2, unsigned int lsize)
{
    static const char *separator = "+----------------------------------------------------------------+\n";
    char tmpbuf[1024];
    char spectrum[64];
    int i, j, o, tmplen;

    for (i = 0; i < 64; i++) {
        spectrum[i] = 0;
    }
    for (i = 0; i < lsize; i++) {
        o = (lbuf2[i] + 32768) / 1024;
        if (o > 63) o = 63;
        spectrum[o]++;
    }
    if (state->symbolcnt > (4800 / SCOPERATE)) {
        state->symbolcnt = 0;
        tmplen = snprintf(tmpbuf, 1023, "\033[14A\nDemod mode: %s, Min: %d, Max: %d, LMid: %d, UMid: %d, Center: %d, Symbol_Rate: 10\n",
                          modulations[state->rf_mod & 1], state->min, state->max, state->lmid, state->umid, state->center);
        write(1, tmpbuf, tmplen);
        write(1, separator, 67);
        for (i = 0; i < 10; i++) {
            char buf[67];
            for (j = 0; j < 64; j++) {
                if (i == 0) {
                    if ((j == ((state->min) + 32768) / 1024) || (j == ((state->max) + 32768) / 1024)) {
                        buf[j+1] = '#';
                    } else if ((j == ((state->lmid) + 32768) / 1024) || (j == ((state->umid) + 32768) / 1024)) { 
                        buf[j+1] = '^';
                    } else if (j == (state->center + 32768) / 1024) {
                        buf[j+1] = '!';
                    } else {
                        buf[j+1] = ((j == 32) ? '|' : ' ');
                   }
                } else {
                    if (spectrum[j] > 9 - i) {
                        buf[j+1] = '*';
                    } else {
                        buf[j+1] = ((j == 32) ? '|' : ' ');
                    }
                }
            }
            buf[0] = '|';
            buf[64] = '|';
            buf[65] = '\n';
            buf[66] = '\0';
            write(1, buf, 66);
        }
        write(1, separator, 67);
    }
}

