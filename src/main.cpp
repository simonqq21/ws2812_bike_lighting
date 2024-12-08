#include <Arduino.h>
#include "buttonlib2.h"
#include "EEPROM.h"
#include "FastLED.h"

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
 * 0.1
 * Momentary button control
 * button1
 * single click - cycle between off, low, medium, and high
 * double click - cycle between driving lights only and driving lights + RGB 
 * single long press - cycle through preset RGB solid colors
 * 
 * 0.2
 * implement the ff. LED effects:
 *  constant
 *  single flash
 *  double flash
 *  fade up and down
 *  double fade up and down
 * 
 * 0.3
 * Implement the ff. LED effects:
 * left/right shift multiple colors throughout the LED array
 * 
 * 
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
const int updatePeriodinMillis = 5;
const int totalPeriodLengthinMillis = 1000;
unsigned int keyPoints[6];

enum PWRSTATE {Off = 0, Low, Med, High};
enum MODESTATE{NORM, NORMPLUSRGB};

unsigned long lastTimeColorSwitched;

const byte hue_values[] = {0, 24, 48, 80, 96, 128, 144, 160, 192, 224, 0};
const byte brightness_values[] = {0, 80, 160, 250};

byte colorIndices[10] = {};
int lengthOfColors = 3;

byte curModeState = NORM;
byte curHue;
byte curSaturation;
byte curBrightness = Off;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void btn1_1shortclick_func() {
  curBrightness++;
  if (curBrightness > High) curBrightness = Off;
  Serial.print("curPowerState = ");
  Serial.println(curBrightness);
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
unsigned int ctr1;
byte curBrightness;

void steadyLEDs() {
  // for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(hue_values[curHue], curSaturation, brightness_values[curPowerState]);
  //   }
}

void singleFlashLEDs() {
  // // 1 Hz, single 30% DC flash
  // if (millis() - flashCycleTimer >= 100) {
  //   flashCycleTimer = millis();
  //   if (ctr1 < 3) curBrightness = brightness_values[curPowerState];
  //   else curBrightness = 0;
  //   ctr1++;
  //   ctr1 = ctr1 > 9? 0:ctr1;
  // }
}

void doubleFlashLEDs() {
  // // 1 Hz, double 15% DC flash
  // if (millis() - flashCycleTimer >= 50) {
  //   flashCycleTimer = millis();
  //   if (ctr1 / 3 == 0 || ctr1 / 3 == 2) curBrightness = brightness_values[curPowerState];
  //   else curBrightness = 0;
    // ctr1++;
    // ctr1 = ctr1 > 19? 0:ctr1;
  // }
}

void singleFadeLEDs() {
  // keyPoints[0] = 0;
  // keyPoints[1] = keyPoints[0] + 400/updatePeriodinMillis;
  // keyPoints[2] = keyPoints[1] + 400/updatePeriodinMillis;
  // keyPoints[3] = keyPoints[2] + 200/updatePeriodinMillis;
  // // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // // 5 ms fading steps
  // // 200 total steps; 0,80,160,200
  // if (millis() - flashCycleTimer >= updatePeriodinMillis) {
  //   flashCycleTimer = millis();
  //   if (ctr1 < keyPoints[1]) {
  //     curBrightness = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * brightness_values[curPowerState] / 255;
  //   } 
  //   else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
  //     curBrightness = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * brightness_values[curPowerState] / 255;
  //   } 
  //   else if (ctr1 >= keyPoints[2]) {
  //     curBrightness = 0;
  //   }
  //   ctr1++;
  //   ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1;
  // }
}

void doubleFadeLEDs() {
  // keyPoints[0] = 0;
  // keyPoints[1] = keyPoints[0] + 200/updatePeriodinMillis;
  // keyPoints[2] = keyPoints[1] + 200/updatePeriodinMillis;
  // keyPoints[3] = keyPoints[2] + 200/updatePeriodinMillis;
  // keyPoints[4] = keyPoints[3] + 200/updatePeriodinMillis;
  // keyPoints[5] = keyPoints[4] + 200/updatePeriodinMillis;
  // // 1 Hz double fade; 200 mS rise, 200 mS fall, 200 mS off
  // // 5 ms fading steps
  // // 200 total steps; 0,40,80,120,160,200
  // if (millis() - flashCycleTimer >= updatePeriodinMillis) {
  //   flashCycleTimer = millis();
  //   if (ctr1 < keyPoints[1]) {
  //     curBrightness = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * brightness_values[curPowerState] / 255;
  //   } 

  //   else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
  //     curBrightness = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * brightness_values[curPowerState] / 255;
  //   } 

  //   if (ctr1 >= keyPoints[2] && ctr1 < keyPoints[3]) {
  //     curBrightness = sin8((ctr1-keyPoints[2])*64/(keyPoints[3] - keyPoints[2])) * brightness_values[curPowerState] / 255;
  //   } 

  //   else if (ctr1 >= keyPoints[3] && ctr1 < keyPoints[4]) {
  //     curBrightness = sin8((ctr1-keyPoints[3])*64/(keyPoints[4] - keyPoints[3])+64) * brightness_values[curPowerState] / 255;
  //   } 
  //   else if (ctr1 >= keyPoints[4]) {
  //     curBrightness = 0;
  //   }
  //   ctr1++;
  //   ctr1 = ctr1 > keyPoints[5] - 1? 0:ctr1;
  // }
  // for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //   leds[i] = CHSV(hue_values[curHue], curSaturation, curBrightness);
  // }
}

void chasingLEDs() {
  // left/right shift multiple colors throughout the LED array
  /**
   * shift the LED pixels every 100 ms
   * 4 pixels red, 4 pixels green, 4 pixels blue 
  */ 
  curBrightness = brightness_values[curBrightness];
  if (millis() - flashCycleTimer >= 100) {
    flashCycleTimer = millis();
    if (ctr1 < 4) {
      curHue = hue_values[0];
    }
    else if (ctr1 >= 4 && ctr1 < 8) {
      curHue = hue_values[4];
    }
    else if (ctr1 >= 8) {
      curHue = hue_values[7];
    }
    // // shift LEDs with the flow of data
    // for (int i=NUM_LEDS - 1;i>0;i--) {
    //   leds[i] = leds[i-1];
    // }
    // leds[0] = CHSV(curHue, curSaturation, curBrightness);
    // shift LEDs against the flow of data
    for (int i=0;i<NUM_LEDS - 1;i++) {
      leds[i] = leds[i+1];
    }
    leds[NUM_LEDS - 1] = CHSV(curHue, curSaturation, curBrightness);
    ctr1++;
    ctr1 = ctr1 > 11? 0:ctr1;
    Serial.print("ctr1=");
    Serial.println(ctr1);
  }
}

void ledLoop() {

  if (curHue == 10) curSaturation = 0;
  else curSaturation = 255;

  // for (int i=0;i<NUM_LEDS/2;i++) {
  //   leds[i] = CHSV(0, 0, brightness_values[curPowerState]);
  // }
  // if (curModeState == NORMPLUSRGB) {
  //   
  // } else {
  //   for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(0, 0, 0);
  //   }
  // }
  

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

