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
#ifndef __DSPPROCESSOR_H__
#define __DSPPROCESSOR_H__

#ifdef __cplusplus
extern "C" {
#endif
 
#define DEBUG_SERIAL

#define AVG_BIAS_SHIFT 8u
#define DAC_RANGE  4096u
#define DAC_BIAS  (DAC_RANGE/2u)
#define ADC_RANGE  4096u
#define ADC_BIAS  (ADC_RANGE/2u)

#define ADC_BIAS_CORRECTION  20 

void audioIO_setup(void);
void audioIO_loop(void);

#ifdef __cplusplus
}
#endif
#endif
