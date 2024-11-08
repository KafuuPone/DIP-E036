#ifndef button_h
#define button_h

class Button {
  private:
    uint8_t btn; // pin number
    uint8_t btn_state;
    unsigned long last_debounce_time = 0;
    bool inverted = true; // default is inverted input, 0 is pressed, 1 is released
    // basic long press - 0.5s
    bool longpress_state = false;
    bool prev_longpress_state = false;
    bool start_longpress_state = false;
    // custom long press - 5s
    bool custom_longpress_state = false;
    bool prev_custom_longpress_state = false;
    bool start_custom_longpress_state = false;

  public:
    // initialize button
    void begin(uint8_t button, bool inverted_input = true) {
      btn = button;
      btn_state = 0xff;
      pinMode(btn, INPUT_PULLUP);
      inverted = inverted_input;
    }

    // update btn_state
    void update() {
      // update shift state
      bool button_input;
      if(inverted) {
        button_input = digitalRead(btn);
      }
      else {
        button_input = !digitalRead(btn);
      }
      btn_state = (btn_state << 1) | button_input | 0b11111100;

      // if debounce(off->on), update
      if(debounce()) {
        last_debounce_time = millis();
      }

      // if in long press mode, update (here we set duration to 500ms)
      longpress_state = (fully_pressed() && (millis() - last_debounce_time > 500));
      // check if it toggled
      start_longpress_state = (prev_longpress_state == false) && (longpress_state == true);
      // update previous state
      prev_longpress_state = longpress_state;

      // custom timing
      // if in long press mode, update (here we set duration to 500ms)
      custom_longpress_state = (fully_pressed() && (millis() - last_debounce_time > 5000));
      // check if it toggled
      start_custom_longpress_state = (prev_custom_longpress_state == false) && (custom_longpress_state == true);
      // update previous state
      prev_custom_longpress_state = custom_longpress_state;
    }

    //-----------NORMAL BUTTONS-------------//

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

    // true if custom long press is activated
    bool start_custom_longpress() {
      return start_custom_longpress_state;
    }

};

#endif