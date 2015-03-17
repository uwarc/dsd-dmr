#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "dsd.h"
#include "fsk4_interp_coeffs.h"
#include "fsk4_rrc_filter_coeffs.h"
#define FSK4_MAX(a,b) ((a) > (b) ? (a) : (b))
#define FSK4_MIN(a,b) ((a) > (b) ? (b) : (a))
float fast_atanf(float z);

#ifndef FFTCOMPLEX_T_DEFINED
typedef struct FFTComplex {
    float re, im;
} FFTComplex;
#define FFTCOMPLEX_T_DEFINED
#endif

#if defined(__amd64__) || defined(__x86_64__) || defined(__i386__) && defined(__SSE__)
float scalarproduct_fir_float_sse(float *v1, float *v2, unsigned int len);
#define scalarproduct_fir_float scalarproduct_fir_float_sse
#elif defined(__ARM_NEON__)
static float scalarproduct_float_neon(float *a, float *b, unsigned int len)
{
    float ret = 0.0f;
    uint32_t remainder = (len & 15);
    len -= remainder;

    __asm__ __volatile("  vmov.f32 q1, #0.0\n"
                       "  cmp %[len], #0\n"
                       "  bne 1f\n"
                       "  b 2f\n"
                       "1:"
                       "  vld1.32 {q8, q9}, [%[b]]!\n"
                       "  vld1.32 {q12, q13}, [%[a]]!\n"
                       "  vld1.32 {q10, q11}, [%[b]]!\n"
                       "  vld1.32 {q14, q15}, [%[a]]!\n"
                       "  vmla.f32 q1, q8, q12\n"
                       "  vmla.f32 q1, q9, q13\n"
                       "  vmla.f32 q1, q10, q14\n"
                       "  vmla.f32 q1, q11, q15\n"
                       "  subs %[len], %[len], #16\n"
                       "  bne 1b\n"
                       "  cmp %[remainder], #0\n"
                       "  beq 3f\n"
                       "2:"
                       "  vld1.32 {q2}, [%[b]]!\n"
                       "  vld1.32 {q3}, [%[a]]!\n"
                       "  vmla.f32 q1, q2, q3\n"
                       "  subs %[remainder], %[remainder], #4\n"
                       "  bne 2b\n"
                       "3:"
                       "  vadd.f32 d2, d2, d3\n"
                       "  vpadd.f32 d2, d2, d2\n"
                       "  vmov.f32 %[ret], d2[0]\n"
                       : [ret] "=&r" (ret), [a] "+r" (a), [b] "+r" (b),
                         [len] "+l" (len), [remainder] "+l" (remainder)
                       :
                       : "cc", "q1", "q2", "q3", "q8", "q9", "q10", "q11",
                               "q12", "q13", "q14", "q15");
    return ret;
}
#define scalarproduct_float scalarproduct_float_neon
#else
static float scalarproduct_fir_float_c(float *v1, float *v2, unsigned int len)
{
    float p = 0.0;
    unsigned int i;
    for (i = 0; i < len; i++) {
        float t = v1[i] + v1[2*len - i];
        p += t * v2[i];
    }
    p += v1[len] * v2[len];
    return p;
}
#define scalarproduct_fir_float scalarproduct_fir_float_c
#endif

// internal fast loop (must be this high to acquire symbol sync)
//#define K_FINE_FREQUENCY 0.125f
#define K_FINE_FREQUENCY 0.0625f
// tracking loop gain constant
#define K_SYMBOL_SPREAD 0.0100f
#define K_SYMBOL_TIMING 0.025f

// constraints on symbol spreading
// upper range limit: +20%
#define SYMBOL_SPREAD_MAX 2.4f
// lower range limit: -20%
#define SYMBOL_SPREAD_MIN 1.6f

unsigned int fsk4_tracking_loop_mmse(dsd_state *state, float input, float *output)
{
  float symbol_error;
  unsigned int i;

  state->d_symbol_clock += state->d_symbol_time;
      
  state->d_history[state->d_history_last++] = input;
  state->d_history_last %= FSK4_NTAPS;
      
  if(state->d_symbol_clock > 1.0f) {
    state->d_symbol_clock -= 1.0f;
	
	int imu = lrintf(FSK4_NSTEPS * ((state->d_symbol_clock / state->d_symbol_time)));
	if (imu >= FSK4_NSTEPS) { 
	  imu = FSK4_NSTEPS - 1;
	}
	
	unsigned int j = state->d_history_last;
	float interp = 0.0;
	float interp_p1 = 0.0;
	for(i=0; i<FSK4_NTAPS; i++) {
	    interp    +=  TAPS[imu  ][i] * state->d_history[j];
	    interp_p1 +=  TAPS[imu+1][i] * state->d_history[j];
	    j = (j+1) % FSK4_NTAPS;
    }
	
	// our output symbol will be interpolated value corrected for
	// symbol_spread and frequency offset
	interp -= state->fine_frequency_correction;
	interp_p1 -= state->fine_frequency_correction;
	
	// output is corrected for symbol deviation (spread)
	*output = 2.0f * interp / state->d_symbol_spread;
	
	// detect received symbol error: basically use a hard decision
	// and subtract off expected position nominal symbol level
	// which will be +/- 0.5 * symbol_spread and +/- 1.5 *
	// symbol_spread remember: nominal symbol_spread will be 2.0
	
	if(interp < -state->d_symbol_spread) {
	  // symbol is -3: Expected at -1.5 * symbol_spread
	  symbol_error = interp + (1.5f * state->d_symbol_spread);
	  state->d_symbol_spread -= (symbol_error * 0.5f * K_SYMBOL_SPREAD);
	} else if(interp < 0.0f) {
	  // symbol is -1: Expected at -0.5 * symbol_spread
	  symbol_error = interp + (0.5f * state->d_symbol_spread);
	  state->d_symbol_spread -= (symbol_error * K_SYMBOL_SPREAD);
	} else if(interp < state->d_symbol_spread) {
	  // symbol is +1: Expected at +0.5 * symbol_spread
	  symbol_error = interp - (0.5f * state->d_symbol_spread);
	  state->d_symbol_spread += (symbol_error * K_SYMBOL_SPREAD);
	} else {
	  // symbol is +3: Expected at +1.5 * symbol_spread
	  symbol_error = interp - (1.5f * state->d_symbol_spread);
	  state->d_symbol_spread += (symbol_error * 0.5f * K_SYMBOL_SPREAD);
	}
	
	// symbol clock tracking loop gain
	if(interp_p1 < interp) {
	  state->d_symbol_clock += symbol_error * K_SYMBOL_TIMING;  
	} else {
	  state->d_symbol_clock -= symbol_error * K_SYMBOL_TIMING;
	}
	
	// it seems reasonable to constrain symbol spread to +/- 20%
	// of nominal 2.0
	state->d_symbol_spread = FSK4_MAX(state->d_symbol_spread, SYMBOL_SPREAD_MIN);
	state->d_symbol_spread = FSK4_MIN(state->d_symbol_spread, SYMBOL_SPREAD_MAX);
	
	// fine loop 
	state->fine_frequency_correction += (symbol_error * K_FINE_FREQUENCY);
	return 1;
  }
  return 0;
}

// approximate sqrt(x^2 + y^2)
static float
envelope(FFTComplex x)
{
    float r_abs = fabsf(x.re);
    float i_abs = fabsf(x.im);

    if(r_abs > i_abs)
        return r_abs + 0.4f * i_abs;
    else
        return i_abs + 0.4f * r_abs;
}

static void
feedforward_agc(FFTComplex *in, unsigned int ninput, FFTComplex *out)
{
    unsigned int i, j;
    float gain;

    for(i = 0; i < ninput; i++) {
      //float max_env = 1e-12;    // avoid divide by zero
      float max_env = 1e-4;   // avoid divide by zero, indirectly set max gain
      for(j = 0; j < 16; j++) {
        max_env = FSK4_MAX(max_env, envelope(in[i+j]));
      }
      gain = 1.0f / max_env;
      out[i].re = gain * in[i].re;
      out[i].im = gain * in[i].im;
    }
}

static inline float polar_disc_fast(float ar, float aj, float br, float bj)
{
    float x = ar*br + aj*bj;
    float y = aj*br - ar*bj;
    float z;
    
    if ((y != y) || (x != x))
        return 0.0f;

    y += 1e-12f;
    if (x < 1e-12f) {
        z = (float)M_PI * 0.5f;
    } else {
        /* compute y/x */
        z=fast_atanf(fabsf(y/x));
        if (x < 0.0f) {
            z = (float)M_PI - z;
        }
    }

    if (z != z) {
        z = 0.0f;
    }

    if (y < 0.0f) {
        z = -z;
    }

    return (z * 0.31831f);
}

static float 
readFromBuffer_s16(int in_fd, dsd_state *state, ssize_t *result)
{
    short tmpbuf[1024];
    unsigned int i;
    float ret = 0.0f;
    if (state->inbuf_pos < state->inbuf_size) {
        *result = 1;
        ret = state->inbuffer[state->inbuf_pos++];
    } else {
        state->inbuf_pos = 1;
        *result = read(in_fd, tmpbuf, 1024 * sizeof(short));
        state->inbuf_size = (*result / sizeof(short));
        for (i = 0; i < state->inbuf_size; i++) {
            state->inbuffer[i] = ((float)tmpbuf[i] / 32768.0f);
        }
        ret = state->inbuffer[0];
    }
    return ret;
}

static float
readFromBuffer_f32(int in_fd, dsd_state *state, ssize_t *result)
{
    float ret = 0.0f;
    if (state->inbuf_pos < state->inbuf_size) {
        *result = 1;
        ret = state->inbuffer[state->inbuf_pos++];
    } else {
        state->inbuf_pos = 1;
        *result = read(in_fd, state->inbuffer, 1024 * sizeof(float));
        state->inbuf_size = (*result / sizeof(float));
        ret = state->inbuffer[0];
    }
    if (ret != ret) ret = 0.0f;
    return ret;
}

static float
readFromBuffer_f32_iq(int in_fd, dsd_state *state, ssize_t *result)
{
    FFTComplex iqbuf[1024];
    unsigned int i;
    float ret = 0.0f;
    if (state->inbuf_pos < state->inbuf_size) {
        *result = 1;
        ret = state->inbuffer[state->inbuf_pos++];
    } else {
        state->inbuf_pos = 1;
        *result = read(in_fd, iqbuf, 1024 * sizeof(FFTComplex));
        state->inbuf_size = (*result / sizeof(FFTComplex));
        state->inbuffer[0] = polar_disc_fast(iqbuf[0].re, iqbuf[0].im, state->prev_r, state->prev_j);
        for (i = 1; i < state->inbuf_size; i++) {
            state->inbuffer[i] = polar_disc_fast(iqbuf[i].re, iqbuf[i].im,
                                                 iqbuf[i-1].re, iqbuf[i-1].im);
        }
        state->prev_r = iqbuf[state->inbuf_size - 1].re;
        state->prev_j = iqbuf[state->inbuf_size - 1].im;
        ret = state->inbuffer[0];
    }
    if (ret != ret) ret = 0.0f;
    return ret;
}

// 1.0546365475f
float ngain = 0.101333437f;
float nxgain = 0.062659371644565f;

float
dmr_filter(dsd_state *state, float sample)
{
  float sum = 0.0f;
  unsigned int i;

  for (i = 0; i < RRC_NZEROS; i++)
    state->xv[i] = state->xv[i+1];

  state->xv[RRC_NZEROS] = sample; // unfiltered sample in
  sum = scalarproduct_fir_float(state->xv, xcoeffs, 40);
  return (sum * ngain); // filtered sample out
}

int
getSymbol (dsd_opts *opts, dsd_state *state, int have_sync)
{
    float sample_out = 0.0f;
    ssize_t result = 0;

    // first we run through all provided data
    while (1) {
        float sample = 0.0f;

        // Read the new sample from the input
        if(opts->audio_in_format == 0) {
            //result = read (opts->audio_in_fd, &tmp, 2);
            sample = readFromBuffer_s16(opts->audio_in_fd, state, &result);
        } else if (opts->audio_in_format == 1) {
            //result = read (opts->audio_in_fd, &sample, 4);
            sample = readFromBuffer_f32(opts->audio_in_fd, state, &result);
        } else if (opts->audio_in_format == 2) {
            sample = readFromBuffer_f32_iq(opts->audio_in_fd, state, &result);
        }

        if(result <= 0) {
          cleanupAndExit (opts, state);
        }

        sample = dmr_filter(state, sample);
        sample *= 16.325f;
        sample *= state->input_gain;
        if (fsk4_tracking_loop_mmse(state, sample, &sample_out)) {
            state->symbolcnt++;
            return lrintf(sample_out * 1024.0f);
        }
    }
    return 0;
}

