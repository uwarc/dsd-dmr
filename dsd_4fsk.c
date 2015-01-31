#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include "dsd.h"
#include "fsk4_interp_coeffs.h"
#define FSK4_MAX(a,b) ((a) > (b) ? (a) : (b))
#define FSK4_MIN(a,b) ((a) > (b) ? (b) : (a))

// internal fast loop (must be this high to acquire symbol sync)
#define K_FINE_FREQUENCY 0.125f
//#define K_FINE_FREQUENCY 0.0625f
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
	
	// at this point we state that linear interpolation was tried
	// but found to be slightly inferior.  Using MMSE
	// interpolation shouldn't be a terrible burden
	
#if 0
	int imu = FSK4_MIN(lrintf((FSK4_NSTEPS * (state->d_symbol_clock / state->d_symbol_time))), FSK4_NSTEPS - 1);
#else
	int imu = lrintf(FSK4_NSTEPS * ((state->d_symbol_clock / state->d_symbol_time)));
	if (imu >= FSK4_NSTEPS) { 
	  imu = FSK4_NSTEPS - 1;
	}
#endif
	
#if 0
	float interp = 0.0;
	float interp_p1 = 0.0;
	for(i = 0, j = state->d_history_last; i < FSK4_NTAPS; ++i) {
	  interp += TAPS[imu][i] * state->d_history[j];
	  interp_p1 += TAPS[imu+1][i] * state->d_history[j];
	  j = (j + 1) % FSK4_NTAPS;
	}
#else
	unsigned int j = state->d_history_last;
	float interp = 0.0;
	float interp_p1 = 0.0;
	for(i=0; i<FSK4_NTAPS; i++) {
	    interp    +=  TAPS[imu  ][i] * state->d_history[j];
	    interp_p1 +=  TAPS[imu+1][i] * state->d_history[j];
	    j = (j+1) % FSK4_NTAPS;
    }
#endif
	
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

int
getSymbol (dsd_opts *opts, dsd_state *state, int have_sync)
{
    unsigned int i;
    float sample_out = 0.0f;
    ssize_t result = 0;

    // first we run through all provided data
    //for(i = 0; i < 10; i++) {
    for(i = 0; i < 4800; i++) {
        float sample = 0.0f;
        short tmp;

        // Read the new sample from the input
        if((opts->audio_in_type == 0) || (opts->audio_in_type == 1)) {
            result = read (opts->audio_in_fd, &tmp, 2);
            sample = (tmp / 16384.0f);
        } else if ((opts->audio_in_type == 4) || (opts->audio_in_type == 5)) {
            result = read (opts->audio_in_fd, &sample, 4);
            sample *= 2.0f;
        }
        sample *= (12000.0f / 648.0f);
  
        if(result <= 0) {
          cleanupAndExit (opts, state);
        }

        sample = dmr_filter(state, sample);
        if (fsk4_tracking_loop_mmse(state, sample, &sample_out)) {
            state->symbolcnt++;
            return lrintf(sample_out * 1024.0f);
        }
    }
    return 0;
}

