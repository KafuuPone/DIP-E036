// Testing out functions in "button.h"

#include "button.h"

#define BUTTON_PIN 1

Button test_button;

void setup() {
  Serial.begin(115200);

  test_button.begin(BUTTON_PIN);
}

void loop() {
  test_button.update();

  if(test_button.debounce()) {
    Serial.println("Released to pressed.");
  }
  if(test_button.release()) {
    Serial.println("Pressed to released.");
  }
  if(test_button.fully_pressed()) {
    Serial.println("Button pressed.");
  }
  if(test_button.start_longpress()) {
    Serial.println("Longpress enabled.");
  }
}