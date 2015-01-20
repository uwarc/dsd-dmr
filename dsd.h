#ifndef DSD_H
#define DSD_H
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

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#define __USE_XOPEN
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <math.h>
#include <mbelib.h>
#include "ReedSolomon.h"
#define SAMPLE_RATE_IN 48000
#define SAMPLE_RATE_OUT 8000
#define FSK4_NTAPS  8
#define FSK4_NSTEPS 128

/*
 * global variables
 */
int exitflag;

typedef struct
{
  int errorbars;
  unsigned int verbose;
  unsigned int p25status;
  unsigned int p25tg;
  unsigned int p25enc;
  unsigned int datascope;
  int audio_in_fd;
  int audio_in_type; // 0 for device, 1 for file, 2 for portaudio
  time_t mbe_out_last_timeval;
  char mbe_out_dir[1024];
  char mbe_out_path[1024];
  int mbe_out_fd;
  unsigned int serial_baud;
  int serial_fd;
  float audio_gain;
  unsigned int wav_out_samplerate;
  int wav_out_fd;
  unsigned int uvquality;
  unsigned char frame_types; // 0 -> DMR, 1 -> DStar, 2 -> NXDN48, 3 -> NXDN96, 4 -> P25P1
  unsigned char inverted_dmr;
  unsigned char inverted_x2tdma;
  unsigned int msize;
} dsd_opts;

typedef struct
{
  float d_history[FSK4_NTAPS+1];
  unsigned int d_history_last;
  float d_symbol_clock;
  float d_symbol_spread;
  float d_symbol_time;
  float fine_frequency_correction;
  float coarse_frequency_correction;

  int *dibit_buf;
  int *dibit_buf_p;
  int repeat;
  short inbuffer[4096];
  float inbuffer2[4096];
  unsigned int inbuf_pos;
  unsigned int inbuf_size;
  float audio_out_temp_buf[160];
  float *audio_out_temp_buf_p;
  //int wav_out_bytes;
  int center;
  int jitter;
  int synctype;
  int min;
  int max;
  int lmid;
  int umid;
  int minref;
  int maxref;
  int lastsample;
  int sbuf[128];
  unsigned int sidx;
  unsigned int ssize;
  int maxbuf[1024];
  int minbuf[1024];
  int midx;
  char err_str[64];
  char ftype[16];
  int symbolcnt;
  int rf_mod;
  int numflips;
  int lastsynctype;
  int lastp25type;
  int offset;
  int carrier;
  unsigned int talkgroup;
  unsigned int lasttg;
  unsigned int radio_id;
  unsigned int last_radio_id;
  unsigned short nac;
  unsigned short p25algid;
  unsigned short p25keyid;
  unsigned int p25kid;
  unsigned int numtdulc;
  int errs;
  int errs2;
  int optind;
  int firstframe;
  char slot0light[8];
  char slot1light[8];
  float aout_gain;
  float aout_max_buf[200];
  int aout_max_buf_idx;
  int samplesPerSymbol;
  int symbolCenter;
  int currentslot;
  mbe_parms cur_mp;
  mbe_parms prev_mp;
  mbe_parms prev_mp_enhanced;

  unsigned int debug_audio_errors;
  unsigned int debug_header_errors;
  unsigned int debug_header_critical_errors;

  // Last dibit read
  int last_dibit;

  unsigned int p25_bit_count;
  unsigned int p25_bit_error_count;

  ReedSolomon ReedSolomon_12_09_04;

  ReedSolomon ReedSolomon_24_12_13;
  ReedSolomon ReedSolomon_24_16_09;
  ReedSolomon ReedSolomon_36_20_17;

#ifdef TRACE_DSD
  char debug_prefix;
  char debug_prefix_2;
  unsigned int debug_sample_index;
  unsigned int debug_sample_left_edge;
  unsigned int debug_sample_right_edge;
  FILE* debug_label_file;
  FILE* debug_label_dibit_file;
#endif
} dsd_state;

/*
 * Frame sync patterns
 */
#define INV_P25P1_SYNC "333331331133111131311111"
#define P25P1_SYNC     "111113113311333313133333"

#define X2TDMA_BS_VOICE_SYNC "113131333331313331113311"
#define X2TDMA_BS_DATA_SYNC  "331313111113131113331133"
#define X2TDMA_MS_DATA_SYNC  "313113333111111133333313"
#define X2TDMA_MS_VOICE_SYNC "131331111333333311111131"

#define DSTAR_HD       "131313131333133113131111"
#define INV_DSTAR_HD   "313131313111311331313333"
#define DSTAR_SYNC     "313131313133131113313111"
#define INV_DSTAR_SYNC "131313131311313331131333"

// Conventional
#define NXDN_BS_DATA_SYNC      "313133113131111313"
#define NXDN_MS_DATA_SYNC      "313133113131111333"
#define NXDN_BS_VOICE_SYNC     "313133113131113113"
#define NXDN_MS_VOICE_SYNC     "313133113131113133"
#define INV_NXDN_BS_DATA_SYNC  "131311331313333131"
#define INV_NXDN_MS_DATA_SYNC  "131311331313333111"
#define INV_NXDN_BS_VOICE_SYNC "131311331313331331"
#define INV_NXDN_MS_VOICE_SYNC "131311331313331311"

// Trunked
#define NXDN_TC_VOICE_SYNC     "313133113113113113"
#define INV_NXDN_TC_VOICE_SYNC "131311331331331331"
#define NXDN_TD_VOICE_SYNC     "313133113133113111"
#define INV_NXDN_TD_VOICE_SYNC "131311331311331333"
#define INV_NXDN_TC_CC_SYNC "131311331333133131"
#define NXDN_TC_CC_SYNC     "313133113111311313"
#define INV_NXDN_TD_CC_SYNC "131311331311133331"
#define NXDN_TD_CC_SYNC     "313133113133311113"

#define DMR_BS_DATA_SYNC  "313333111331131131331131"
#define DMR_BS_VOICE_SYNC "131111333113313313113313"
#define DMR_MS_DATA_SYNC  "311131133313133331131113"
#define DMR_MS_VOICE_SYNC "133313311131311113313331"

#define INV_PROVOICE_SYNC    "31313111333133133311331133113311"
#define PROVOICE_SYNC        "13131333111311311133113311331133"
#define INV_PROVOICE_EA_SYNC "13313133113113333311313133133311"
#define PROVOICE_EA_SYNC     "31131311331331111133131311311133"

/*
 * Inline functions
 */
static inline unsigned int
get_uint(unsigned char *payload, unsigned int bits)
{
    unsigned int i, v = 0;
    for(i = 0; i < bits; i++) {
        v <<= 1;
        v |= payload[i];
    }
    return v;
}

/*
 * Function prototypes 
 */
void Golay23_Correct(unsigned int *block);
unsigned int Golay23_Encode(unsigned int cw);
void Hamming15_11_3_Correct(unsigned int *codeword);
void p25_hamming15_11_3_decode(unsigned int *codeword);

int getDibit (dsd_opts * opts, dsd_state * state);
void skipDibit (dsd_opts * opts, dsd_state * state, int count);
void Shellsort_int(int *in, unsigned int n);

int openAudioInDevice (dsd_opts *opts, const char *audio_in_dev);
void demodAmbe3600x24x0Data (int *errs2, char ambe_fr[4][24], char ambe_d[49]);
void saveAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d);
void closeMbeOutFile (dsd_opts * opts, dsd_state * state);
void openMbeOutFile (dsd_opts * opts, dsd_state * state);
void openWavOutFile (dsd_opts *opts, const char *wav_out_file);
void closeWavOutFile (dsd_opts *opts);
void processFrame (dsd_opts * opts, dsd_state * state);
void printFrameSync (dsd_opts * opts, dsd_state * state, char *frametype, int offset);
int getFrameSync (dsd_opts * opts, dsd_state * state);
void print_datascope(dsd_state *state, int *lbuf2, unsigned int lsize);
void noCarrier (dsd_opts * opts, dsd_state * state);
void cleanupAndExit (dsd_opts * opts, dsd_state * state);
void sigfun (int sig);
void processAMBEFrame (dsd_opts * opts, dsd_state * state, char ambe_fr[4][24]);
void processIMBEFrame (dsd_opts * opts, dsd_state * state, char imbe_d[88]);
int getSymbol (dsd_opts * opts, dsd_state * state, int have_sync);
void processEmb (dsd_state *state, unsigned char lcss, unsigned char emb_fr[4][32]);
void processDMRdata (dsd_opts * opts, dsd_state * state);
void processDMRvoice (dsd_opts * opts, dsd_state * state);
void processNXDNVoice (dsd_opts * opts, dsd_state * state);
void processNXDNData (dsd_opts * opts, dsd_state * state);
void processX2TDMAData (dsd_opts * opts, dsd_state * state);
void processDSTAR (dsd_opts * opts, dsd_state * state);
void processDSTAR_HD (dsd_opts * opts, dsd_state * state);
void process_p25_frame(dsd_opts *opts, dsd_state *state, unsigned char duid);
float get_p25_ber_estimate (dsd_state* state);
float dmr_filter(float sample);
float nxdn_filter(float sample);

void ReedSolomon_36_20_17_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity);
int ReedSolomon_36_20_17_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity);
void ReedSolomon_24_16_09_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity);
int ReedSolomon_24_16_09_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity);
void ReedSolomon_24_12_13_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity);
int ReedSolomon_24_12_13_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity);

#endif // DSD_H
