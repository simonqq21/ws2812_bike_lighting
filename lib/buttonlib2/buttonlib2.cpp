#include "buttonlib2.h"

InterruptButton::InterruptButton(int pin) {
    _pin = pin;
}

void InterruptButton::changeInterruptFunc() {
    _changed = true;
    _lastDebounceTime = millis();
}

// void InterruptButton::setChangeInterruptFunc(void (*changeInterruptFunc)()) {
//     attachInterrupt(digitalPinToInterrupt(_pin), changeInterruptFunc, CHANGE);
// }

void InterruptButton::begin(void (*changeInterruptFunc)()) {
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_pin), changeInterruptFunc, CHANGE);
}

void InterruptButton::set1ShortPressFunc(void (*func)()) {
    _1ShortPressFunc = func;
}

void InterruptButton::set2ShortPressFunc(void (*func)()) {
    _2ShortPressFunc = func;
}

void InterruptButton::set3ShortPressFunc(void (*func)()) {
    _3ShortPressFunc = func;
}

void InterruptButton::set1LongPressFunc(void (*func)()) {
    _1LongPressFunc = func;
}

void InterruptButton::set2LongPressFunc(void (*func)()) {
    _2LongPressFunc = func;
}

void InterruptButton::set3LongPressFunc(void (*func)()) {
    _3LongPressFunc = func;
}

void InterruptButton::loop() {
    if (_changed && millis() - _lastDebounceTime > DEBOUNCE_DELAY) {
        _curState = digitalRead(_pin);
        /**
         * when pressed, increment the btn click count if within the multiclick delay.
         */
        if (!_curState) {
            // Serial.print("btn pressed ");
            _lastClickTime = millis();
            _numClicks++;
        }
        // else Serial.print("btn released ");
        _changed = false;
    }

    if (_numClicks) {
        if (millis() - _lastClickTime > MULTICLICK_DURATION) {
            if (digitalRead(_pin)) {
                // Serial.print("button clicked ");
                // Serial.print(_numClicks);
                // Serial.println(" times.");
                switch (_numClicks) {
                    case 1: 
                        if (_1ShortPressFunc != NULL) {
                            _1ShortPressFunc();
                            break;
                        }             
                    case 2: 
                    if (_2ShortPressFunc != NULL) {
                        _2ShortPressFunc();
                        break;
                    }
                    default: 
                    if (_3ShortPressFunc != NULL) {
                        _3ShortPressFunc();
                    }
                }
                _numClicks = 0; 
            }
        }
        if (millis() - _lastClickTime > LONGCLICK_DURATION) {
            if (!digitalRead(_pin)) {
                // Serial.print("button long clicked ");
                // Serial.print(_numClicks);
                // Serial.println(" times.");
                switch (_numClicks) {
                    case 1: 
                        if (_1LongPressFunc != NULL) {
                            _1LongPressFunc();
                            break;
                        }
                    case 2: 
                        if (_2LongPressFunc != NULL) {
                            _2LongPressFunc();
                            break;   
                        }
                    default: 
                        if (_3LongPressFunc != NULL) {
                            _3LongPressFunc();
                        }
                }
                _numClicks = 0; 
            }
        }
    }
}