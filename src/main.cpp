#include <Arduino.h>
#include "buttonlib2.h"
#include "EEPROM.h"
#include "FastLED.h"
#include <IRremote.hpp>

/**
 * TODO: 
 * define three classes of ws2812 LED pixels:
 * front light
 * RGB light
 * rear light
 * 
 * front light is only either white or orange
 * rear light is only either red or orange
 * rgb light is controlled by the set RGB mode.
 * 
 * v1: 
 * Momentary button control
 * button1
 * single click - cycle between off, low, medium, and high
 * double click - cycle between driving lights only and driving lights + RGB 
 * single long press - cycle through preset RGB solid colors
 * 
 */

const int LED_PIN = 4;
const int BTN1_PIN = 2;

InterruptButton btn1(BTN1_PIN);
const int NUM_LEDS = 8;
const int BRIGHTNESS = 250;
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
CRGB leds[NUM_LEDS];
#define UPDATES_PER_SECOND 100

CRGBPalette16 curPalette;
TBlendType curBlending;

enum PWRSTATE {Off, Low, Med, High};
enum MODESTATE{NORM, NORMPLUSRGB};

unsigned long lastTimeColorSwitched;
byte curHue;
byte curSaturation;
byte curValue;
// byte hueValues[] = {0, }

byte hue_values[] = {0, 32, 48, 80, 96, 128, 144, 160, 192, 224, 0};
byte brightness_values[] = {0, 80, 160, 250};


byte curPowerState = Off;
byte curModeState = NORM;
byte curRGBState;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void btn1_1shortclick_func() {
  curPowerState++;
  Serial.print("curPowerState = ");
  Serial.println(curPowerState);
  if (curPowerState > High) curPowerState = 0;
}

void btn1_2shortclicks_func() {
  curModeState++;
  Serial.print("curModeState = ");
  Serial.println(curModeState);
  if (curModeState > NORMPLUSRGB) curModeState = 0;
}

void btn1_1longpress_func() {

}

void ledLoop() {
  if (millis() - lastTimeColorSwitched > 1000) {
    lastTimeColorSwitched = millis();
    curValue = 1;
    if (curHue == 10) curSaturation = 0;
    else curSaturation = 255;
    for (int i=0;i<NUM_LEDS;i++) {
      leds[i].setHSV(hue_values[curHue], curSaturation, brightness_values[curValue]);
    }
    curHue++;
    if (curHue > 10) curHue = 0;
  }
  // for (int i=0;i<NUM_LEDS;i++) {
  //   leds[i] = CHSV(hue_values[2], 255, brightness_values[1]);
  // }
  
  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  btn1.begin(btn1_change_func);
  // single click - cycle between off, low, medium, and high
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  // double click - cycle between driving lights only and driving lights + RGB 
  btn1.set2ShortPressFunc(btn1_2shortclicks_func);
  // single long press - cycle through preset RGB solid colors
  btn1.set1LongPressFunc(btn1_1longpress_func);
  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
}

void loop() {
  btn1.loop();
  ledLoop();
}

