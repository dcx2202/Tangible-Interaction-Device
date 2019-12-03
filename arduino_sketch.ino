#include "Adafruit_DRV2605.h"
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// Pins
#define LED_RING_PIN 6 // Led ring pin
#define BUTTON_PIN 7 // Button pin (input)

// Constants
#define NUM_PIXELS 24 // NeoPixel ring LED count
#define EXHALE_WAIT_TIME 5000 // Time to wait (in milliseconds) after the "exhale" phase of the breathing effect (brightness fades to 0%) ; higher = longer
#define BREATHING_SPEED 8000 // Duration (in milliseconds) of breathing effect (0% brightness to 100% back to 0%) ; higher = slower
#define PINK_HUE 370 // Hue of the pink color

// Hardware objects
Adafruit_DRV2605 drv; // Vibration motor
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800); // Setting up the NeoPixel library

// Auxiliary Variables
long lastBreathingUpdate; // Time of last breathing effect update
long lastExhale; // Time of last exhale
long loopCount; // Total number of loops
bool increasing; // Breathing effect direction
int currentBrightness; // Breathing effect current brightness
int heartRate; // Time to wait (in milliseconds) between heartbeats
int heartPulsesInterval; // Time to wait (in milliseconds) between each pulse in a heartbeat

void setup()
{
  Serial.begin(9600);

  // Setup BUTTON_PIN as input
  pinMode(BUTTON_PIN, INPUT);

  // Setup vibration motor
  drv.begin();
  drv.selectLibrary(1);
  
  // I2C trigger by sending 'go' command 
  // default, internal trigger when sending GO command
  drv.setMode(DRV2605_MODE_INTTRIG); 

  pixels.begin(); // Initialize NeoPixel strip object
  pixels.setBrightness(100); // Set starting brightness at 100%

  // Initialize auxiliary variables
  lastBreathingUpdate = 0;
  lastExhale = 0;
  increasing = true;
  currentBrightness = 0;
  heartRate = 800;
  heartPulsesInterval = 400;
}

void breathing_effect()
{
  // Update the effect
  // Wait for some time after the "exhale" phase if that's now
  if(millis() - lastExhale < EXHALE_WAIT_TIME)
    return;

  // Limit the number of times we update the effect
  if(millis() - lastBreathingUpdate < double(BREATHING_SPEED) * 10.0 / 5000.0)
    return;

  if(increasing)
  {
    if(currentBrightness < 255)
      currentBrightness++;
    else
    {
      increasing = false;
      currentBrightness--;
    }
  }
  else
  {
    if(currentBrightness > 0)
      currentBrightness--;
    else
    {
      increasing = true;
      currentBrightness++;
      lastExhale = millis();
    }
  }
  
  uint32_t color = pixels.ColorHSV(PINK_HUE, 255, currentBrightness);
  pixels.fill(color);
  pixels.show();

  lastBreathingUpdate = millis();
}

void heartbeat_effect()
{
  heartPulsesInterval = map(inoise16(loopCount, loopCount), 0, 65535, 300, 500);
  heartRate = map(inoise16(loopCount, loopCount), 0, 65535, 400, 1200);
}

void loop()
{
  loopCount++;
  heartbeat_effect();
  
  // If the button isn't being pressed then the user
  // isn't handling the device and we don't turn on the leds/vibration motor
  if(digitalRead(BUTTON_PIN) == LOW)
  {
    breathing_effect();
    return;
  }

  increasing = false;
  currentBrightness = 255;
  
  // If the user is handling the device we turn on the leds and play an heartrate vibration effect
  pixels.fill(pixels.ColorHSV(PINK_HUE, 255, 255));
  pixels.show();

  // Set the heartrate vibration effect
  // First pulse
  drv.setWaveform(0, 47);
  drv.setWaveform(1, 0);
  drv.go();

  delay(heartPulsesInterval);

  // Second pulse
  drv.setWaveform(0, 48);
  drv.setWaveform(1, 0);
  drv.go();

  delay(heartRate);
}
