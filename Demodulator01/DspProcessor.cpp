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
#include "DspProcessor.h"

#include "AUDIOFilter.h"
#include "AVGFilter.h"
#include "Dec8KFilter.h"
#include "HilbertPlus45Filter.h"
#include "HilbertMinus45Filter.h"

#include "pico/multicore.h"
#include "dynamicFilters.h"

#include <I2S.h>
#include <ADCInput.h>


/**************************************************************************************
 * Some macro's
 * See Alpha Max plus Beta Min algorithm for MAG (vector length)
 **************************************************************************************/
#define ABS(x)    ((x)<0?-(x):(x))
#define MAG(i,q)  (ABS(i)>ABS(q) ? ABS(i)+((3*ABS(q))>>3) : ABS(q)+((3*ABS(i))>>3))

// GPIO I2S pin numbers
// TO connect the PCM5102A DAC
#define I2S_BCLK 15
#define I2S_WS (I2S_BCLK+1)
#define I2S_DOUT 17
const int sampleRate = 8000;

// Create the I2S port using a PIO state machine
I2S i2s(OUTPUT);

// Create input on Adc 0 - GPIO26 & Adc 1 - GPIO27
ADCInput adcIn(A0,A1);

// controll button to select filter
#define PIN_BUTTON_FL 14
// controll button to select nr
#define PIN_BUTTON_NR 13
// control to se the audio gain
#define PIN_BUTTON_AUDIO_GAIN 11

// Over range onboard led
// #define LED_PIN 25
// Over range external led
#define LED_PIN 12

// define the maximum safe signal in input
// the led blink at 1/10 of maximum input level
// this assures no distorsion or overload.
// the 12 bit range ADC is -2048 to +2047 (+- 1.5 V, 3V pp)
// The over range bling at about 150 mV of input peak signal
// (300 mV pp) this is a good safe guard for 
// the Pico ADC.
#define OVER_RANGE 150

// define min and max gain for output amplification
#define MIN_GAIN   30   // suitable for AM
#define MAX_GAIN   100  // suitable for NARROW MODES

// globals
volatile uint8_t     decimator_ct = 0,over_on =1;
volatile uint8_t     decimator_factor = 2;
volatile int16_t     avg, sum, out_sample = 0;
volatile int32_t adc_result_bias_i = (ADC_BIAS << AVG_BIAS_SHIFT);
volatile int32_t adc_result_bias_q = (ADC_BIAS << AVG_BIAS_SHIFT);

AUDIOFilter   flt0;   // SSB/CW filter
AVGFilter     flt2;   // AVG DNR  filter
AVGFilter     flt3;   // AVG DNR  filter
 
HP45Filter    fltHP; // Hilbert Plus 45
HM45Filter    fltHM; // Hilbert Minus 45

AUDIOFilter   fltDec_I1,fltDec_Q1; // Input filter for AM Envelope

Dec8KFilter   fltDec_I,fltDec_Q;   // Input decimatore for Narrow
Dec8KFilter   fltInt;  // Interpolatore filter         

int           passInput = 0;
uint8_t       filterMode = 0;
uint8_t       demodMode = 0;
uint8_t       nrMode = 0;
int16_t       outSample = 0;
int16_t       outSample_8k = 0;
int16_t       gainAudio = 0;
int16_t       gainFilter = 0;
int16_t       gainDec = 0;


// agc
int16_t AGC_TOP    = 150;
volatile uint32_t s_rssi;
volatile int32_t rx_agc = 1;


// At the moment no AGC, then we set fixed gain for modes
void setMaxGAIN(){
  AGC_TOP    = 150;
  gainAudio = MAX_GAIN;
  gainDec=2;
  gainFilter = 10; 
}

// Set minimum Gain for AM Recetion
void setMinGAIN(){
  AGC_TOP    = 150;
  gainAudio = MIN_GAIN;
  gainDec=2;
  gainFilter = 5;
}   
// continuous loop running for audio processing
void audioIO_loop(void)
{
  int16_t newSample_I = 0;
  int16_t newSample_Q = 0;
  int16_t outSample1= 0;
  int16_t outSample2= 0;
  int16_t rawSample_I = 0;
  int16_t rawSample_Q = 0;

  // For debug only
#ifdef DEBUG_SERIAL
  int  nn = micros();
#endif


#ifdef DEBUG_SERIAL
  //char buffer[40];
  //sprintf(buffer, "Value 0 %d ", cap_buf[0]);
  //Serial.println(buffer);
#endif
 while(1){
  while (adcIn.available() > 0) {

    // Get fresh samples 
    newSample_I = adcIn.read();  
    newSample_Q = adcIn.read();
   
    // Remove ADC bias DC component from samples with long average
    adc_result_bias_i += (int16_t)(newSample_I - (adc_result_bias_i >> AVG_BIAS_SHIFT));
    adc_result_bias_q += (int16_t)(newSample_Q - (adc_result_bias_q >> AVG_BIAS_SHIFT));
    newSample_I -= (adc_result_bias_i>>AVG_BIAS_SHIFT);
    newSample_Q -= (adc_result_bias_q>>AVG_BIAS_SHIFT); 

    /* Blink the builtin LED if the input signal go over range */
    if (over_on==1){   
      if (newSample_I > OVER_RANGE) {
        gpio_put(LED_PIN, 1);
      } else {
        gpio_put(LED_PIN, 0);
      }
    }

    // Approximate amplitude, with alpha max + beta min function
    uint32_t temp =  MAG(newSample_I,newSample_Q);
    s_rssi = MAX(1,temp);
    rx_agc = AGC_TOP/s_rssi;   // calculate scaling factor
    if (rx_agc==0) rx_agc=1;

    newSample_I = newSample_I * gainAudio*rx_agc;
    newSample_Q = newSample_Q * gainAudio*rx_agc;

    // Cut at 8KHz for future decimation and low noise
     Dec8KFilter_put(&fltDec_I, newSample_I);
     newSample_I = Dec8KFilter_get(&fltDec_I)*gainDec;
        
     Dec8KFilter_put(&fltDec_Q, newSample_Q);
     newSample_Q = Dec8KFilter_get(&fltDec_Q)*gainDec;

     // apply the NR I channel.
     AVGFilter_put(&flt2, newSample_I);
     newSample_I = AVGFilter_get(&flt2);

     // apply the NR Q channel.
     AVGFilter_put(&flt3, newSample_Q);
     newSample_Q = AVGFilter_get(&flt3);

     // For Narrow Modes (SSB CW)
     if (demodMode != 3) { 

        // decimate by factor = 2 and compute at 8Ksps
        decimator_ct ++;
        if ( decimator_ct >= decimator_factor ) {
          decimator_ct = 0;

            // Apply Hilbert
            HP45Filter_put(&fltHP, newSample_I);
            newSample_I = HP45Filter_get(&fltHP);

            // Apply Hilbert
            HM45Filter_put(&fltHM, newSample_Q);
            newSample_Q = HM45Filter_get(&fltHM);
           
            // Demodulate 
            if (demodMode == 0) {   // LSB
               outSample_8k = newSample_I-newSample_Q;
            }
            else
            if (demodMode == 1) {   // USB
                outSample_8k = newSample_I+newSample_Q;
            }
            else
            if (demodMode == 2) {   // CW
                outSample_8k = newSample_I+newSample_Q;
            }
                  
            // Apply the main output filter
            AUDIOFilter_put(&flt0, outSample_8k);
            outSample = AUDIOFilter_get(&flt0); 
        }
    }
    else // ( demodMode == 3 )
    {    // AM  (WIDE MODE)

        AUDIOFilter_put(&fltDec_I1, newSample_I);
        newSample_I = AUDIOFilter_get(&fltDec_I1)*gainDec;
        
        AUDIOFilter_put(&fltDec_Q1, newSample_Q);
        newSample_Q = AUDIOFilter_get(&fltDec_Q1)*gainDec;

        // this is the envelope
        outSample = MAG(newSample_I,newSample_Q);
        //outSample = sqrt((newSample_I*newSample_I) + (newSample_Q*newSample_Q));
    }

  
    // Post decimation (interpolation) filter
    Dec8KFilter_put(&fltInt, outSample);
    outSample = Dec8KFilter_get(&fltInt);

    // A small gain can helps
    outSample2= outSample * gainFilter;

    // write the same sample twice, once for left and once for the right channel
    if (i2s.availableForWrite()>0){
      i2s.write(outSample2);
      i2s.write(outSample2);
    }
   }

  };
}


uint8_t val, old_val = HIGH;
uint8_t val1, old_val1 = HIGH;

// check commands on core 1
void core1_commands_check() {
  int max_modes =3;

  // run on loop
  while (1) {
    
    //if the push gain is selected the buttons will allow to change it
    int pushGain = digitalRead(PIN_BUTTON_AUDIO_GAIN);
      
    // debounche
    val = digitalRead(PIN_BUTTON_FL);
    if (val == LOW) { gpio_put(LED_PIN, 1); over_on=0; }
    if (val != old_val) {
      old_val = val;

      if (val == LOW) {
   
        if (pushGain == LOW){
          if (demodMode==3){
           gainAudio +=5;
          }else{
            gainAudio +=5;
          }
        }else{

            // Roll the filter selection
            if (demodMode == max_modes)
              demodMode = 0;
            else
              demodMode++;
    
            // Demodulator
            if (demodMode == 0 || demodMode == 1){ // SSB
              setMaxGAIN();
              AUDIOFilter_init(&flt0, ID_BANDPASS, W_HAMMING, 200, 2700, sampleRate);
            }
            else if (demodMode == 2){ // CW 500hz
              setMaxGAIN();
              AUDIOFilter_init(&flt0, ID_BANDPASS, W_BLACKMAN, 400, 1000, sampleRate);
              // additional gain
              gainFilter+=5;
            }
            else if (demodMode == 3){ // AM
              setMinGAIN();
            }
          }
         
      }
    }

    // debounche
    val1 = digitalRead(PIN_BUTTON_NR);
    if (val1 == LOW) { gpio_put(LED_PIN, 1); over_on=0; }
    if (val1 != old_val1) {
      old_val1 = val1;

      if (val1 == LOW) {
         if (pushGain == LOW){
           if (demodMode==3){
            gainAudio =gainAudio>0?gainAudio-10:0;
          }else{
            gainAudio =gainAudio>0?gainAudio-10:0;
          }
        }else{

            // Roll the filter selection
            if (nrMode == 3)
              nrMode = 0;
            else
              nrMode++;
    
            // Noise Reduction stage
            if (nrMode == 0) {
              AVGFilter_init(&flt2, 3);
              AVGFilter_init(&flt3, 3);
            } else if (nrMode == 1) {
              AVGFilter_init(&flt2, 6);
              AVGFilter_init(&flt3, 6);
            } else if (nrMode == 2) {
              AVGFilter_init(&flt2, 10);
              AVGFilter_init(&flt3, 10);
            } else if (nrMode == 3) {
              AVGFilter_init(&flt2, 15);
              AVGFilter_init(&flt3, 15);
            }
        }
      }
    }

    sleep_ms(500);
    gpio_put(LED_PIN, 0);
    over_on=1;
  }
}


// general setup
void audioIO_setup() {

  // Init all Filters
  // Narrow Modes filters
  AUDIOFilter_init(&flt0, ID_BANDPASS, W_BLACKMAN, 200.0, 3000.0, sampleRate);

  // Averanges DNR Filters
  AVGFilter_init(&flt2, 2);
  AVGFilter_init(&flt3, 2);

  // IN/OUT filters ( Decimation and interpolation ) 
  Dec8KFilter_init(&fltInt);
  Dec8KFilter_init(&fltDec_I);
  Dec8KFilter_init(&fltDec_Q);

  // AM FRONTEND FILTERS
  AUDIOFilter_init(&fltDec_I1, ID_BANDPASS, W_HAMMING, 300.0, 4500.0, sampleRate*2);
  AUDIOFilter_init(&fltDec_Q1, ID_BANDPASS, W_HAMMING, 300.0, 4500.0, sampleRate*2);

  // HILBERT FILTERS
  HP45Filter_init(&fltHP);
  HM45Filter_init(&fltHM);

  gpio_init_mask(1 << LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0);

  i2s.setBCLK(I2S_BCLK);
  i2s.setDATA(I2S_DOUT);
  i2s.setBitsPerSample(16);

  // start I2S at the sample rate with 16-bits per sample
  // The with 2 channels the input sample rate will be freq/2
  // then we must set the i2s freq as Number Chennels * ADC freq.
  if (!i2s.begin(sampleRate*2)) {

#ifdef DEBUG_SERIAL
    Serial.println("Failed to initialize I2S!");
#endif
    while (1); // do nothing
  }

  adcIn.setBuffers(4, 32);

  if (!adcIn.begin(sampleRate)) {
    //Serial.println("Failed to initialize ADCInput!");
    while (1); // do nothing
  }

  // start controller commands :
  pinMode(PIN_BUTTON_FL, INPUT_PULLUP);
  pinMode(PIN_BUTTON_NR, INPUT_PULLUP);
  pinMode(PIN_BUTTON_AUDIO_GAIN, INPUT_PULLUP);

  // set the maximum audio gain
  setMaxGAIN();

  // pushbutton to select the filter on core 1
  multicore_launch_core1(core1_commands_check);
  sleep_ms(2000);

  // start DSP processor
  audioIO_loop();

}
