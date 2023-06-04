 /*
 * This is part of
 * Demodulator Multimode IQ (AM SSB CW) for Shorthwave Receiver
 * 
 * 
 * Created: 2023
 * Author: Giuseppe Callipo - IK8YFW
 * https://github.com/ik8yfw
 * 
*/

#ifndef AUDIOFILTER_H_
#define AUDIOFILTER_H_

/*
 * 
This Fitter hold the dynamic filter 
kernel and expose it as astandard Fir Filter.

*/

#define AUDIOFILTER_TAP_NUM 32

typedef struct {
  int history[AUDIOFILTER_TAP_NUM];
  unsigned int last_index;
} AUDIOFilter;

void AUDIOFilter_init(AUDIOFilter* f, const int TYPE, const int WINDOW, int lowCut, int highCut, float sampleFrequency);
void AUDIOFilter_put(AUDIOFilter* f, int input);
int AUDIOFilter_get(AUDIOFilter* f);

#endif
