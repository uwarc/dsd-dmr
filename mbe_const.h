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
 * AMBE interleave schedule
 */
const unsigned char rX[72] = {                                                  
  23,  5, 10,  3, 22,  4, 9,  2, 21,  3, 8, 1, 20,  2, 7, 0, 19,  1, 6, 13, 18,  0,  5, 12,
  17, 22,  4, 11, 16, 21, 3, 10, 15, 20, 2, 9, 14, 19, 1, 8, 13, 18, 0,  7, 12, 17, 10, 6,
  11, 16,  9,  5, 10, 15, 8,  4,  9, 14, 7, 3,  8, 13, 6, 2,  7, 12, 5,  1,  6, 11,  4, 0
};

const unsigned char rW[72] = {                                                  
  0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 3, 0, 0, 1, 3,       
  0, 1, 1, 3, 0, 1, 1, 3, 0, 1, 1, 3, 0, 1, 1, 3, 0, 1, 1, 3, 0, 1, 2, 3,       
  0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3        
};

/*
 * P25 Phase1 IMBE interleave schedule
 */

const unsigned char iW[72] = {
  0, 2, 4, 1, 3, 5,
  0, 2, 4, 1, 3, 6,
  0, 2, 4, 1, 3, 6,
  0, 2, 4, 1, 3, 6,
  0, 2, 4, 1, 3, 6,
  0, 2, 4, 1, 3, 6,
  0, 2, 5, 1, 3, 6,
  0, 2, 5, 1, 3, 6,
  0, 2, 5, 1, 3, 7,
  0, 2, 5, 1, 3, 7,
  0, 2, 5, 1, 4, 7,
  0, 3, 5, 2, 4, 7
};

const unsigned char iX[72] = {
  22, 20, 10, 20, 18, 0,
  20, 18, 8, 18, 16, 13,
  18, 16, 6, 16, 14, 11,
  16, 14, 4, 14, 12, 9,
  14, 12, 2, 12, 10, 7,
  12, 10, 0, 10, 8, 5,
  10, 8, 13, 8, 6, 3,
  8, 6, 11, 6, 4, 1,
  6, 4, 9, 4, 2, 6,
  4, 2, 7, 2, 0, 4,
  2, 0, 5, 0, 13, 2,
  0, 21, 3, 21, 11, 0
};

const unsigned char iY[72] = {
  1, 3, 5, 0, 2, 4,
  1, 3, 6, 0, 2, 4,
  1, 3, 6, 0, 2, 4,
  1, 3, 6, 0, 2, 4,
  1, 3, 6, 0, 2, 4,
  1, 3, 6, 0, 2, 5,
  1, 3, 6, 0, 2, 5,
  1, 3, 6, 0, 2, 5,
  1, 3, 6, 0, 2, 5,
  1, 3, 7, 0, 2, 5,
  1, 4, 7, 0, 3, 5,
  2, 4, 7, 1, 3, 5
};

const unsigned char iZ[72] = {
  21, 19, 1, 21, 19, 9,
  19, 17, 14, 19, 17, 7,
  17, 15, 12, 17, 15, 5,
  15, 13, 10, 15, 13, 3,
  13, 11, 8, 13, 11, 1,
  11, 9, 6, 11, 9, 14,
  9, 7, 4, 9, 7, 12,
  7, 5, 2, 7, 5, 10,
  5, 3, 0, 5, 3, 8,
  3, 1, 5, 3, 1, 6,
  1, 14, 3, 1, 22, 4,
  22, 12, 1, 22, 20, 2
};

/*
 * ProVoice IMBE interleave schedule
 */

const unsigned char pW[142] = {
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 3, 4, 5, 6,
  1, 2, 3, 4, 5, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  1, 2, 3, 4, 5, 6,
  1, 2, 3, 4, 5,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 4, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 3, 5, 6,
  0, 1, 2, 4, 5, 6,
  1, 2, 3, 4, 5, 6,
  1, 2, 3, 4, 6
};

const unsigned char pX[142] = {
  18, 18, 17, 16, 7, 21,
  15, 15, 14, 13, 4, 18,
  12, 12, 11, 10, 1, 15,
  9, 9, 8, 7, 13, 12,
  6, 6, 5, 4, 10, 9,
  3, 3, 2, 1, 7, 6,
  0, 0, 22, 13, 4, 3,
  21, 20, 19, 10, 1, 0,
  17, 17, 16, 15, 6, 20,
  14, 14, 13, 12, 3, 17,
  11, 11, 10, 9, 0, 14,
  8, 8, 7, 6, 12, 11,
  5, 5, 4, 3, 9, 8,
  2, 2, 1, 0, 6, 5,
  23, 22, 21, 12, 3, 2,
  20, 19, 18, 9, 0,
  16, 16, 15, 14, 5, 19,
  13, 13, 12, 11, 2, 16,
  10, 10, 9, 8, 14, 13,
  7, 7, 6, 5, 11, 10,
  4, 4, 3, 2, 8, 7,
  1, 1, 0, 14, 5, 4,
  22, 21, 20, 11, 2, 1,
  19, 18, 17, 8, 22
};

