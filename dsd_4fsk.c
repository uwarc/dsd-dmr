#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "dsd.h"
#include "fsk4_interp_coeffs.h"
#include "fast_atanf.h"
#define FSK4_MAX(a,b) ((a) > (b) ? (a) : (b))
#define FSK4_MIN(a,b) ((a) > (b) ? (b) : (a))

static const float inv_pi  =  0.3183098733;  /* 0x3ea2f984 */ 

// DMR filter
// Filter details ..
// filtertype	 =	 Raised Cosine
// samplerate	 =	 48000
// corner	 =	 2400
// beta	 =	 0.2 
// impulselen	 =	 81
// racos	 =	 sqrt
// comp	 =	 no

static float __attribute__((aligned(16))) xcoeffs[84] = {
    +0.0273676736, +0.0190682959, +0.0070661879, -0.0075385898,
    -0.0231737159, -0.0379433607, -0.0498333862, -0.0569528373,
    -0.0577853377, -0.0514204905, -0.0377352004, -0.0174982391,
    +0.0076217868, +0.0351552125, +0.0620353691, +0.0848941519,
    +0.1004237235, +0.1057694293, +0.0989127431, +0.0790009892,
    +0.0465831968, +0.0037187043, -0.0460635022, -0.0979622825,
    -0.1462501260, -0.1847425896, -0.2073523972, -0.2086782295,
    -0.1845719273, -0.1326270847, -0.0525370892, +0.0537187153,
    +0.1818868577, +0.3256572849, +0.4770745929, +0.6271117870,
    +0.7663588857, +0.8857664963, +0.9773779594, +1.0349835419,
    +1.0546365475, +1.0349835419, +0.9773779594, +0.8857664963,
    +0.7663588857, +0.6271117870, +0.4770745929, +0.3256572849,
    +0.1818868577, +0.0537187153, -0.0525370892, -0.1326270847,
    -0.1845719273, -0.2086782295, -0.2073523972, -0.1847425896,
    -0.1462501260, -0.0979622825, -0.0460635022, +0.0037187043,
    +0.0465831968, +0.0790009892, +0.0989127431, +0.1057694293,
    +0.1004237235, +0.0848941519, +0.0620353691, +0.0351552125,
    +0.0076217868, -0.0174982391, -0.0377352004, -0.0514204905,
    -0.0577853377, -0.0569528373, -0.0498333862, -0.0379433607,
    -0.0231737159, -0.0075385898, +0.0070661879, +0.0190682959,
    +0.0273676736, 0.0, 0.0, 0.0
};

#if defined(__amd64__) || defined(__x86_64__) || defined(__i386__) && defined(__SSE__)
static float scalarproduct_float_sse(float *v1, float *v2, unsigned int len) {
        float sum = 0.0f;
        unsigned int i;

        for(i=0; i<len; i+=4) {
            __asm__ volatile(
                 "movups  (%1), %%xmm2 \n\t"
                 "movaps  (%2), %%xmm3 \n\t"
                 "mulps %%xmm3, %%xmm2 \n\t"
                 "addps %%xmm2, %0 \n\t"
                :"=x"(sum) :"r"(&v1[i]), "r"(&v2[i]));
        }
        __asm__ volatile(
                 "movaps      %0, %%xmm5 \n\t"
                 "shufps   $0x1b, %0, %%xmm5 \n\t"
                 "addps       %0, %%xmm5 \n\t"
                 "movhlps %%xmm5, %0     \n\t"
                 "addps   %%xmm5, %0     \n\t"
                :"+x"(sum));
        return sum;
}
#define scalarproduct_float scalarproduct_float_sse
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
static float scalarproduct_float_c(float *v1, float *v2, unsigned int len)
{
    float p = 0.0;
    unsigned int i;
    for (i = 0; i < len; i++) {
        p += v1[i] * v2[i];
    }
    return p;
}
#define scalarproduct_float scalarproduct_float_c
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

static const float
C1  = -5.0000000000e-01,
C2  =  4.1666667908e-02, /* 0x3d2aaaab */
C3  = -1.3888889225e-03, /* 0xbab60b61 */
C4  =  2.4801587642e-05, /* 0x37d00d01 */
C5  = -2.7557314297e-07, /* 0xb493f27c */
C6  =  2.0875723372e-09, /* 0x310f74f6 */
C7  = -1.1359647598e-11; /* 0xad47d74e */

// Differs from libc cosf on [0, pi/2] by at most 0.0000001229f
// Differs from libc cosf on [0, pi] by at most 0.0000035763f
static inline float k_cosf(float x)
{
    float z = x*x;
    return (1.0f+z*(C1+z*(C2+z*(C3+z*(C4+z*(C5+z*(C6+z*C7)))))));
}

static const float
S1  = -1.66666666666666324348e-01, /* 0xBFC55555, 0x55555549 */
S2  =  8.33333333332248946124e-03, /* 0x3F811111, 0x1110F8A6 */
S3  = -1.98412698298579493134e-04, /* 0xBF2A01A0, 0x19C161D5 */
S4  =  2.75573137070700676789e-06, /* 0x3EC71DE3, 0x57B1FE7D */
S5  = -2.50507602534068634195e-08, /* 0xBE5AE5E6, 0x8A2B9CEB */
S6  =  1.58969099521155010221e-10; /* 0x3DE5D93A, 0x5ACFD57C */

// Differs from libc sinf on [0, pi/2] by at most 0.0000001192f
// Differs from libc sinf on [0, pi] by at most 0.0000170176f
static inline float k_sinf(float x)
{
    float z = x*x;
    return x*(1.0f+z*(S1+z*(S2+z*(S3+z*(S4+z*(S5+z*S6))))));
}

float sinc(float x)
{
    float z;
    if (x < 0.0f) {
        x = -x;
    }
    if (x < 1e-12f) {
        z = 1.0f;
    } else {
        float y;
        uint32_t n = (uint32_t)(x*inv_pi);
        y = (x - ((float)M_PI * (float)n));
        z = k_sinf(y) / x;
        if (n&1) {
            z = -z;
        }
    }
    return z;
}

static inline float fast_cosf(float x) {
    float y = fabsf(x), z;
    uint32_t n = (uint32_t)(y*inv_pi);
    z = k_cosf(y - ((float)M_PI * (float)n));
    return ((n&1) ? -z : z);
}

// 1.0546365475f
float ngain = 0.101333437f;
float nxgain = 0.062659371644565f;

float dsd_gen_root_raised_cosine(float sampling_freq, float symbol_rate)
{
    float bps = symbol_rate/sampling_freq; // symbols/samples per bit
    float scale = 0.0f;
    int ntaps = 81, i;

    for(i = 0; i < ntaps; i++) {
	    float num;
	    float xindx = i - ntaps/2;
        float xindx2 = xindx * bps;
        float x1 = 0.64f*(1.5625f - xindx2*xindx2);

	    num = 0.8f*((float)M_1_PI*fast_cosf((float)M_PI * 1.2f*xindx2) + sinc((float)M_PI * 0.8f*xindx2));
        if (x1 == 0.0f) {
	        xcoeffs[i] = -0.2f;
        } else {
	        xcoeffs[i] = num / x1;
        }
	    scale += xcoeffs[i];
    }
    xcoeffs[ntaps] = xcoeffs[0];
    scale = (1.0f / scale);
    ngain = scale;
    return scale;
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

/*****************************************************************************
 Function: Arc tangent

 Syntax: angle = fast_atan2(y, x);
 float y y component of input vector
 float x x component of input vector
 float angle angle of vector (x, y) in radians

 Description: This function calculates the angle of the vector (x,y)
 based on a table lookup and linear interpolation. The table uses a
 256 point table covering -PI/4 to +PI/4 and uses symetry to
 determine the final angle value in the range of -PI to PI. 
 Note that this function uses the small angle approximation
 for values close to zero. This routine calculates the arc tangent
 with an average error of +/- 6.21e-7 radians.
*****************************************************************************/
static inline float fast_atanf(float z)
{
  float alpha, angle, base_angle, z_in = z;
  unsigned int index;

  if (z > 1.0f) {
    __asm__ volatile("rcpss %1, %0 \t\n" 
                    :"=x"(z) :"x"(z_in));
    z *= (2.0f-z*z_in);
    //z = 1.0f/z;
  }

  /* when ratio approaches the table resolution, the angle is */
  /* best approximated with the argument itself... */
  if(z < TAN_MAP_RES) {
    base_angle = z;
  } else {
    /* find index and interpolation value */
    alpha = z * (float)TAN_MAP_SIZE;
    index = ((unsigned int)alpha) & 0xff;
    alpha -= (float)index;
    /* determine base angle based on quadrant and */
    /* add or subtract table value from base angle based on quadrant */
    base_angle  =  fast_atan_table[index];
    base_angle += (fast_atan_table[index + 1] - fast_atan_table[index]) * alpha;
  }

  if(z_in < 1.0f) { /* -PI/4 -> PI/4 or 3*PI/4 -> 5*PI/4 */
    angle = base_angle; /* 0 -> PI/4, angle OK */
  }
  else { /* PI/4 -> 3*PI/4 or -3*PI/4 -> -PI/4 */
    angle = (float)M_PI*0.5f - base_angle; /* PI/4 -> PI/2, angle = PI/2 - angle */
  }

  return (angle);
}

static inline float fast_atan2f(float y, float x)
{
    float z;
    
    if ((y != y) || (x != x))
        return 0.0f;

    y += 1e-12f;
    if (fabsf(x) < 1e-12f) {
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

static inline float polar_disc_fast(FFTComplex a, FFTComplex b)
{
    float cr = a.re*b.re + a.im*b.im;
    float cj = a.im*b.re - a.re*b.im;
    return fast_atan2f(cj, cr);
}

static ssize_t 
readFromBuffer_s16(int in_fd, dsd_state *state)
{
    short tmpbuf[1024];
    unsigned int i;
    ssize_t result = read(in_fd, tmpbuf, 1024 * sizeof(short));
    state->inbuf_size = (result / sizeof(short));
    for (i = 0; i < state->inbuf_size; i++) {
        state->inbuffer[i] = ((float)tmpbuf[i] / 32768.0f);
    }
    return result;
}

static ssize_t 
readFromBuffer_f32(int in_fd, dsd_state *state)
{
    float tmpbuf[1024];
    unsigned int i;
    ssize_t result = read(in_fd, tmpbuf, 1024 * sizeof(float));
    state->inbuf_size = (result / sizeof(float));
    for (i = 0; i < state->inbuf_size; i++) {
        float f = tmpbuf[i];
        if (f != f) f = 0.0f;
        state->inbuffer[i] = f;
    }
    return result;
}

static ssize_t 
readFromBuffer_f32_iq(int in_fd, dsd_state *state)
{
    FFTComplex iqbuf[1024];
    unsigned int i;
    ssize_t result = read(in_fd, iqbuf, 1024 * sizeof(FFTComplex));
    state->inbuf_size = (result / sizeof(FFTComplex));
    state->inbuffer[0] = polar_disc_fast(iqbuf[0], state->prev);
    for (i = 1; i < state->inbuf_size; i++) {
        state->inbuffer[i] = polar_disc_fast(iqbuf[i], iqbuf[i-1]);
    }
    state->prev = iqbuf[state->inbuf_size - 1];
    return result;
}

float
dmr_filter(dsd_state *state, float sample)
{
  float sum = 0.0f;
  unsigned int i;

  for (i = 0; i < RRC_NZEROS; i++)
    state->xv[i] = state->xv[i+1];

  state->xv[RRC_NZEROS] = sample; // unfiltered sample in
  sum = scalarproduct_float(state->xv, xcoeffs, 84);
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
        if (state->inbuf_pos < state->inbuf_size) {
            sample = state->inbuffer[state->inbuf_pos++];
        } else {
            state->inbuf_pos = 1;
            if(opts->audio_in_format == 0) {
                result = readFromBuffer_s16(opts->audio_in_fd, state);
            } else if (opts->audio_in_format == 1) {
                result = readFromBuffer_f32(opts->audio_in_fd, state);
            } else if (opts->audio_in_format == 2) {
                result = readFromBuffer_f32_iq(opts->audio_in_fd, state);
            }
            if(result <= 0) {
                cleanupAndExit (opts, state);
            }
            sample = state->inbuffer[0];
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

