#ifndef button_h
#define button_h

#include "Arduino.h"

class Button {
  private:
    uint8_t btn; // pin number
    uint8_t btn_state;
    unsigned long last_debounce_time = 0;
    bool longpress_state = false;
    bool prev_longpress_state = false;
    bool start_longpress_state = false;

  public:
    // initialize button
    void begin(uint8_t button) {
      btn = button;
      btn_state = 0xff;
      pinMode(btn, INPUT_PULLUP);
    }

    // update btn_state
    void update() {
      // update shift state
      btn_state = (btn_state << 1) | digitalRead(btn) | 0b11111100;

      // if debounce(off->on), update
      if(debounce()) {
        last_debounce_time = millis();
      }

      // if in long press mode, update (here we set duration to 1s = 1000ms)
      longpress_state = (fully_pressed() && (millis() - last_debounce_time > 500));

      // check if it toggled
      start_longpress_state = (prev_longpress_state == false) && (longpress_state == true);
      // update previous state
      prev_longpress_state = longpress_state;
    }

    // detects for sequence 100000000 for last 9 bits
    // released(1) to pressed(0) transition
    bool debounce() {
      return (btn_state == 0b11111110);
    }
    // detects for sequence 011111111 for last 9 bits
    // pressed(0) to release(1) transition
    bool release() {
      return (btn_state == 0b11111101);
    }

    // true if button is fully pressed
    bool fully_pressed() {
      return (btn_state == 0b11111100);
    }

    // true if long press is activated
    bool start_longpress() {
      return start_longpress_state;
    }

    uint16_t state() {
      return btn_state;
    }
};

#endif