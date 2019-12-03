#include <EEPROM.h>
#include "Adafruit_DRV2605.h"
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// Pins
#define LED_RING_PIN 6 // Led ring pin
#define BUTTON_PIN 2 // Button pin (input)

// Constants
#define NUM_PIXELS 24 // NeoPixel ring LED count
#define EXHALE_WAIT_TIME 5000 // Time to wait (in milliseconds) after the "exhale" phase of the breathing effect (brightness fades to 0%) ; higher = longer
#define BREATHING_SPEED 8000 // Duration (in milliseconds) of breathing effect (0% brightness to 100% back to 0%) ; higher = slower
#define NUM_BUTTON_PRESSES 5 // Number of times the button must be pressed consecutively to toggle the color pick mode

// Hardware objects
Adafruit_DRV2605 drv; // Vibration motor
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800); // Setting up the NeoPixel library

// Auxiliary Variables
int hue;
long lastBreathingUpdate; // Time of last breathing effect update
long lastExhale; // Time of last exhale
long loopCount; // Total number of loops
int currentBrightness; // Breathing effect current brightness
int heartRate; // Time to wait (in milliseconds) between heartbeats
int heartPulsesInterval; // Time to wait (in milliseconds) between each pulse in a heartbeat
bool brightnessIncreasing; // Breathing effect direction
bool colorPickMode; // Holds whether or not the color picked mode is active
int consecutiveButtonPresses; // Number of times the button was pressed within 1s of another press
long lastButtonPress; // Time of the last button press

void setup()
{
  Serial.begin(9600);

  // Setup BUTTON_PIN as input
  pinMode(BUTTON_PIN, INPUT);

  // Attach the button pin to an interrupt so that we can monitor button presses (used in entering/exiting color pick mode)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, RISING);

  // Setup vibration motor
  drv.begin();
  drv.selectLibrary(1);
  
  // I2C trigger by sending 'go' command 
  // default, internal trigger when sending GO command
  drv.setMode(DRV2605_MODE_INTTRIG); 

  pixels.begin(); // Initialize NeoPixel strip object
  pixels.setBrightness(100); // Set starting brightness at 100%

  // Initialize auxiliary variables
  // Get the hue value that's stored in the EEPROM
  EEPROM.get(0, hue);

  // Auxiliary variables to control the breathing effect
  lastBreathingUpdate = 0;
  lastExhale = 0;
  currentBrightness = 0;
  brightnessIncreasing = true;

  // Auxiliary variables to control the heartbeat effect
  heartRate = 800;
  heartPulsesInterval = 400;

  // Auxiliary variables to control color pick mode
  colorPickMode = false;
  consecutiveButtonPresses = 0;
  lastButtonPress = 0;
}

void breathingEffect()
{
  // Update the effect
  // Wait for the time we specified in EXHALE_WAIT_TIME if that's the phase we are in now
  if(millis() - lastExhale < EXHALE_WAIT_TIME)
    return;

  // Limit the number of times we update the effect so that it takes the time we specified in BREATHING_SPEED
  if(millis() - lastBreathingUpdate < double(BREATHING_SPEED) * 10.0 / 5000.0)
    return;

  // If brightness is increasing then keep increasing it until it reaches the max
  if(brightnessIncreasing)
  {
    if(currentBrightness < 255)
      currentBrightness++;
    else
    {
      brightnessIncreasing = false;
      currentBrightness--;
    }
  }
  // If it's decreasing then keep decreasing it until it reaches the min
  else
  {
    if(currentBrightness > 0)
      currentBrightness--;
    else
    {
      brightnessIncreasing = true;
      currentBrightness++;
      lastExhale = millis();
    }
  }
  
  // Update the LEDs
  uint32_t color = pixels.ColorHSV(hue, 255, currentBrightness);
  pixels.fill(color);
  pixels.show();

  // Update the time of the last breathing update
  lastBreathingUpdate = millis();
}

void heartbeatEffect()
{
  // Change the interval between pulses and the heart rate using perlin noise
  // to make the heartbeat effect feel more realistic
  heartPulsesInterval = map(inoise16(loopCount, loopCount), 0, 65535, 300, 500);
  heartRate = map(inoise16(loopCount, loopCount), 0, 65535, 400, 1200);
}

void buttonInterrupt()
{
  // Filter out some interrupt calls that can happen when the button is in a state that's not on or off
  if(millis() - lastButtonPress < 50)
    return;

  // If the last button press was less than a second ago then we assume the user is trying to toggle the color pick mode
  if(millis() - lastButtonPress < 1000)
    consecutiveButtonPresses++;
  else
    consecutiveButtonPresses = 0;
    
  lastButtonPress = millis();

  // If the button was pressed in the correct combination (NUM_BUTTON_PRESSES consecutive presses no more than a second apart)
  // then toggle the mode and, if we are exiting the color pick mode, save the chosen hue to use it in case the device is rebooted
  if(consecutiveButtonPresses >= NUM_BUTTON_PRESSES)
  {
    consecutiveButtonPresses = 0;

    // Save the hue in the EEPROM to be able to get it in case the device is rebooted
    if(colorPickMode)
      EEPROM.put(0, hue);

    // Toggle the color pick mode
    colorPickMode = !colorPickMode;
  }
}

void loop()
{
  // If we are in the color pick mode then cycle through all the ColorHSV
  // When we exit out of the color pick mode, the current hue is the one that's saved
  if(colorPickMode)
  {
    // Only cycle while the button is not pressed to allow the user to think about a color before it changes
    // by pressing the button
    if(digitalRead(BUTTON_PIN) == LOW)
    {
      // Keep cycling through the hue (varies from 0 - 65535)
      if(hue >= 65535)
        hue = 0;
      else
        hue += 2;
      
      pixels.fill(pixels.ColorHSV(hue, 255, 255));
      pixels.show();
    }

    return;
  }

  // Increase the loop count
  loopCount++;

  // Update the heartbeat effect
  heartbeatEffect();
  
  // If the button isn't being pressed then the user
  // isn't handling the device and we don't turn on the leds/vibration motor
  if(digitalRead(BUTTON_PIN) == LOW)
  {
    breathingEffect();
    return;
  }

  // If the button is being pressed then increase the brightness to the max and reset the breathing effect
  brightnessIncreasing = false;
  currentBrightness = 255;
  
  // If the user is handling the device we turn on the leds at max brightness and play an heartbeat vibration effect
  pixels.fill(pixels.ColorHSV(hue, 255, currentBrightness));
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
