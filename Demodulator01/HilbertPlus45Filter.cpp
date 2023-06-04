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
#include "HilbertPlus45Filter.h"

int filter_taps_HP[HP45FILTER_TAP_NUM];

//PLUS 45 @ 8k 33Taps Kaiser 290 - 3700 Hz
static double filter_dtaps [HP45FILTER_TAP_NUM]={
 -0.001321548797620119,
 0.000164884356797782,
-0.003862824121293162,
-0.000879941851241083,
-0.008374391029228734,
-0.004416499895943777,
-0.015062475229563017,
-0.012716877157152836,
-0.023544827831948719,
-0.029300165099355006,
-0.032767600324147252,
-0.061085737770685086,
-0.041169604096446884,
-0.130839079183274420,
-0.047077895563595013,
-0.443286526703382033,
 0.657655576219318805,
 0.443286526702905415,
-0.047077895563169228,
 0.130839079183301010,
-0.041169604096269477,
 0.061085737770737426,
-0.032767600324059967,
 0.029300165099401497,
-0.023544827831907238,
 0.012716877157186930,
-0.015062475229546438,
 0.004416499895964453,
-0.008374391029223509,
 0.000879941851251772,
-0.003862824121292272,
-0.000164884356793235,
-0.001321548797620479
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
