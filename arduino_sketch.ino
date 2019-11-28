#include "Adafruit_DRV2605.h"
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// Pins
#define LED_RING_PIN 6 // Led ring pin
#define BUTTON_PIN 7 // Button pin (input)
//#define fsrpin A0 // Force sensor pin

// Constants
#define NUM_PIXELS 24 // NeoPixel ring LED count
#define NUM_LOOP_SKIPS 100 // Number of loops to skip before updating the breathing effect ; higher = slower breathing effect
#define WAIT_TIME 400 // Time to wait after the "exhale" phase of the breathing effect ; higer = longer
#define HUE 370 // Hue of the pink color

// Hardware objects
Adafruit_DRV2605 drv; // Vibration motor
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800); // Setting up the NeoPixel library

// Auxiliary Variables
long loop_count; // Total loop count
bool increasing; // Breathing effect direction
int current_brightness; // Breathing effect current brightness
int current_wait_time; // Time left to wait after the "exhale" phase of the breathing effect
int heart_rate; // Time to wait between heartbeats
int heart_pulses_interval; // Time to wait between each pulse in a heartbeat

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
  pixels.setBrightness(250); // Set starting brightness at 100%

  // Initialize auxiliary variables
  loop_count = 0;
  increasing = true;
  current_brightness = 0;
  current_wait_time = 0;
  heart_rate = 800;
  heart_pulses_interval = 400;
}

void breathing_effect()
{
  // Limit the number of times we update the effect
  if(loop_count % NUM_LOOP_SKIPS != 0)
    return;

  // Update the effect
  // Wait for some time after the "exhale" phase if that's now
  if(current_wait_time > 0)
  {
    current_wait_time--;
    return;
  }

  // First phase
  if(increasing)
  {
    // Increase the brightness until the max, then start decreasing
    if(current_brightness < 255)
      current_brightness++;
    else
    {
      increasing = false;
      current_brightness--;
    }
  }
  // Second phase
  else
  {
    // Decrease the brightness until the min, then start increasing
    if(current_brightness > 0)
      current_brightness--;
    else
    {
      current_wait_time = WAIT_TIME;
      increasing = true;
      current_brightness++;
    }
  }
  
  pixels.fill(pixels.ColorHSV(HUE, 255, current_brightness));
  pixels.show();
}

void heartbeat_effect()
{
  heart_pulses_interval = map(inoise16(loop_count, loop_count), 0, 65535, 300, 500);
  heart_rate = map(inoise16(loop_count, loop_count), 0, 65535, 400, 1200);
}

void loop()
{
  loop_count++;
  heartbeat_effect();
  
  // If the button isn't being pressed then the user
  // isn't handling the device and we don't turn on the leds/vibration motor
  if(digitalRead(BUTTON_PIN) == LOW)
  {
    breathing_effect();
    return;
  }

  increasing = false;
  current_brightness = 255;
  current_wait_time = 0;
  
  // If the user is handling the device we turn on the leds and play an heartrate vibration effect
  pixels.fill(pixels.ColorHSV(HUE, 255, 255));
  pixels.show();

  // Set the heartrate vibration effect
  // First pulse
  drv.setWaveform(0, 47);
  drv.setWaveform(1, 0);
  drv.go();

  delay(heart_pulses_interval);

  // Second pulse
  drv.setWaveform(0, 48);
  drv.setWaveform(1, 0);
  drv.go();

  delay(heart_rate);
}
