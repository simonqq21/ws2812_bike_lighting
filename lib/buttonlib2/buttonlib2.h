#ifndef INTERRUPT_BUTTON_H
#define INTERRUPT_BUTTON_H
#include <Arduino.h>

#define DEBOUNCE_DELAY 20
#define MULTICLICK_DURATION 500
#define LONGCLICK_DURATION 1000
/**
 * short click: single, double, triple
 * long press: single, double, triple
 */


class InterruptButton {
    public:
        InterruptButton(int pin);
        // void setChangeInterruptFunc(void (*changeInterruptFunc)());
        void begin(void (*changeInterruptFunc)());
        void changeInterruptFunc();
        void set1ShortPressFunc(void (*func)() = NULL);
        void set2ShortPressFunc(void (*func)() = NULL);
        void set3ShortPressFunc(void (*func)() = NULL);
        void set1LongPressFunc(void (*func)() = NULL);
        void set2LongPressFunc(void (*func)() = NULL);
        void set3LongPressFunc(void (*func)() = NULL);
        void loop();
    private:
        int _pin;
        bool _curState;
        bool _changed;
        unsigned long _lastDebounceTime;
        unsigned long _lastClickTime;
        int _numClicks;
        void (*_1ShortPressFunc)();
        void (*_2ShortPressFunc)();
        void (*_3ShortPressFunc)();
        void (*_1LongPressFunc)();
        void (*_2LongPressFunc)();
        void (*_3LongPressFunc)();
};

#endif

