#include "AUDIOFilter.h"
#include "dynamicFilters.h"

int filter_taps[AUDIOFILTER_TAP_NUM];

void AUDIOFilter_init(AUDIOFilter* f, const int TYPE, const int WINDOW, int lowCut, int highCut, float sampleFrequency) {
  int i;
  double filter_taps_dub[AUDIOFILTER_TAP_NUM];
  audioFilter(filter_taps_dub, AUDIOFILTER_TAP_NUM, TYPE, WINDOW, lowCut, highCut, sampleFrequency);
  convertCoeffToInt16(filter_taps_dub, filter_taps, AUDIOFILTER_TAP_NUM);

  for(i = 0; i < AUDIOFILTER_TAP_NUM; ++i){
    f->history[i] = 0;
  }
  f->last_index = 0;
}

void AUDIOFilter_put(AUDIOFilter* f, int input) {
  f->history[f->last_index++] = input;
  if(f->last_index == AUDIOFILTER_TAP_NUM)
    f->last_index = 0;
}

int AUDIOFilter_get(AUDIOFilter* f) {
  long long acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < AUDIOFILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : AUDIOFILTER_TAP_NUM-1;
    acc += (long long)f->history[index] * filter_taps[i];
  };
  return acc >> 16;
}
