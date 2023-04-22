
/*************************************************************
   
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
// TO connect the MAX98357A power Amplifier
#define I2S_BCLK 15
#define I2S_WS (I2S_BCLK+1)
#define I2S_DOUT 17
const int sampleRate = 8000;

// Create the I2S port using a PIO state machine
I2S i2s(OUTPUT);

// Create input on Adc 0 - GPIO26 & Adc 1 - GPIO27
//ADCInput adcIn(26,27);
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
#define OVER_RANGE 200

// define min and max gain for output amplification
#define MIN_GAIN   20   // suitable for headphone
#define MAX_GAIN   100  // suitable for speaker

// globals
volatile uint8_t     decimator_ct = 0;
volatile uint8_t     decimator_factor = 2;
volatile int16_t     avg, sum, out_sample = 0;

AUDIOFilter   flt0;   // AM/SSB filter
AVGFilter     flt2;   // AVG  filter

HP45Filter    fltHP;
HM45Filter    fltHM;

Dec8KFilter   fltDec_I;
Dec8KFilter   fltDec_Q;
Dec8KFilter   fltDec;

int           passInput = 0;
uint8_t       filterMode = 0;
uint8_t       demodMode = 0;
uint8_t       nrMode = 0;
int16_t       outSample = 0;
int16_t       outSample_8k = 0;
int16_t       gainAudio = MIN_GAIN;
int16_t       gainFilter = 2;
int16_t       gainDec = 2;

// Check if need to boost the audio
// For safe reasons the value will be
// only at startup.
void initAudioGain(void) {

  // set the default audioGain for headphones
  gainAudio = MIN_GAIN;
  if (digitalRead(PIN_BUTTON_AUDIO_GAIN) == LOW) {
   
    gainAudio = MAX_GAIN;
    gainFilter = 20;
    gainDec = 6;
  }

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

    newSample_I = adcIn.read();
    newSample_I = newSample_I - ADC_BIAS - ADC_BIAS_SHIFT;

    newSample_Q = adcIn.read();
    newSample_Q = newSample_Q - ADC_BIAS - ADC_BIAS_SHIFT;


    /* Blink the builtin LED if the input signal go over range */
    if (newSample_I > OVER_RANGE) {
      gpio_put(LED_PIN, 1);
    } else {
      gpio_put(LED_PIN, 0);
    }
    newSample_I = newSample_I * gainAudio;
    newSample_Q = newSample_Q * gainAudio;

    Dec8KFilter_put(&fltDec_I, newSample_I);
    newSample_I = Dec8KFilter_get(&fltDec_I)*gainDec;
        
    Dec8KFilter_put(&fltDec_Q, newSample_Q);
    newSample_Q = Dec8KFilter_get(&fltDec_Q)*gainDec;

    rawSample_I = newSample_I;
    rawSample_Q = newSample_Q;

    
    if (demodMode != 3) { 
        decimator_ct ++;
        if ( decimator_ct >= decimator_factor ) {
          decimator_ct = 0;
    
            HP45Filter_put(&fltHP, newSample_I);
            newSample_I = HP45Filter_get(&fltHP);
        
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
          
        
            // apply the NR.
            AVGFilter_put(&flt2, outSample_8k);
            outSample_8k = AVGFilter_get(&flt2);
        
            // Apply the main output filter
            AUDIOFilter_put(&flt0, outSample_8k);
            outSample = AUDIOFilter_get(&flt0); 
    
        }
    }
    else
    {   // AM
        outSample = MAG(rawSample_I,rawSample_Q);
    }

    Dec8KFilter_put(&fltDec, outSample);
    outSample = Dec8KFilter_get(&fltDec);
  
    outSample2= outSample * gainFilter;

    // write the same sample twice, once for left and once for the right channel
    if (i2s.availableForWrite()>0){
      i2s.write(outSample2);
      i2s.write(outSample2);
    }
   }

  };

}


   
// continuous loop running for audio processing
void audioIO_loop_ok(void)
{
  int16_t newSample_I = 0;
  int16_t newSample_Q = 0;
  int16_t outSample1= 0;
  int16_t outSample2= 0;

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

    newSample_I = adcIn.read();
    newSample_I = newSample_I - ADC_BIAS - ADC_BIAS_SHIFT;

    newSample_Q = adcIn.read();
    newSample_Q = newSample_Q - ADC_BIAS - ADC_BIAS_SHIFT;


    /* Blink the builtin LED if the input signal go over range */
    if (newSample_I > OVER_RANGE) {
      gpio_put(LED_PIN, 1);
    } else {
      gpio_put(LED_PIN, 0);
    }
    newSample_I = newSample_I * gainAudio;
    newSample_Q = newSample_Q * gainAudio;

    HP45Filter_put(&fltHP, newSample_I);
    newSample_I = HP45Filter_get(&fltHP);

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
    else
    if (demodMode == 3) {   // AM
        outSample_8k = MAG(newSample_I,newSample_Q);
    }

    // apply the NR.
    AVGFilter_put(&flt2, outSample_8k);
    outSample_8k = AVGFilter_get(&flt2);
   
    // Apply the main output filter
    AUDIOFilter_put(&flt0, outSample_8k);
    outSample = AUDIOFilter_get(&flt0);

    Dec8KFilter_put(&fltDec, outSample);
    outSample = Dec8KFilter_get(&fltDec);
  
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
  if (gainAudio == MAX_GAIN){
    max_modes =2; // skip AM
  }

  // run on loop
  while (1) {

    // debounche
    val = digitalRead(PIN_BUTTON_FL);
    if (val != old_val) {
      old_val = val;

      if (val == LOW) {

        // Roll the filter selection
        if (demodMode == max_modes)
          demodMode = 0;
        else
          demodMode++;

        // Demodulator
        if (demodMode == 0 || demodMode == 1){ // SSB
          AUDIOFilter_init(&flt0, ID_BANDPASS, W_HAMMING, 200, 3000, sampleRate);
          //gainFilter=10;
        }
        else if (demodMode == 2){ // CW 500hz
          AUDIOFilter_init(&flt0, ID_BANDPASS, W_BLACKMAN, 450, 950, sampleRate);
          //gainFilter=10;
        }
        else if (demodMode == 3){ // AM
          //AUDIOFilter_init(&flt0, ID_LOWPASS, W_HAMMING, 4000, 0, sampleRate);
          //gainFilter=10;
        }
      }
    }

    // debounche
    val1 = digitalRead(PIN_BUTTON_NR);
    if (val1 != old_val1) {
      old_val1 = val1;

      if (val1 == LOW) {

        // Roll the filter selection
        if (nrMode == 3)
          nrMode = 0;
        else
          nrMode++;

        // Noise Reduction stage
        if (nrMode == 0) {
          AVGFilter_init(&flt2, 4);
        } else if (nrMode == 1) {
          AVGFilter_init(&flt2, 7);
        } else if (nrMode == 2) {
          AVGFilter_init(&flt2, 10);
        } else if (nrMode == 3) {
          AVGFilter_init(&flt2, 15);
        }

      }
    }

    sleep_ms(500);
  }
}


// general setup
void audioIO_setup() {

  AUDIOFilter_init(&flt0, ID_BANDPASS, W_BLACKMAN, 200.0, 3000.0, sampleRate);
  AVGFilter_init(&flt2, 4);
  Dec8KFilter_init(&fltDec);
  Dec8KFilter_init(&fltDec_I);
  Dec8KFilter_init(&fltDec_Q);
  
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
  initAudioGain();

  // pushbutton to select the filter on core 1
  multicore_launch_core1(core1_commands_check);
  sleep_ms(1400);

  // start DSP processor
  audioIO_loop();

}
