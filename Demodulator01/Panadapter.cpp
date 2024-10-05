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

#include "Arduino.h"
#include "Panadapter.h"


/*
  OLED 128x64 Bandscope Waterfall JR3XNW 2023/6/10
  Library to add
  arduinoFFT.h    https://www.arduino.cc/reference/en/libraries/arduinofft/
  Wire.h
  U8g2lib.h       https://github.com/olikraus/U8g2_Arduino
*/

#include <U8g2lib.h>  // This is for controlling the OLED display
#include <Wire.h>  // This library is needed for I2C communication, which is used by the OLED display
#include <arduinoFFT.h> // v2.0.2

//////////////////////////////////////////
//   Global Variables and Definitions   //
//////////////////////////////////////////
#define PX1 63  // The X coordinate where the display starts
#define PY1 23  // The Y coordinate where the spectrum display ends
#define PY2 56  // The Y coordinate where the waterfall display ends
#define SAMPLES 256  // The number of samples to take for the FFT. This should be a power of 2.
#define WFrow 12  // Number of rows in the waterfall display
#define SCREEN_WIDTH 128  // Width of the OLED display
#define SCREEN_HEIGHT 32  // Height of the OLED display
#define THRESHOLD 5  // Threshold for the waterfall display
#define HISTORY_LENGTH SCREEN_HEIGHT  // The number of previous FFT results to keep for the waterfall display

double vReal[SAMPLES];  // Array to hold the real part of the complex numbers for FFT
double vImag[SAMPLES];  // Array to hold the imaginary part of the complex numbers for FFT

double history[HISTORY_LENGTH][SAMPLES];  // Array to hold the history of FFT results
int historyIndex = 0;  // Index of the current FFT result in the history

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  // Object for controlling the OLED display
ArduinoFFT<double> FFT;  // v2.0.2 Explicit data types using templates


////////////////////////////
//      Graph Length      //
////////////////////////////
int barLength(double d) {  // Calculate the length of the graph
  float fy;
  int y;

  fy = 14.0 * (log10(d) + 3.3);  //3.3 14
  y = fy;
  y = constrain(y, 0, 22);  // Cut off upper and lower limits
  
  return y;
}

//////////////////////////////
//    Band scope display    //
//////////////////////////////
int mod=0;
void setPan_mod(int mymod){
  mod=mymod;
}

int nr=0;
void setPan_nr(int mynr){
  nr=mynr;
}

void showScope() {  // Spectrum Display
  int d, d1, d2;
  
  for (int xi = 1; xi < 64; xi++) {  // Positive frequency spectrum display
      d1 = barLength(vReal[xi*2]);
      d2 = barLength(vImag[xi*2+1]);
    d = sqrt(d1 * d1 + d2 * d2);
    u8g2.drawVLine(xi + 64 , PY1 - d, d);  
  }
  for (int xi = 64; xi < 128; xi++) { // Negative frequency spectrum display
      d1 = barLength(vReal[xi*2]);
      d2 = barLength(vImag[xi*2+1]);
    d = sqrt(d1 * d1 + d2 * d2);
    u8g2.drawVLine(xi - 64 , PY1 - d, d); 
  }
}

///////////////////////////////
//     LSB/USB/CW/AM switching     //
///////////////////////////////
void show_mod() {  // Spectrum Display
  u8g2.setFont(u8g2_font_6x10_tf);
 // u8g2.setFont(u8g2_font_micro_tr);  // Small font(3x5)
  if ( mod == 0 ){
    u8g2.drawStr(0, 0, "LSB ");       //
  } else  if ( mod == 1 ){ 
    u8g2.drawStr(0, 0, "USB ");       //
  } else  if ( mod == 2 ){ 
    u8g2.drawStr(0, 0, "CW  ");       //
  } else if ( mod == 3 ){ 
    u8g2.drawStr(0, 0, "AM  ");       //
  }
}



///////////////////////////////
//     LSB/USB/CW/AM switching     //
///////////////////////////////
void show_nr() {  // Spectrum Display
  u8g2.setFont(u8g2_font_6x10_tf);
 // u8g2.setFont(u8g2_font_micro_tr);  // Small font(3x5)
  if ( nr == 0 ){
    u8g2.drawStr(68, 0, "DNR OFF ");       //
  } else  if ( nr == 1 ){ 
    u8g2.drawStr(68, 0, "DNR ON 1");       //
  } else  if ( nr == 2 ){ 
    u8g2.drawStr(68, 0, "DNR ON 2");       //
  } else if ( nr == 3 ){ 
    u8g2.drawStr(68, 0, "DNR ON 3");       //
  }
}

//////////////////////////////
//        Water fall        //
//////////////////////////////
void displayWaterfall() {
  // Calculate the magnitudes of the FFT results
  for (int xi = 0; xi < SAMPLES; xi++) {
    double d1 = barLength(vReal[xi]);
    double d2 = barLength(vImag[xi]);
    double d = sqrt(d1 * d1 + d2 * d2);
    // Store the FFT results into the history array
    if(xi < SAMPLES/2){
      history[historyIndex][xi + SAMPLES / 2] = d; 
    }else{
      history[historyIndex][xi - SAMPLES / 2] = d;
    }
  }
  
  // Increment the history index, looping back to 0 if it reaches the end of the array
  historyIndex = (historyIndex + 1) % HISTORY_LENGTH;

  // Draw the history
  for (int y = 0; y < HISTORY_LENGTH; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      // Skip drawing the pixel at the center of the screen (0 Hz frequency)
      if (x == SCREEN_WIDTH / 2) continue;
      // Retrieve the FFT magnitude from the history
      double magnitude = history[(historyIndex + y) % HISTORY_LENGTH][x * (SAMPLES / SCREEN_WIDTH)];
      // If the magnitude is above the threshold, draw a pixel
      if (magnitude > THRESHOLD) {
        u8g2.drawPixel(x, (SCREEN_HEIGHT + 23) - y - 1);
      }
    }
  }
}

////////////////////////////////
//      on-screen display     //
////////////////////////////////
void showGraphics() {  // Modifying Graphs
  // area demarcation line
  u8g2.drawHLine(0, 23, 128);  // Lower end line for waterfall 55
  u8g2.drawHLine(0, 55, 128);  // Lower end line for waterfall 55

  // Frequency scale (horizontal axis)
  u8g2.drawBox(PX1 - 24, PY2, 2, 2);  // Positive Frequency 10kscale
  u8g2.drawBox(PX1 - 46, PY2, 2, 2);  // Positive Frequency 20kscale

  u8g2.drawBox(PX1, PY2, 2, 2);       // Negative frequency 0kscale
  u8g2.drawBox(PX1 + 22, PY2, 2, 2);  // Negative frequency 10kscale
  u8g2.drawBox(PX1 + 45, PY2, 2, 2);  // Negative frequency 20kscale

  u8g2.setFont(u8g2_font_micro_tr);  // Small font(3x5)
  u8g2.drawStr(9, 58, "-5k");       // -20k Negative frequency display 58 26
  u8g2.drawStr(32, 58, "-2k");      // -10k

  u8g2.drawStr(63, 58, "0");  // Positive Frequency
  u8g2.drawStr(81, 58, "2k");
  u8g2.drawStr(105, 58, "5k");
  
}

////////////////////////////
//          setup          //
////////////////////////////
void panadapter_setup() {
  //pinMode(25, OUTPUT);      // pico built-in LED
  //pinMode(MOD_BUTTON,INPUT_PULLUP); //MODE_SWITCHING Set to input and pull-up
  //Serial.begin(115200);

  //analogReadResolution(12);  // Set ADC full scale to 12 bits
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();  // The upper left corner is used as the character position reference.
  u8g2.clearBuffer();
  u8g2.drawStr(20, 0, "Demodulator SDR");
  u8g2.drawStr(25, 10,"    IK8YFW");
  u8g2.drawStr(26, 20,"     2024");
  u8g2.sendBuffer();
  sleep_ms(500); 

}

//////////////////////////////
//           loop           //
//////////////////////////////
static int iSamples = 0;
static int iReadSemaphore = 0;

void update_loop(int I_IN, int Q_IN ) {

    /*SAMPLING*/
    vReal[iSamples] = (I_IN - 2048) * 3.3 / 4096.0;  //Arduinoは「0」。
    vImag[iSamples] = (Q_IN - 2048) * 3.3 / 4096.0;  //
    
    // increase ...
    iSamples ++;
    if (iSamples<SAMPLES) {
      // continue to accumulate new samples ...
     // iReadSemaphore = 0;
      return;
    }else{
      iSamples =0;
      // update the screen ...
    }
    
    /*FFT*/
    FFT.windowing(vReal, SAMPLES, FFTWindow::Hamming, FFTDirection::Forward);  // Use new enum types instead of FFT_WIN_TYP_HAMMING and FFT_FORWARD
    FFT.windowing(vImag, SAMPLES, FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(vReal, vImag, SAMPLES, FFTDirection::Reverse);  // Change FFT_REVERSE to FFTDirection::Reverse
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);  // No change
    
    u8g2.clearBuffer();  // Screen buffer clear
    showScope();  // Spectrum Display
    displayWaterfall();
    showGraphics();  // Scale line and other indications
    show_mod();  //mode
    show_nr();  //nr
    u8g2.sendBuffer();

   sleep_ms(2);
    
}
