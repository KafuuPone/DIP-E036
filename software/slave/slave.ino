// ESP-WROOM-32
#include "Wire.h"                 // I2C library
#include "cstring"                // String functions
#include "cstdlib"                // Standrad functions (malloc)
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

// Global states to send
bool bluetooth_mode = false;
uint8_t connection_state = 0; // 0 - OFF, 1 - CONNECTED, 2 - DISCONNECTED, 3 - CONNECTING, 4 - DISCONNECTING
char* device_name = nullptr;
uint8_t audio_state = 0; // 0 - NO AUDIO, 1 - AUDIO ONGOING, 2 - AUDIO SUSPENDED(PAUSED)

// // Function when receiving from master
// void onReceive(int len) {
//   // Character array to hold the received bytes (+1 for null terminator)
//   char temp[len + 1];  // +1 to hold the null terminator
//   int index = 0;

//   while (Wire.available()) {
//     // Read each byte and store it in the temp array
//     temp[index++] = Wire.read();
//   }

//   // Null-terminate the received string
//   temp[len] = '\0';  // Make sure temp is a valid C-style string

//   // Do stuff
//   if (strcmp(temp, "BLUETOOTH ON") == 0) {
//     a2dp_sink.start("Wireless_speaker");
//     bluetooth_mode = true;
//     connection_state = 2; // DISCONNECTED
//   }
//   else if (strcmp(temp, "BLUETOOTH OFF") == 0) {
//     a2dp_sink.end();
//     bluetooth_mode = false;
//     connection_state = 0; // OFF
//   }
//   else {
//     Serial.print("Invalid signal received: ");
//     Serial.println(temp);
//   }
// }

// // Function when sending info to master
// void onRequest() {
//   // Info to send to master
//   Wire.print('1');
// }

// Change in bluetooth connection state
void connection_state_change(esp_a2d_connection_state_t state, void *obj) {
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      Serial.println("Connection state: CONNECTED");
      connection_state = 1;
      device_name_update();
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
      Serial.println("Connection state: DISCONNECTED");
      connection_state = 2;
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      Serial.println("Connection state: CONNECTING");
      connection_state = 3;
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
      Serial.println("Connection state: DISCONNECTING");
      connection_state = 4;
      break;
    default:
      Serial.println("Invalid bluetooth connection state.");
      break;
  }
}

// Change in bluetooth audio state
void audio_state_change(esp_a2d_audio_state_t state, void *obj) {
  switch (state) {
    case ESP_A2D_AUDIO_STATE_STOPPED:
      Serial.println("Audio state: STOPPED");
      audio_state = 0;
      break;
    case ESP_A2D_AUDIO_STATE_STARTED:
      Serial.println("Audio state: STARTED");
      audio_state = 1;
      break;
    case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND:
      Serial.println("Audio state: SUSPENDED");
      audio_state = 2;
      break;
    default:
      Serial.println("Invalid bluetooth audio state.");
      break;
  }
}

// Change in metadata
void metadata_update(uint8_t id, const uint8_t *data) {
  Serial.printf("Metadata update: 0x%x, %s\n", id, data);
}

// Update device name
void device_name_update() {
  // Get device name
  const char* retrieved_device_name = a2dp_sink.get_peer_name();
  // If device name is different
  if(strcmp(device_name, retrieved_device_name) != 0) {
    // Free the memory for original string
    free(device_name);
    // Allocate new memory for the new string
    device_name = (char*)malloc(strlen(retrieved_device_name) + 1); // Allocate memory, +1 for the null terminator
    // Copy the new value to global variable
    strcpy(device_name, retrieved_device_name);

    Serial.print("Device name: ");
    Serial.println(device_name);
  }
}

void setup() {
  // Bluetooth config
  auto cfg = i2s.defaultConfig(); //set the config to default: 44.1 kHz sample frequency and 16 bits per sample
  cfg.pin_bck = BCK;
  cfg.pin_ws = LRCK;
  cfg.pin_data = DIN;
  i2s.begin(cfg);

  // Metadata options
  a2dp_sink.set_avrc_metadata_attribute_mask(
    ESP_AVRC_MD_ATTR_TITLE | 
    ESP_AVRC_MD_ATTR_ARTIST | 
    ESP_AVRC_MD_ATTR_ALBUM |
    ESP_AVRC_MD_ATTR_PLAYING_TIME
  );

  // Global variable async updates
  a2dp_sink.set_on_connection_state_changed(connection_state_change); // Connection state changed
  a2dp_sink.set_on_audio_state_changed(audio_state_change); // Audio state changed
  a2dp_sink.set_avrc_metadata_callback(metadata_update); // Metadata state changed

  // // I2S config
  // Wire.onReceive(onReceive);
  // Wire.onRequest(onRequest);
  // Wire.begin(SLAVE_ADDRESS);

  // TEMPORARY, TESTING PURPOSES
  a2dp_sink.start("Wireless_speaker");
  bluetooth_mode = true;
  connection_state = 2; // DISCONNECTED
}

void loop() {
  // Nothing (for now I hope, since everything can be ran as async)
}