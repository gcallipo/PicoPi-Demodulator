#ifndef AUDIOFILTER_H_
#define AUDIOFILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 8000 Hz

fixed point precision: 16 bits

* 0 Hz - 2800 Hz
  gain = 1
  desired ripple = 2 dB
  actual ripple = n/a

* 3200 Hz - 4000 Hz
  gain = 0
  desired attenuation = -60 dB
  actual attenuation = n/a

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
