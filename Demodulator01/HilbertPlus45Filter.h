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

#ifndef HP45FILTER_H_
#define HP45FILTER_H_

#define HP45FILTER_TAP_NUM 33

typedef struct {
  int history[HP45FILTER_TAP_NUM];
  unsigned int last_index;
} HP45Filter;

void HP45Filter_init(HP45Filter* f);
void HP45Filter_put(HP45Filter* f, int input);
int HP45Filter_get(HP45Filter* f);

#endif
