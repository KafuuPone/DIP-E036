// Testing out functions in "button.h"

#include "button.h"

#define BUTTON_PIN 4

Button test_button;

void setup() {
  Serial.begin(115200);

  test_button.begin(BUTTON_PIN);
}

void loop() {
  test_button.update();

  if(test_button.debounce()) {
    Serial.println("Pressed.");
  }
  if(test_button.release()) {
    Serial.println("Released.");
  }
  if(test_button.start_longpress()) {
    Serial.println("Longpress enabled.");
  }
  // uint16_t state = test_button.state();
  // for (int i = 15; i >= 0; i--) {
  //     // Check if the i-th bit is set
  //     if (state & (1 << i)) {
  //         Serial.print("1");
  //     } else {
  //         Serial.print("0");
  //     }
  // }
  // Serial.println("");
  delay(1);
}