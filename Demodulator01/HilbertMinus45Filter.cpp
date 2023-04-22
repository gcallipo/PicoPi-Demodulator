#include "HilbertMinus45Filter.h"

int filter_taps_HM[HM45FILTER_TAP_NUM];

static double filter_dtaps[HM45FILTER_TAP_NUM]= {
 0.001549061858766211,
 0.007221975525526658,
-0.003391693645071686,
 0.017953676826800144,
-0.029032497160002911,
 0.012220425474219235,
-0.080430014663020452,
-0.058847159360611551,
-0.136843454732120839,
-0.412191325505263939,
 0.543882825156903893,
 0.412191325614789772,
-0.136843454681906257,
 0.058847159433432821,
-0.080430014592853233,
-0.012220425449258960,
-0.029032497103992451,
-0.017953676832136379,
-0.003391693617606767,
-0.007221975536254936,
 0.001549061865471842
 };

void HM45Filter_init(HM45Filter* f) {
  int i;
  for(i = 0; i < HM45FILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;

  int acc =0; 
  for(i = 0; i < HM45FILTER_TAP_NUM; ++i) {
    acc = (int)(filter_dtaps[i] * 32768.0);
    filter_taps_HM[i] = acc;
  };
}

void HM45Filter_put(HM45Filter* f, int input) {
  f->history[f->last_index++] = input;
  if(f->last_index == HM45FILTER_TAP_NUM)
    f->last_index = 0;
}

int HM45Filter_get(HM45Filter* f) {
  long long acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < HM45FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : HM45FILTER_TAP_NUM-1;
    acc += (long long)f->history[index] * filter_taps_HM[i];
  };
  return acc >> 16;
}
