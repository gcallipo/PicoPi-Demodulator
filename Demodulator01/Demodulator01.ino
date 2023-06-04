/************************************************************* 
 * Demodulator.ino
 * Demodulator Multimode IQ (AM SSB CW) for Shorthwave Receiver
 * 
 * 
 * Created: Apr 2023
 * Author: Giuseppe Callipo - IK8YFW
 * https://github.com/ik8yfw
 * 
 * License: Creative Common with attribution 
 * 
 * Last update: 09/04/2023
 * 
 * 01.05.2023 test filters
 * 04.06.2023 refactoring Demodulator chain
 */

 

#include "DspProcessor.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void setup() {

#ifdef DEBUG_SERIAL 
  Serial.begin(115200);
#endif  

  // Keep SMPS PWM mode to reduce audio noise. (RPi Pico Dataseet p.18)
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  audioIO_setup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void loop(void)
{
  audioIO_loop();
}
