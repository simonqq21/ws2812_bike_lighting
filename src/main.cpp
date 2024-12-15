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
 * double click - switch operation mode between norm and normplusrgb
 * single long press - cycle through colors
 * double long press - cycle between constant, single flash, double flash, single fade, and double fade
 * 
 * 0.6
 * Implement automatic saving to the EEPROM
 * Save current configuration to the EEPROM 60 seconds after the settings are last changed.
 * 
 * 1.0 - version 1 complete
 * 
 * Proposed complete features:
 *    ESP32 will be used because of it has built-in BLE.
 *    Hardware power switch
 *    3x 18650 battery
 *    2 boost converters to boost from battery voltage to 5V to power MCU and LED strips 
 *    Control via single pushbutton and mobile app over bluetooth
 *    single push button will be used to:
 *      power on/off, 
 *      change brightness,
 *      change operating mode, 
 *      cycle through 8 presets
 *    Mobile app control over BLE will be used to:
 *      perform all operations of the pushbutton, and 
 *      change RGB modes for each of the 8 presets
 *      
 *    LDR for automatic on-off and three level brightness adjustment
 *    
 */

// #define AVR
#define ESP32

#include <Arduino.h>
#include "buttonlib2.h"
#include "FastLED.h"
#ifdef AVR
  #include "EEPROM.h"
#endif
#ifdef ESP32
  #include <Preferences.h>
  Preferences prefs;
#endif

/** 
 * light power states
 *    off
 *    low
 *    med
 *    high
 */
enum PWRSTATE {
  PWR_OFF = 0, 
  PWR_LOW, 
  PWR_MED, 
  PWR_HIGH,
};
/** operation modes
 *    MODE_NORM - only front and rear lights are on
 *    MODE_NORMPLUSRGB - front, rear, and RGB side lights are on
 */
enum MODESTATE {
  MODE_NORM = 0, 
  MODE_NORMPLUSRGB,
};
/**
 * RGB modes
 *    constant on, 
 *    single flash, 
 *    double flash, 
 *    single fade, 
 *    double fade, 
 *    forward shift, 
 *    reverse shift
 */
enum RGBMODESTATE {
  RGBMODE_CONSTANT = 0, 
  RGBMODE_SINGLEFLASH, 
  RGBMODE_DOUBLEFLASH, 
  RGBMODE_SINGLEFADE, 
  RGBMODE_DOUBLEFADE, 
  RGBMODE_FORWARDSHIFT, 
  RGBMODE_REVERSESHIFT,
};
// constant hue and saturation values for front and rear lights
const byte WHITE_HUE = 0;
const byte WHITE_SATURATION = 0;
const byte RED_HUE = 0;
const byte RED_SATURATION = 255;

// red, orange, yellow, yellowgreen, green, bluegreen, cyan, blue, violet, purple, white, black
// constant values for changing hue and saturation
const byte HUE_VALUES[] = {0, 24, 48, 80, 96, 128, 144, 160, 192, 224, 255, 0};
const byte WHITE_HUE_INDEX = 10;
const byte BLACK_HUE_INDEX = WHITE_HUE_INDEX + 1;
const byte BRIGHTNESS_VALUES[] = {0, 80, 160, 250};

/**
 * curMode - either MODE_NORM or MODE_NORMPLUSRGB
 *    MODE_NORM - only front and rear lights on
 *    MODE_NORMPLUSRGB - front, rear, and RGB lights on
 * curBrightness - global brightness of all lights
 *    - either PWR_OFF, PWR_LOW, PWR_MED, or PWR_HIGH
 * curColors - array of colors in that mode
 *    - can contain anywhere between 0 to 10 colors, indices inside HUE_VALUES
 *    - length of colors is in lenColors
 * curRGBMode - current RGB mode
 *    - can be either RGBMODE_CONSTANT, RGBMODE_SINGLEFLASH, RGBMODE_DOUBLEFLASH,
 *      RGBMODE_SINGLEFADE, RGBMODE_DOUBLEFADE, RGBMODE_FORWARDSHIFT, 
 *      RGBMODE_REVERSESHIFT
 */ 
struct ledsConfig {
  byte curMode;
  byte curBrightness;
  byte curColors[10];
  int lenColors;
  byte curRGBMode;
  byte checkByte;
};

#ifdef AVR
  // 1 button
  const int BTN1_PIN = 2;
  // three strings of LEDs: front, side, and rear
  const int LED_PIN = 4;
#endif 
#ifdef ESP32
  // 1 button
  const int BTN1_PIN = 18;
  // three strings of LEDs: front, side, and rear
  const int LED_PIN = 2;
#endif 

// control button
InterruptButton btn1(BTN1_PIN);
// FastLED stuff
const int NUM_LEDS = 8;
const int BRIGHTNESS = 250;
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define UPDATES_PER_SECOND 100
CRGB leds[NUM_LEDS];
CRGB* frontLeds = &leds[0];
CRGB* rgbLeds = &leds[NUM_LEDS/2];

// update period for fading modes
unsigned int updatePeriodinMillis = 5;
// period length for all RGB modes
int totalPeriodLengthinMillis = 1000;
// keypoints per RGB mode cycle that determine the behavior for each RGB mode
unsigned int keyPoints[6];
// flashCycleTimer - used to keep time in RGB modes
unsigned long flashCycleTimer;
/**
 * ctr1 is a counter variable used to animate the LEDs.
 * ctr1 is measured in multiples of updatePeriodinMillis milliseconds, which varies 
 * depending on the RGB mode. For example, it could be in multiples of 5 ms for fading, or 100 ms
 * for flashing.
 */
unsigned int ctr1;

ledsConfig* configuration;
byte buff[sizeof(ledsConfig)];
unsigned long lastTimeConfigChanged;
bool configChanged;
const unsigned long AUTOSAVE_DELAY = 20000;
const int configAddr = 0x00;

/**
 * current hue and saturation variables to be written to LEDs
 */
byte curHueIndex;
byte curHueVal;
byte curSaturationVal;
byte curBrightnessVal;

// btn1 interrupt function
void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void printConfiguration() {
  #ifdef ESP32
    Serial.printf("LED configuration:\n");
    Serial.printf("configuration->curmode=%d\n", configuration->curMode);
    Serial.printf("configuration->curBrightness=%d\n", configuration->curBrightness);
    Serial.printf("configuration->lenColors=%d\n", configuration->lenColors);
    Serial.printf("configuration->curColors");
    for (int i=0;i<configuration->lenColors;i++) {
      Serial.printf("%d, ", configuration->curColors[i]);
    }
    Serial.println();
    Serial.printf("configuration->curRGBMode=%d\n", configuration->curRGBMode);
    Serial.printf("configuration->checkByte=%d\n", configuration->checkByte);
  #endif
}

/**
 * Load LED config from EEPROM
 */
void loadConfiguration() {
  Serial.println("load");
  #ifdef AVR
    EEPROM.get(configAddr, configuration);
  #endif 
  #ifdef ESP32
    const int lCLen = prefs.getBytesLength("lC");
    Serial.printf("lCLen = %d\n", lCLen);
    prefs.getBytes("lC", buff, lCLen);
    configuration = (ledsConfig *) buff;
  #endif
}

/**
 * save LED config to EEPROM
 */
void saveConfiguration() {
  Serial.println("save");
  #ifdef AVR
    EEPROM.put(configAddr, configuration);
  #endif 
  #ifdef ESP32
    prefs.putBytes("lC", configuration, sizeof(ledsConfig));
  #endif
}

void activateAutoSave() {
  lastTimeConfigChanged = millis();
  configChanged = true;
}

/**
 * loop function to check for changes and autosave 
 */
void checkAutoSaveToEEPROM() {
  if (millis() - lastTimeConfigChanged > AUTOSAVE_DELAY && configChanged) {
    configChanged = false;
    Serial.println("autosave");
    saveConfiguration();
  }
}

/**
 * single click - switch brightness between off, low, med, and high
 */
void btn1_1shortclick_func() {
  activateAutoSave();
  configuration->curBrightness++;
  // Serial.println(configuration->curBrightness > PWR_HIGH);
  if (configuration->curBrightness > PWR_HIGH) configuration->curBrightness = PWR_OFF;
  Serial.print("curBrightness = ");
  Serial.println(configuration->curBrightness);
}

/**
 * double click - switch operation mode between norm and normplusrgb
 */
void btn1_2shortclicks_func() {
  activateAutoSave();
  configuration->curMode++;
  if (configuration->curMode > MODE_NORMPLUSRGB) configuration->curMode = 0;
  Serial.print("curMode = ");
  Serial.println(configuration->curMode);
}

/**
 * single long press - cycle through colors
 */
void btn1_1longpress_func() {
  activateAutoSave();
  configuration->lenColors = 1;
  configuration->curColors[0]++;
  if (configuration->curColors[0] > WHITE_HUE_INDEX) configuration->curColors[0] = 0;
  Serial.print("configuration->curColors[0] = ");
  Serial.println(configuration->curColors[0]);
}

/**
 * double long press - cycle between constant, single flash, double flash, single fade, and double fade
 */
void btn1_2longpress_func() {
  activateAutoSave();
  configuration->curRGBMode++;
  if (configuration->curRGBMode > 6) configuration->curRGBMode = 0;
  Serial.print("configuration->curRGBMode = ");
  Serial.println(configuration->curRGBMode);
}

/**
 * control front white and rear red LEDs
 */
void controlfrLEDs() {
  for (int i=0;i<NUM_LEDS/2;i++) {
    frontLeds[i] = CHSV(WHITE_HUE, WHITE_SATURATION, BRIGHTNESS_VALUES[configuration->curBrightness]);
  }
}

/**
 * turn off RGB LEDs
 */
void offLEDs() {
  for (int i=0;i<NUM_LEDS/2;i++) {
    rgbLeds[i] = CHSV(0, 0, 0);
  }
}

/**
 * constant on RGB LEDs
 */
void constantLEDs() {
  curSaturationVal = configuration->curColors[0] == WHITE_HUE_INDEX? 0 : 255;
  curHueVal = HUE_VALUES[configuration->curColors[0]];
  curBrightnessVal = BRIGHTNESS_VALUES[configuration->curBrightness];
  for (int i=0;i<NUM_LEDS/2;i++) {
    rgbLeds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
  }
}

/**
 * single flash RGB LEDs
 */
void singleFlashLEDs() {
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300/updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis/updatePeriodinMillis;
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) curBrightnessVal = BRIGHTNESS_VALUES[configuration->curBrightness];
    else curBrightnessVal = 0;
    curHueIndex = curHueIndex > configuration->lenColors-1? 0: curHueIndex;
    curHueVal = HUE_VALUES[configuration->curColors[curHueIndex]];
    curSaturationVal = configuration->curColors[curHueIndex] == WHITE_HUE_INDEX? 0 : 255;
    if (ctr1 > keyPoints[2] - 1) {
      curHueIndex++;
    }
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1 + 1;
    for (int i=0;i<NUM_LEDS/2;i++) {
      rgbLeds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

/**
 * double flash RGB LEDs
 */
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
    if ((ctr1 < keyPoints[1]) || (ctr1 >= keyPoints[2] && ctr1 < keyPoints[3])) curBrightnessVal = BRIGHTNESS_VALUES[configuration->curBrightness];
    else curBrightnessVal = 0;
    curHueIndex = curHueIndex > configuration->lenColors-1? 0: curHueIndex;
    curHueVal = HUE_VALUES[configuration->curColors[curHueIndex]];
    curSaturationVal = configuration->curColors[curHueIndex] == WHITE_HUE_INDEX? 0 : 255;
    if (ctr1 > keyPoints[4] - 1) {
      curHueIndex++;
    }
    ctr1 = ctr1 > keyPoints[4] - 1? 0:ctr1 + 1;
    for (int i=0;i<NUM_LEDS/2;i++) {
      rgbLeds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

/**
 * single fade RGB LEDs
 */
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
      curBrightnessVal = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      curBrightnessVal = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[2]) {
      curBrightnessVal = 0;
    }
    curHueIndex = curHueIndex > configuration->lenColors-1? 0: curHueIndex;
    curHueVal = HUE_VALUES[configuration->curColors[curHueIndex]];
    curSaturationVal = configuration->curColors[curHueIndex] == WHITE_HUE_INDEX? 0 : 255;
    if (ctr1 > keyPoints[3] - 1) {
      curHueIndex++;
    }
    ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1 + 1;
    for (int i=0;i<NUM_LEDS/2;i++) {
      rgbLeds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

/**
 * double fade RGB LEDs
 */
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
      curBrightnessVal = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    }
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      curBrightnessVal = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    if (ctr1 >= keyPoints[2] && ctr1 < keyPoints[3]) {
      curBrightnessVal = sin8((ctr1-keyPoints[2])*64/(keyPoints[3] - keyPoints[2])) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[3] && ctr1 < keyPoints[4]) {
      curBrightnessVal = sin8((ctr1-keyPoints[3])*64/(keyPoints[4] - keyPoints[3])+64) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[4]) {
      curBrightnessVal = 0;
    }
    curHueIndex = curHueIndex > configuration->lenColors-1? 0: curHueIndex;
    curHueVal = HUE_VALUES[configuration->curColors[curHueIndex]];
    curSaturationVal = configuration->curColors[curHueIndex] == WHITE_HUE_INDEX? 0 : 255;
    if (ctr1 > keyPoints[5] - 1) {
      curHueIndex++;
    }
    ctr1 = ctr1 > keyPoints[5] - 1? 0:ctr1 + 1;
    for (int i=0;i<NUM_LEDS/2;i++) {
      rgbLeds[i] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

/**
 * shift forward/backward RGB LEDs 
 */
void shiftLEDs(bool forward=true) {
  // left/right shift multiple colors throughout the LED array
  /**
   * shift the LED pixels every 100 ms
   * 4 pixels red, 4 pixels green, 4 pixels blue 
  */ 
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  for (int i=0;i<3;i++) {
    keyPoints[i+1] = keyPoints[i] + 400/updatePeriodinMillis;
  }
  
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    ctr1++;
    if (!(ctr1 % 4)) {
      curHueIndex++;
    }
    // chase single colors with black
    if (configuration->lenColors <= 1 ) {
      configuration->curColors[1] = BLACK_HUE_INDEX;
      curHueIndex = curHueIndex > 1 ?0: curHueIndex;
    }
    else {
      curHueIndex = curHueIndex > (configuration->lenColors - 1)?0: curHueIndex;
    }
    curHueVal = HUE_VALUES[configuration->curColors[curHueIndex]];
    if (configuration->curColors[curHueIndex] == BLACK_HUE_INDEX) {
      curBrightnessVal = 0;
    }
    else if (configuration->curColors[curHueIndex] == WHITE_HUE_INDEX) {
      curBrightnessVal = BRIGHTNESS_VALUES[configuration->curBrightness];
      curSaturationVal = 0;
    }
    else {
      curBrightnessVal = BRIGHTNESS_VALUES[configuration->curBrightness];
      curSaturationVal = 255;
    }

    if (forward) {
      // shift LEDs with the flow of data
      for (int i=NUM_LEDS/2 - 1;i>0;i--) {
        rgbLeds[i] = rgbLeds[i-1];
      }
      rgbLeds[0] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    } 
    else {
      // shift LEDs against the flow of data
      for (int i=0;i<NUM_LEDS/2 - 1;i++) {
        rgbLeds[i] = rgbLeds[i+1];
      }
      rgbLeds[NUM_LEDS/2 - 1] = CHSV(curHueVal, curSaturationVal, curBrightnessVal);
    }
  }
}

/**
 * LED control loop for all LEDs
 */
void ledLoop() {
  controlfrLEDs();
  if (configuration->curMode == MODE_NORMPLUSRGB) {
    switch (configuration->curRGBMode) {
      case RGBMODE_SINGLEFLASH:
        singleFlashLEDs();
        break;
      case RGBMODE_DOUBLEFLASH:
        doubleFlashLEDs();
        break;
      case RGBMODE_SINGLEFADE:
        singleFadeLEDs();
        break;
      case RGBMODE_DOUBLEFADE:
        doubleFadeLEDs();
        break;
      case RGBMODE_FORWARDSHIFT:
        shiftLEDs(true);
        break;
      case RGBMODE_REVERSESHIFT:
        shiftLEDs(false);
        break;
      default:
        constantLEDs();
    }
  } else {
    offLEDs();
  }
  FastLED.show();
}

void setup() {
  #ifdef ESP32
    prefs.begin("lC");
  #endif
  Serial.begin(115200);
  Serial.println("RESET");
  configuration = (ledsConfig *) buff;
  btn1.begin(btn1_change_func);
  // single click - cycle between off, low, medium, and high
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  // double click - cycle between driving lights only and driving lights + RGB 
  btn1.set2ShortPressFunc(btn1_2shortclicks_func);
  // single long press - cycle through preset RGB solid colors
  btn1.set1LongPressFunc(btn1_1longpress_func);
  // double long press - between constant, single flash, double flash, single fade, and double fade
  btn1.set2LongPressFunc(btn1_2longpress_func);
  pinMode(LED_BUILTIN, OUTPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  // printConfiguration();
  loadConfiguration();
  // printConfiguration();

 
  printConfiguration();
  // check the checkByte in the config to see if the data is valid
  Serial.print("cb=");
  Serial.println(configuration->checkByte);
  if (configuration->checkByte != 0x0F) {
    configuration->checkByte = 0x0F;
    // configuration->curColors[0] = 0;
    // configuration->curColors[1] = 1;
    // configuration->curColors[2] = 2;
    // configuration->curColors[3] = 4;
    // configuration->curColors[4] = 7;
    // configuration->curColors[5] = 8;
    // configuration->curColors[6] = 10;
    // configuration->lenColors = 7;

    // configuration->curColors[0] = 0;
    // configuration->curColors[1] = 7;
    // configuration->lenColors = 2;
    configuration->curColors[0] = 0;
    configuration->lenColors = 1;
    configuration->curMode = MODE_NORMPLUSRGB;
    configuration->curRGBMode = RGBMODE_SINGLEFADE;
    configuration->curBrightness = PWR_LOW;

    saveConfiguration();
    printConfiguration();
  }
}

void loop() {
  /**
   * button loop and LED loop
   */
  btn1.loop();
  ledLoop();
  checkAutoSaveToEEPROM();
}

