#include "AudioTools.h"         // AudioTools library for audio processing
#include "BluetoothA2DPSink.h"  // Bluetooth A2DP Sink library

I2SStream i2s;                   // Create an I2S stream object
BluetoothA2DPSink a2dp_sink(i2s); // Create a Bluetooth A2DP sink object

void setup() {
  // Initialize Serial for debugging purposes
  Serial.begin(115200);
  Serial.println("Starting Bluetooth A2DP Sink...");

  // Set up I²S with default pins (BCK = GPIO 14, WS = GPIO 15, DATA = GPIO 22)
  auto config = i2s.defaultConfig(TX_MODE);
  config.sample_rate = 44100;        // Standard audio sample rate (CD quality)
  config.bits_per_sample = 16;       // 16-bit audio (standard for Bluetooth audio)
  config.i2s_format = I2S_PHILIPS_MODE;  // Standard I²S format
  config.use_apll = false;           // Disable APLL (Audio Phase-Locked Loop)
  i2s.begin(config);

  // Start Bluetooth A2DP sink with a custom device name
  a2dp_sink.start("ESP32_Wireless_Speaker");

  Serial.println("Bluetooth A2DP Sink started. Ready to receive audio.");
}

void loop() {
  // The Bluetooth A2DP sink and I²S stream handle everything in the background
  delay(1000);
}