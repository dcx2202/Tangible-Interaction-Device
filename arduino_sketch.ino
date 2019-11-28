#include <Wire.h>
#include "Adafruit_DRV2605.h"
#include <Adafruit_NeoPixel.h>


#define PIN 6 // Led ring pin
#define BUTTON_IN 7 // Button pin
#define fsrpin A0 // Force sensor pin
#define NUMPIXELS 24 // NeoPixel ring led count
#define DELAYVAL 20 // Time (in milliseconds) to pause between pixels
#define LED_R 255
#define LED_G 30
#define LED_B 30


Adafruit_DRV2605 drv; // Vibration motor
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800); // Setting up the NeoPixel library
int fsrreading; //Variable to store FSR value

void setup()
{
  Serial.begin(9600);

  // Setup pin as input
  pinMode(BUTTON_IN, INPUT);

  // Setup vibration motor
  drv.begin();
  drv.selectLibrary(1);
  
  // I2C trigger by sending 'go' command 
  // default, internal trigger when sending GO command
  drv.setMode(DRV2605_MODE_INTTRIG); 

  pixels.begin(); // Initialize NeoPixel strip object
  pixels.setBrightness(100);
}

void loop()
{
  // Read the FSR pin
  fsrreading = analogRead(fsrpin);

  // If the force sensor and the button aren't being pressed then the user
  // isn't handling the device and we don't turn on the leds/vibration motor
  if(digitalRead(BUTTON_IN) == LOW && fsrreading == 0)
  {
    // Turn off all pixels
    for(int i=0; i<NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
    }
    return;
  }
  
  // If the user is handling the device we turn on the leds and play an heartrate vibration effect
  for(int i=0; i<NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(LED_R, LED_G, LED_B));
    pixels.show();
  }

  // Set the heartrate vibration effect
  // First pulse
  drv.setWaveform(0, 47);
  drv.setWaveform(1, 0);
  drv.go();

  delay(300);

  // Second pulse
  drv.setWaveform(0, 48);
  drv.setWaveform(1, 0);
  drv.go();

  delay(800);
}
