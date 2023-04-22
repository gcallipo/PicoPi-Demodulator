#include "HilbertPlus45Filter.h"

int filter_taps_HP[HP45FILTER_TAP_NUM];

static double filter_dtaps [HP45FILTER_TAP_NUM]={
 0.001549061865473779,
-0.007221975536269554,
-0.003391693617580800,
-0.017953676832196033,
-0.029032497103910572,
-0.012220425449367340,
-0.080430014592734772,
 0.058847159433406050,
-0.136843454681960880,
 0.412191325615903270,
 0.543882825155810989,
-0.412191325505801731,
-0.136843454731516601,
-0.058847159360874403,
-0.080430014662815685,
 0.012220425474159465,
-0.029032497159967696,
 0.017953676826812631,
-0.003391693645084002,
 0.007221975525541078,
 0.001549061858757982
 };

void HP45Filter_init(HP45Filter* f) {
  int i;
  for(i = 0; i < HP45FILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;

  int acc =0; 
  for(i = 0; i < HP45FILTER_TAP_NUM; ++i) {
    acc = (int)(filter_dtaps[i] * 32768.0);
    filter_taps_HP[i] = acc;
  };
}

void HP45Filter_put(HP45Filter* f, int input) {
  f->history[f->last_index++] = input;
  if(f->last_index == HP45FILTER_TAP_NUM)
    f->last_index = 0;
}

int HP45Filter_get(HP45Filter* f) {
  long long acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < HP45FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : HP45FILTER_TAP_NUM-1;
    acc += (long long)f->history[index] * filter_taps_HP[i];
  };
  return acc >> 16;
}
