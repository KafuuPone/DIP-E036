// ESP-WROOM-32
#include "Wire.h"                 // I2C library
#include "cstring"
#include "AudioTools.h"           // Digital audio library
#include "BluetoothA2DPSink.h"    // Bluetooth library

// I2C address
#define SLAVE_ADDRESS 0x55
// I2S pins
#define BCK 2
#define LRCK 4
#define DIN 5

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

bool bluetooth_mode = false;

// Function when receiving from master
void onReceive(int len) {
  // Character array to hold the received bytes (+1 for null terminator)
  char temp[len + 1];  // +1 to hold the null terminator
  int index = 0;

  while (Wire.available()) {
    // Read each byte and store it in the temp array
    temp[index++] = Wire.read();
  }

  // Null-terminate the received string
  temp[len] = '\0';  // Make sure temp is a valid C-style string

  // Do stuff
  if (strcmp(temp, "BLUETOOTH ON") == 0) {
    a2dp_sink.start("Wireless_speaker");
    bluetooth_mode = true;
  }
  else if (strcmp(temp, "BLUETOOTH OFF") == 0) {
    a2dp_sink.stop();
    bluetooth_mode = false;
  }
  else {
    Serial.print("Invalid signal received: ");
    Serial.println(temp);
  }
}

// Function when sending info to master
void onRequest() {
  // Info to send to master
  Wire.print('1');
}

void setup() {
  // Bluetooth config
  auto cfg = i2s.defaultConfig(); //set the config to default: 44.1 kHz sample frequency and 16 bits per sample
  cfg.pin_bck = BCK;
  cfg.pin_ws = LRCK;
  cfg.pin_data = DIN;
  i2s.begin(cfg);

  // I2S config
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin(SLAVE_ADDRESS);
}

void loop() {
  if(bluetooth_mode) {
    // Reads if any device is connected

    // If a device is connected
    // Get device name
    // If device is playing song, display song name, author, etc.

    // Update global variable for onRequest() to send
  }

  // delay 1ms
  delay(1);
}