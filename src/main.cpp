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
 * v2:
 * implement the ff. LED effects:
 *  constant
 *  single flash
 *  double flash
 *  fade up and down
 *  double fade up and down
 * 
 * Proposed RGB control UI:
 * 10 single colors, 10 double colors, 10 triple colors, 10 quad colors, 1 rainbow (10 * (1+2+3+4)+10 = 110 bytes)
 * constant, single flash, double flash, single fade, and double fade
 * special modes 
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

enum PWRSTATE {Off = 0, Low, Med, High};
enum MODESTATE{NORM, NORMPLUSRGB};

unsigned long lastTimeColorSwitched;

byte hue_values[] = {0, 24, 48, 80, 96, 128, 144, 160, 192, 224, 0};
byte brightness_values[] = {0, 80, 160, 250};


byte curPowerState = Off;
byte curModeState = NORM;
byte curSaturation;
byte curHue;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void btn1_1shortclick_func() {
  curPowerState++;
  if (curPowerState > High) curPowerState = Off;
  Serial.print("curPowerState = ");
  Serial.println(curPowerState);
}

void btn1_2shortclicks_func() {
  curModeState++;
  if (curModeState > NORMPLUSRGB) curModeState = 0;
  Serial.print("curModeState = ");
  Serial.println(curModeState);
}

void btn1_1longpress_func() {
  curHue++;
  if (curHue > 10) curHue = 0;
  Serial.print("curHue = ");
  Serial.println(curHue);
}

unsigned long flashCycleTimer;
int ctr1, ctr2;
byte curBrightness;

const int updatePeriodinMillis = 5;
const int totalPeriodLengthinMillis = 1000;
const int startRise = 0;
const int riseLength = 400/5;
const int startFall = startRise + riseLength;
const int fallLength = 400/5;
const int startOff =  startFall + fallLength;
const int offLength = 200/5;
const int endCycle = startOff + offLength;

void ledLoop() {
  if (curHue == 10) curSaturation = 0;
  else curSaturation = 255;

  // for (int i=0;i<NUM_LEDS/2;i++) {
  //   leds[i] = CHSV(0, 0, brightness_values[curPowerState]);
  // }
  // if (curModeState == NORMPLUSRGB) {
  //   for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(hue_values[curHue], curSaturation, brightness_values[curPowerState]);
  //   }
  // } else {
  //   for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(0, 0, 0);
  //   }
  // }
  

  // // 1 Hz, single 30% DC flash
  // if (millis() - flashCycleTimer >= 100) {
  //   flashCycleTimer = millis();
  //   if (ctr1 < 3) curBrightness = brightness_values[curPowerState];
  //   else curBrightness = 0;
  //   ctr1++;
  //   ctr1 = ctr1 > 9? 0:ctr1;
  // }

  // // 1 Hz, double 15% DC flash
  // if (millis() - flashCycleTimer >= 50) {
  //   flashCycleTimer = millis();
  //   if (ctr1 / 3 == 0 || ctr1 / 3 == 2) curBrightness = brightness_values[curPowerState];
  //   else curBrightness = 0;
    // ctr1++;
    // ctr1 = ctr1 > 19? 0:ctr1;
  // }

  // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,80,160,200

  if (millis() - flashCycleTimer >= 5) {
    flashCycleTimer = millis();
    if (ctr1 < startFall) {
      curBrightness = sin8((ctr1-startRise)*64/riseLength) * brightness_values[curPowerState] / 255;
    } 
    // else if (ctr1 >= 80 && ctr1 < 100) {
    //   curBrightness = brightness_values[curPowerState];
    // }
    else if (ctr1 >= startFall && ctr1 < startOff) {
      curBrightness = sin8((ctr1-startFall)*64/fallLength+64) * brightness_values[curPowerState] / 255;
    }
    else if (ctr1 >= startOff && ctr1 < endCycle) {
      curBrightness = 0;
    }
    ctr1++;
    ctr1 = ctr1 > endCycle - 1? 0:ctr1;
  }

  // 1 Hz double fade; 200 mS rise, 200 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,40,80,120,160,200


  for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
    leds[i] = CHSV(hue_values[curHue], curSaturation, curBrightness);
  }
  FastLED.show();

}

void setup() {
  Serial.begin(115200);
  Serial.println("RESET");
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

