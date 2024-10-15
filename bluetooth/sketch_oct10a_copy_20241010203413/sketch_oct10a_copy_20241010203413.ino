#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void audio_data_callback(const uint8_t *data, uint32_t len) {
    Serial.print("Received audio data: ");
    Serial.println(len);
}

void setup() {
    Serial.begin(115200);  // Start serial communication
    auto cfg = i2s.defaultConfig();
    cfg.pin_bck = 26;
    cfg.pin_ws = 25;
    cfg.pin_data = 22;
    i2s.begin(cfg);

    a2dp_sink.set_stream_reader(audio_data_callback);
    a2dp_sink.start("Wireless_speaker_demo");
}

void loop() {
    // Continuously process Bluetooth audio in the background
}