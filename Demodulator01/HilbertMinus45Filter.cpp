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
#include "HilbertMinus45Filter.h"

int filter_taps_HM[HM45FILTER_TAP_NUM];

//MINUS 45 @ 8k 33Taps Kaiser 290 - 3700 Hz
static double filter_dtaps[HM45FILTER_TAP_NUM]= {
 0.001321548797617529,
 0.000164884356798102,
 0.003862824121285384,
-0.000879941851242766,
 0.008374391029212183,
-0.004416499895952709,
 0.015062475229533436,
-0.012716877157178791,
 0.023544827831901021,
-0.029300165099413390,
 0.032767600324080964,
-0.061085737770807232,
 0.041169604096365019,
-0.130839079183537710,
 0.047077895563500623,
-0.443286526704272266,
-0.657655576218000304,
 0.443286526703795092,
 0.047077895563074748,
 0.130839079183563245,
 0.041169604096186030,
 0.061085737770860751,
 0.032767600323994131,
 0.029300165099460141,
 0.023544827831860522,
 0.012716877157211580,
 0.015062475229516027,
 0.004416499895973361,
 0.008374391029206545,
 0.000879941851253693,
 0.003862824121284522,
-0.000164884356793600,
 0.001321548797617784
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
