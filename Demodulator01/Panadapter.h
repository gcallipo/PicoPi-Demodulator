 /*
 * This is part of
 * Demodulator Multimode IQ (AM SSB CW) for Shorthwave Receiver
 * 
 * 
 * Created: 2024
 * Author: Giuseppe Callipo - IK8YFW
 * https://github.com/ik8yfw
 * 
*/
#ifndef __PANADAPTER_H__
#define __PANADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif

void setPan_mod(int mod);
void setPan_nr(int nr);
void update_loop(int I_IN, int Q_IN );
void panadapter_setup();

#ifdef __cplusplus
}
#endif
#endif
