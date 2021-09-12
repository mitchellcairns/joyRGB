#include <FastLED.h>
#include "config.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

#define LED_COUNT 4
#define COLOR_TIME 50 // Time to fade between colors
#define CHECK_COUNT 5 // How many times to poll buttons for averages
#define BUTTON_THRESHOLD 45 // Value cutoff to determine if button is pressed or not (analog read)
byte SLOWEST_RAINBOW = 26;

// Button states setup
#define NO_PRESS 0
#define RELEASED_PRESS 1
#define HELD_PRESS 2

// Release state setup
#define HELD_START 200
#define HELD_ONE 201
#define HELD_TWO 202
#define HELD_THREE 203


// PIN setup
// ----------------
#define LED_PIN 0
#define aPin A3
#define yPin A2
#define ePin 2
// ----------------
// ----------------

byte SAV_DEBUG = 2;

// Button tracking
// ---------------
bool aPressed = false;
bool yPressed = false;
bool allPressed = false; // true if all buttons pressed
bool finishedHold = true; // Keep track of presses to prevent trailing inputs
byte aValue = 0; // Use to get a button value
byte yValue = 

0; // Use to get y button value
int timer = 0; // store the timer value when counting button presses
int buttonTime = 100; // timer cap
byte buttonPresses = 0; // how many times the button has been pressed
// ----------------
// ----------------

// Color/Brightness setting
// ----------------
byte brightness = 70; // led brightness

bool colorDirty = true; // Set true after changing modes!
bool colorSet = false; // Set true to start a fade!
bool colorInit = false; // Set true after changing and setting up your mode
float colorIncrement = 0; // Used to determine color fade increment (between 0 and 1)

byte colorStep = 15;         // Amount to step up when performing first color selection before going into fine

byte fadeTime = COLOR_TIME; // Time to fade between color ints (1 value at a time) for rainbow modes in ms
byte rainbowTime = 20;

CRGB lastLeds[LED_COUNT]; // Previous color values
CRGB leds[LED_COUNT]; // Current color values
CRGB nextLeds[LED_COUNT]; // Upcoming color values (when fading)
// ----------------
// ----------------

// Mode settings
// ----------------
byte configMode = 0;
#define DEFAULT_MODE    0
#define CONFIG_MODE     1   // Edit general configuration
#define BRIGHTNESS_MODE 2   // Edit brightness for all presets

bool enabled = true;
bool displayMode = true; // If display mode is ON, the LEDs will stay on. 

byte colorMode = 0;
#define COLOR_PRESET     0
#define COLOR_SOLID      1
#define COLOR_RAINBOW    2
byte colorModes = 3;

byte presetSelection = 0;

byte presetCount = 5;
CRGB presets[5][LED_COUNT] = {
  { CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB:: Red }, //SFC sideways right 
  { CRGB::Green, CRGB::Blue, CRGB::Red, CRGB::Yellow}, // SFC
  { 0xCBCBCB, 0xCBCBCB, 0x41C4C0, 0xEC1328}, // Gamecube - Gray, Gray, Teal, Red
  { 0xE96261, 0xF2CF1A, 0x62CBA1, 0x3E7CCB}, // Dreamcast
  { 0xFFC100, 0xFFC100, CRGB::MediumBlue, CRGB::Green}  // N64
};

CHSV rainbowColor = CHSV(0, 255, 255); // Start with red

UserPreference upref;

// ----------------
// ----------------

void setup() {
  // put your setup code here, to run once:
  delay(100);
  EEPROM.get(0, upref);
  delay(100);
  if (upref.saved == SAV_DEBUG) {
    brightness = upref.brightness;
    colorMode = upref.colorMode;
    rainbowTime = upref.rainbowTime;
    rainbowColor.hue = upref.hue;
    rainbowColor.saturation = upref.saturation;
    presetSelection = upref.presetSelection;
    displayMode = upref.displayMode;
  }
  
  ADMUX = 0x10010000; // Enable internal 2.56v reference
  // FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, LED_COUNT);
  FastLED.setBrightness(brightness);
  FastLED.show();

  pinMode(yPin, INPUT);
  pinMode(aPin, INPUT);
  pinMode(ePin, INPUT_PULLUP);
}

void loop() {

  byte butt = buttonCounter();

  idle(butt);

  FastLED.delay(CHECK_COUNT);

  if (!displayMode) {
    if (sleepCheck()) sleep(); // Check to see if we should sleep!
  }
  else {
    delayMicroseconds(400);
  }

}

byte buttonCounter() {

  if (buttonCheck(false) == RELEASED_PRESS) { // When a button is pressed what do we do?
    if (timer == 0 and buttonPresses == 0) { // If we haven't started a timer, lets add a button press and begin adding to the timer.
      timer += 1;
      buttonPresses = 1;
    }
    else if (timer < buttonTime) { // If we are still in the allotted timeframe, add another button press.
      buttonPresses += 1;
    }
  }

  if (timer > 0 and timer < buttonTime) { // If we're in a time loop, do the following
    timer += 1;
  }
  else if (timer >= buttonTime) { // What do we do if the timer is full? We reset the timer, return the button press count and reset that.
    byte toReturn = buttonPresses;
    if (buttonCheck(true) == HELD_PRESS){
      toReturn += HELD_START;
    }
    buttonPresses = 0;
    timer = 0;
    return toReturn;
  }

  // Default return 0
  return 0;
}

byte buttonCheck(bool timeOver) {
  smooth();

  if (aValue >= BUTTON_THRESHOLD) aPressed = true;
  else {
    aPressed = false;
    allPressed = false;
  }

  if (yValue >= BUTTON_THRESHOLD) yPressed = true;
  else {
    yPressed = false;
  }

  if (aPressed and yPressed and !allPressed) {
    allPressed = true;
  }
  else if (allPressed and !yPressed and finishedHold) {
    allPressed = false;
    return RELEASED_PRESS;
  }
  else if (!finishedHold and !yPressed){
    allPressed = false;
    finishedHold = true;
  }
  
  if (allPressed and timeOver) {
    allPressed = false;
    finishedHold = false;
    return HELD_PRESS;
  }

  return NO_PRESS;
}

bool sleepCheck() {
  bool shouldSleep = true;
  for (int i = 0; i < 4; i++){
    int returnValue = digitalRead(ePin);
    if (returnValue == LOW) shouldSleep = false;
  }

  return shouldSleep;
}

void smooth() { // Smooth read of values
  aValue = 0;
  yValue = 0;

  for (byte i = 0; i < CHECK_COUNT; i++) {
    int tempA = analogRead (aPin);
    tempA = analogRead(aPin);
    if ((tempA/4) > aValue) aValue = (tempA/4);
  }
  FastLED.delay(1);
  for (byte i = 0; i < CHECK_COUNT; i++) {
    int tempY = analogRead (yPin);
    tempY = analogRead(yPin);
    if ((tempY/4) > yValue) yValue = (tempY/4);
  }
  FastLED.delay(1);

  // Take an average of all the readings.
  //aValue = aValue / CHECK_COUNT;
  //yValue = yValue / CHECK_COUNT;

}

#define POWER_SWITCH      0
#define SATURATION_CHANGE 1
#define HUE_CHANGE        2
#define BRIGHTNESS_UP     3
#define BRIGHTNESS_DOWN   4
#define COLOR_MODE_CHANGE 5
#define RAINBOW_SPEEDUP   6
#define PRESET_CYCLE      7
#define ENABLE_BRIGHTNESS 11
#define SAVE_ALL          12
#define ENABLE_CONFIG     13
#define MODE_CYCLE        14
#define DISPLAY_MODE      15

void buttonFunction(byte in) {
  if (in == COLOR_MODE_CHANGE) {          // Switch color modes
    colorMode += 1;
    if (colorMode >= colorModes) colorMode = 0;
    if (upref.saved == SAV_DEBUG)
    {
      rainbowColor.hue = upref.hue;
      rainbowColor.saturation = upref.saturation;
    }
    switchIndicator(CRGB::Blue);
    resetColor(false);
  }
  else if (in == POWER_SWITCH) {          // Turn power on/off
    if (enabled) {
      for (byte i = brightness; i > 0; i--) {
        FastLED.setBrightness(i);
        FastLED.show();
        FastLED.delay(5);
      }
      FastLED.clear();
      FastLED.show();
      enabled = false;
    }
    else if (!enabled) {
      for(byte i = 0; i < LED_COUNT; i++) {
        leds[i] = lastLeds[i];
      }
      for (byte i = 1; i <= brightness; i++) {
        FastLED.setBrightness(i);
        FastLED.show();
        FastLED.delay(10);
      }
      enabled = true;
    }
  }
  else if (in == SATURATION_CHANGE) {     // Uptick the saturation
    rainbowColor.saturation -= 20;
    if (rainbowColor.saturation <= 0) rainbowColor.saturation = 255;
    resetColor(colorMode == COLOR_RAINBOW);
  }
  else if (in == HUE_CHANGE) {            // Uptick the hue
    rainbowColor.hue += 15;
    if (rainbowColor.hue >= 255) rainbowColor.hue = 0;
    resetColor(colorMode == COLOR_RAINBOW);
  }
  else if (in == BRIGHTNESS_UP) {         // Uptick brightness
    if (brightness + 20 <= 255) brightness += 20;
    FastLED.setBrightness(brightness);
    FastLED.show();
  }
  else if (in == BRIGHTNESS_DOWN) {       // Downtick brightness
    if (brightness - 20 >= 0) brightness -= 20;
    FastLED.setBrightness(brightness);
    FastLED.show();
  }
  else if (in == RAINBOW_SPEEDUP) {       // Uptick rainbow fade speed
    rainbowTime -= 2;
    if (rainbowTime <= 0) rainbowTime = SLOWEST_RAINBOW;
  }
  else if (in == PRESET_CYCLE) {          // Next item in presets
    presetSelection += 1;
    if (presetSelection >= presetCount) presetSelection = 0;
    resetColor(false);
  }
  else if (in == ENABLE_BRIGHTNESS) {     // Switch to brightness edit mode
    FastLED.setBrightness(brightness);
    FastLED.show();
    configMode = BRIGHTNESS_MODE;
    switchIndicator(CRGB::Yellow);
    resetColor(false);
  }
  else if (in == SAVE_ALL) {              // Save all settings to EEPROM
    upref.hue = rainbowColor.hue;
    upref.saturation = rainbowColor.saturation;
    upref.brightness = brightness;
    upref.colorMode = colorMode;
    upref.rainbowTime = rainbowTime;
    upref.saved = SAV_DEBUG;
    upref.presetSelection = presetSelection;
    upref.displayMode = displayMode;
    EEPROM.put(0, upref);
    delay(100);
    configMode = DEFAULT_MODE;
    switchIndicator(CRGB::White);
    resetColor(false);
  }
  else if (in == ENABLE_CONFIG) {
    configMode = CONFIG_MODE;
    switchIndicator(CRGB::Teal);
    resetColor(false);
  }
  else if (in == MODE_CYCLE) {
    if ( colorMode == COLOR_PRESET ) {
      presetSelection += 1;
      resetColor(false);
      if (presetSelection >= presetCount) 
      {
        presetSelection=0;
        buttonFunction(COLOR_MODE_CHANGE);
      }
    }
    else if ( colorMode == COLOR_SOLID ) {
      buttonFunction(COLOR_MODE_CHANGE);
    }
    else if ( colorMode == COLOR_RAINBOW ) {
      presetSelection = 0;
      buttonFunction(COLOR_MODE_CHANGE);
    }
  }
  else if (in == DISPLAY_MODE) {
    displayMode = !displayMode;
    if (displayMode) switchIndicator(CRGB::Orange);
    else switchIndicator(CRGB::Green);
    
    resetColor(false);
  }
}

void idle(byte pressCount) {
  // COLOR RUNTIME
  if (!colorSet and colorDirty and !colorInit) {    // Only run the mode init if the color is not set and marked dirty. This should only happen once per mode switch
    fadeTime = COLOR_TIME;          // Reset to default on all dirty sets
    if (colorMode == COLOR_PRESET) {
      setNextColors(presets[presetSelection]);
    }
    else if (colorMode == COLOR_SOLID) {
      setNextColor(rainbowColor);
    }
    else if (colorMode == COLOR_RAINBOW and !colorInit) {
      fadeTime = rainbowTime;
      rainbowColor = CHSV(0, 255, 255);
      setNextColor(rainbowColor);
      colorInit = true;
    }
  }
  else if (colorSet and (colorMode == COLOR_RAINBOW) and !colorDirty and colorInit) { // Special rule for rainbow mode
    fadeTime = rainbowTime;
    buttonFunction(HUE_CHANGE);
    setNextColor(rainbowColor);
    colorInit = true;
  }

  if (enabled) colorTick(fadeTime); // perform color tick if the chip is set enable


  // USER INTERFACE
  // --------------
  // 1 and 2 button press options
  if (configMode == CONFIG_MODE and colorMode == COLOR_PRESET) {
    if (pressCount == 1) buttonFunction(PRESET_CYCLE);
  }
  else if ((configMode == CONFIG_MODE and colorMode == COLOR_SOLID)) {
    if (pressCount == 1) buttonFunction(HUE_CHANGE);
    else if (pressCount == 2) buttonFunction(SATURATION_CHANGE);
  }
  else if (configMode == CONFIG_MODE and colorMode == COLOR_RAINBOW) {
    if (pressCount == 1) buttonFunction(RAINBOW_SPEEDUP);
  }

  // 3 button press options
  if (configMode == CONFIG_MODE) {
    if (pressCount == 3) buttonFunction(COLOR_MODE_CHANGE);
  }

  // 4 button press options
  if (configMode == DEFAULT_MODE) {
    if (pressCount == HELD_ONE) buttonFunction(POWER_SWITCH);
    else if (pressCount == HELD_TWO and enabled) buttonFunction(MODE_CYCLE);
    else if (pressCount == HELD_THREE and enabled) buttonFunction(DISPLAY_MODE);
    else if (pressCount == 2 and enabled) buttonFunction(BRIGHTNESS_DOWN);
    else if (pressCount == 3 and enabled) buttonFunction(BRIGHTNESS_UP);
    else if (pressCount == 4 and enabled) buttonFunction(ENABLE_CONFIG);
  }
  else if (configMode == CONFIG_MODE) {
    if (pressCount == 4) buttonFunction(ENABLE_BRIGHTNESS);
  }
  else if (configMode == BRIGHTNESS_MODE) {
    if (pressCount == 1) buttonFunction(BRIGHTNESS_DOWN);
    else if (pressCount == 2) buttonFunction(BRIGHTNESS_UP);
    else if (pressCount == 4) buttonFunction(SAVE_ALL);
  }
  // -------------
  // -------------
  // END USER INTERFACE

}

void setNextColors(CRGB colors[LED_COUNT]) {
  for (byte i = 0; i < LED_COUNT; i++) {
    lastLeds[i] = leds[i];
    nextLeds[i] = colors[i];
  }
  resetColor(false);
}

void setNextColor(CRGB color) {
  for (byte i = 0; i < LED_COUNT; i++) {
    lastLeds[i] = leds[i];
    nextLeds[i] = color;
  }
  resetColor(colorMode==COLOR_RAINBOW);
}

void colorTick(float changeTime) { // Handle fading between colors for transition purposes
  if (colorDirty and !colorSet) {

    colorIncrement += (255 / changeTime);
    byte inc = (byte) colorIncrement;

    for (byte i = 0; i < LED_COUNT; i++) { // Set the next color on all LEDs blended
      leds[i] = blend(lastLeds[i], nextLeds[i], inc);
    }

    if (inc >= 255) { // If the fade is complete, reset the increment and set as not dirty!
      for (byte i = 0; i < LED_COUNT; i++) {
        leds[i] = nextLeds[i]; // Set all colors to their final form
      }
      colorIncrement = 0;
      colorDirty = false; // Unmark that the color is dirty
      colorSet = true; // Mark that the color has been 'set'.
    }

  }
  FastLED.show(); // Send update to LEDs
}

void resetColor(bool init) {
  colorDirty = true;
  colorSet = false;
  colorInit = init;
}

void switchIndicator(CRGB col) {

  for (byte i = 0; i < 2; i++)
  {
    fill_solid(leds, LED_COUNT, col);
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    delay(500);
  }
}

void sleep() {
  configMode = DEFAULT_MODE;
  enabled = true;
  buttonFunction(POWER_SWITCH);
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0,wakeUp,LOW);
  sleep_mode();
  sleep_disable();
  detachInterrupt(0);
  enabled = false;
  buttonFunction(POWER_SWITCH);

} // sleep

void wakeUp() {}
