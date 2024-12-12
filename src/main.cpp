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
 * 0.4 
 * Implement a standard for storing the colors, modes, brightness
 * Incorporate the ability to have multiple colors for all modes.
 * 
 * 0.5
 * Implement basic button control.
 * Control brightness, rgb enable, color, and mode.
 * Proposed RGB control UI with one button:
 * 10 single colors
 * constant, single flash, double flash, single fade, and double fade
 * single click - switch brightness between off, low, med, and high
 * double click - switch between norm and normplusrgb
 * single long press - cycle through colors
 * double long press - cycle between constant, single flash, double flash, single fade, and double fade
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
unsigned int updatePeriodinMillis = 5;
int totalPeriodLengthinMillis = 1000;
unsigned int keyPoints[6];

enum PWRSTATE {
  PWR_OFF = 0, 
  PWR_LOW, 
  PWR_MED, 
  PWR_HIGH,
};
enum MODESTATE {
  MODE_NORM = 0, 
  MODE_NORMPLUSRGB,
};
enum RGBMODESTATE {
  RGBMODE_CONSTANT = 0, 
  RGBMODE_SINGLEFLASH, 
  RGBMODE_DOUBLEFLASH, 
  RGBMODE_SINGLEFADE, 
  RGBMODE_DOUBLEFADE, 
  RGBMODE_FORWARDSHIFT, 
  RGBMODE_REVERSESHIFT,
};

unsigned long lastTimeColorSwitched;
// int x = RGBMODE_CONSTANT;
const byte hue_values[] = {0, 24, 48, 80, 96, 128, 144, 160, 192, 224, 0};
const byte brightness_values[] = {0, 80, 160, 250};

/**
 * curMode - either MODE_NORM or MODE_NORMPLUSRGB
 *    MODE_NORM - only front and rear lights on
 *    MODE_NORMPLUSRGB - front, rear, and RGB lights on
 * curBrightness - global brightness of all lights
 *    - either PWR_OFF, PWR_LOW, PWR_MED, or PWR_HIGH
 * curColors - array of colors in that mode
 *    - can contain anywhere between 0 to 10 colors, indices inside hue_values
 *    - length of colors is in lenColors
 * curRGBMode - current RGB mode
 *    - can be either RGBMODE_CONSTANT, RGBMODE_SINGLEFLASH, RGBMODE_DOUBLEFLASH,
 *      RGBMODE_SINGLEFADE, RGBMODE_DOUBLEFADE, RGBMODE_FORWARDSHIFT, 
 *      RGBMODE_REVERSESHIFT
 */ 

struct mainConfig {
  byte curMode = MODE_NORMPLUSRGB;
  byte curBrightness = PWR_LOW;
  byte curColors[10];
  int lenColors = 3;
  byte curRGBMode;
};

mainConfig configuration;

/**
 * current hue and saturation variables
 */
byte curHueIndex;
byte curHueVal;
byte curSaturationVal;
byte curBrightnessVal;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void btn1_1shortclick_func() {
  configuration.curBrightness++;
  if (configuration.curBrightness > PWR_HIGH) configuration.curBrightness = PWR_OFF;
  Serial.print("curBrightness = ");
  Serial.println(configuration.curBrightness);
}

void btn1_2shortclicks_func() {
  configuration.curMode++;
  if (configuration.curMode > MODE_NORMPLUSRGB) configuration.curMode = 0;
  Serial.print("curMode = ");
  Serial.println(configuration.curMode);
}

void btn1_1longpress_func() {
  curHueVal++;
  if (curHueVal > 10) curHueVal = 0;
  Serial.print("curHueVal = ");
  Serial.println(curHueVal);
}

unsigned long flashCycleTimer;
unsigned int ctr1;

void steadyLEDs() {
  // for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(hue_values[curHueVal], curSaturationVal, brightness_values[curBrightness]);
  //   }
}

void singleFlashLEDs() {
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300/updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis/updatePeriodinMillis;
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) curBrightnessVal = brightness_values[configuration.curBrightness];
    else curBrightnessVal = 0;
    ctr1++;
    if (ctr1 > keyPoints[2] - 1) {
      curHueIndex++;
      curHueIndex = curHueIndex > configuration.lenColors-1? 0: curHueIndex;
      curHueVal = hue_values[configuration.curColors[curHueIndex]];
    }
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1;
    for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
      leds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

void doubleFlashLEDs() {
  // 1 Hz, double 15% DC flash
  updatePeriodinMillis = 50;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 150/updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 150/updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 150/updatePeriodinMillis;
  keyPoints[4] = totalPeriodLengthinMillis/updatePeriodinMillis;
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if ((ctr1 < keyPoints[1]) || (ctr1 >= keyPoints[2] && ctr1 < keyPoints[3])) curBrightnessVal = brightness_values[configuration.curBrightness];
    else curBrightnessVal = 0;
    ctr1++;
    if (ctr1 > keyPoints[4] - 1) {
      curHueIndex++;
      curHueIndex = curHueIndex > configuration.lenColors-1? 0: curHueIndex;
      curHueVal = hue_values[configuration.curColors[curHueIndex]];
    }
    ctr1 = ctr1 > keyPoints[4] - 1? 0:ctr1;
    for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
      leds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

void singleFadeLEDs() {
  updatePeriodinMillis = 5;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 400/updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 400/updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 200/updatePeriodinMillis;
  // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,80,160,200
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      curBrightnessVal = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * brightness_values[configuration.curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      curBrightnessVal = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * brightness_values[configuration.curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[2]) {
      curBrightnessVal = 0;
    }
    ctr1++;
    if (ctr1 > keyPoints[3] - 1) {
      curHueIndex++;
      curHueIndex = curHueIndex > configuration.lenColors-1? 0: curHueIndex;
      curHueVal = hue_values[configuration.curColors[curHueIndex]];
    }
    ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1;
    for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
      leds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

void doubleFadeLEDs() {
  updatePeriodinMillis = 5;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 200/updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 200/updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 200/updatePeriodinMillis;
  keyPoints[4] = keyPoints[3] + 200/updatePeriodinMillis;
  keyPoints[5] = keyPoints[4] + 200/updatePeriodinMillis;
  // 1 Hz double fade; 200 mS rise, 200 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,40,80,120,160,200
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      curBrightnessVal = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * brightness_values[configuration.curBrightness] / 255;
    } 

    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      curBrightnessVal = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * brightness_values[configuration.curBrightness] / 255;
    } 

    if (ctr1 >= keyPoints[2] && ctr1 < keyPoints[3]) {
      curBrightnessVal = sin8((ctr1-keyPoints[2])*64/(keyPoints[3] - keyPoints[2])) * brightness_values[configuration.curBrightness] / 255;
    } 

    else if (ctr1 >= keyPoints[3] && ctr1 < keyPoints[4]) {
      curBrightnessVal = sin8((ctr1-keyPoints[3])*64/(keyPoints[4] - keyPoints[3])+64) * brightness_values[configuration.curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[4]) {
      curBrightnessVal = 0;
    }
    ctr1++;
    if (ctr1 > keyPoints[5] - 1) {
      curHueIndex++;
      curHueIndex = curHueIndex > configuration.lenColors-1? 0: curHueIndex;
      curHueVal = hue_values[configuration.curColors[curHueIndex]];
    }
    ctr1 = ctr1 > keyPoints[5] - 1? 0:ctr1;
  }
  for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
    leds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
  }
}

void chasingLEDs() {
  // left/right shift multiple colors throughout the LED array
  /**
   * shift the LED pixels every 100 ms
   * 4 pixels red, 4 pixels green, 4 pixels blue 
  */ 
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 400/updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 400/updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 400/updatePeriodinMillis;
  curBrightnessVal = brightness_values[configuration.curBrightness];
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    ctr1++;
    if (!(ctr1 % 4)) {
      curHueIndex++;
    }
    curHueIndex = curHueIndex > (configuration.lenColors - 1)?0: curHueIndex;
    curHueVal = hue_values[configuration.curColors[curHueIndex]];

    // // shift LEDs with the flow of data
    // for (int i=NUM_LEDS - 1;i>0;i--) {
    //   leds[i] = leds[i-1];
    // }
    // leds[0] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    
    // shift LEDs against the flow of data
    for (int i=0;i<NUM_LEDS - 1;i++) {
      leds[i] = leds[i+1];
    }
    
    leds[NUM_LEDS - 1] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    
  }
}

void ledLoop() {

  if (curHueVal == 10) curSaturationVal = 0;
  else curSaturationVal = 255;

  // for (int i=0;i<NUM_LEDS/2;i++) {
  //   leds[i] = CHSV(0, 0, brightness_values[curBrightness]);
  // }
  // if (curMode == MODE_NORMPLUSRGB) {
  //   
  // } else {
  //   for (int i=NUM_LEDS/2;i<NUM_LEDS;i++) {
  //     leds[i] = CHSV(0, 0, 0);
  //   }
  // }
  
  // singleFlashLEDs();
  doubleFlashLEDs();
  // singleFadeLEDs();
  // doubleFadeLEDs();
  // chasingLEDs();
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

  // curColors[0] = 0;
  // curColors[1] = 1;
  // curColors[2] = 2;
  // curColors[3] = 4;
  // curColors[4] = 7;
  // curColors[5] = 8;
  // lenColors = 6;

  // curColors[0] = 4;
  // lenColors = 1;

  configuration.curColors[0] = 0;
  configuration.curColors[1] = 7;
  configuration.lenColors = 2;
}

void loop() {
  btn1.loop();
  ledLoop();
}

