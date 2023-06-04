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

#ifndef HM45FILTER_H_
#define HM45FILTER_H_

#define HM45FILTER_TAP_NUM 33

typedef struct {
  int history[HM45FILTER_TAP_NUM];
  unsigned int last_index;
} HM45Filter;

void HM45Filter_init(HM45Filter* f);
void HM45Filter_put(HM45Filter* f, int input);
int HM45Filter_get(HM45Filter* f);

#endif
