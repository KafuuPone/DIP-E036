#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "LiquidCrystal_I2C.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void initialize_bluetooth(){
    Serial.println("switch to bluetooth mode");
    lcd.clear();
    lcd.setCursor(0,0);
    a2dp_sink.start("Wireless_speaker_demo");
    lcd.print("Bluetooth Mode");
} 

void setup() {
    Serial.begin(115200);  // Start serial communication
  // Initialize the LCD
    lcd.init();
    lcd.backlight();

    //Setup the bluetooth configuration
    auto cfg = i2s.defaultConfig(); //set the config to default: 44.1 kHz sample frequency and 16 bits per sample
    cfg.pin_bck = 26; // to BCLK
    cfg.pin_ws = 25; // to LCK
    cfg.pin_data = 22; // to DIN
    i2s.begin(cfg);

    initialize_bluetooth(); 
}

void loop() {
    // Continuously process Bluetooth audio in the background
}