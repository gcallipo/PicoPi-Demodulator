/************************************************************* 
 * Demodulator01.ino
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
 * 27.08.2024 Creare version 2.0 
 * 27.08.2024 solve some issue and add panadapter 
 * 05.10.2024 refactoring new version
 * 
 * 
 * 
 * NOTE:  The panadapter needs an Oled display , and the following libraies
 * 
 * OLED 128x64 Bandscope Waterfall 
 * Library to add
 * arduinoFFT.h    https://www.arduino.cc/reference/en/libraries/arduinofft/
 * Wire.h
 * U8g2lib.h       https://github.com/olikraus/U8g2_Arduino
 * 
 * Credits: Thenk you to JR3XNW for the code related the band scope waterfall.
 * 
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
