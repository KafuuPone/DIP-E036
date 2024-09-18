#include <LiquidCrystal_I2C.h>

// Initialize the I2C LCD (Set the LCD address as per your module, typically 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD

int time_passed = 0;

void setup() {
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  

  // Print a welcome message
  lcd.setCursor(0, 0);
  lcd.print("welcome to E036");
  delay(2000);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("FM  Receiver");
  lcd.setCursor(0,1);
  lcd.print("Frequency: 100.00 MHz");
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time passed:");
  lcd.setCursor(0, 1);
  lcd.print(time_passed);
  lcd.print(" seconds");
  time_passed += 1;
  delay(1000);
}