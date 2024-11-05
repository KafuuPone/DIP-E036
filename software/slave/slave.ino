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
// Device name
#define BT_DEVICE_NAME "DIP-E036 Speaker"

// Bluetooth audio output
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// Bluetooth data to put in datastring to send
bool bluetooth_mode = false;
uint8_t connection_state = 0; // 0 - OFF, 1 - CONNECTED, 2 - DISCONNECTED, 3 - CONNECTING, 4 - DISCONNECTING
uint8_t playback_state = 0; // 0 - STOPPED, 1 - PLAYING, 2 - PAUSED, 3 - FWD SEEK, 4 - REV SEEK, 5 - ERROR
char* device_name = (char*)malloc(1);
char* media_title = (char*)malloc(1);
char* media_artist = (char*)malloc(1);
char* media_album = (char*)malloc(1);

// Datastring to send for I2C
uint8_t* datastring = (uint8_t*)malloc(1);
uint8_t data_length = 0; // total length of datastring
uint8_t packet_num = 0; // total packet number
uint8_t packet_index = 0; // packet index

// Update datastring to send
void update_datastring() {
  // Info to send to master
  // max length for one packet is 32 bits
  // First 2 bits will be packet index and total packet number (0 indexed)
  // example 3 packets: 0/3, 1/3, 2/3
  // The rest will be the data, split using 0x1d as separator
  // Terminator and paddings will be using NULL, 0x00
  data_length = 0;
  data_length += 2; // bluetooth_mode
  data_length += 2; // connection_state
  data_length += 2; // playback_state
  data_length += strlen(device_name) + 1;
  data_length += strlen(media_title) + 1;
  data_length += strlen(media_artist) + 1;
  data_length += strlen(media_album) + 1;
  // determine packet number, each packet contains 30 bytes of metadata
  packet_num = (data_length - 1) / 30 + 1;

  // create datastring to send
  free(datastring);
  datastring = (uint8_t*)malloc(data_length);
  datastring[0] = bluetooth_mode; datastring[1] = 0x1d;
  datastring[2] = connection_state; datastring[3] = 0x1d;
  datastring[4] = playback_state; datastring[5] = 0x1d;
  // device_name + separator
  for(int i=0; i<strlen(device_name); i++) {
    datastring[6 + i] = device_name[i];
  }
  datastring[6 + strlen(device_name)] = 0x1d;
  // media_title + separator
  for(int i=0; i<strlen(media_title); i++) {
    datastring[6 + strlen(device_name) + 1 + i] = media_title[i];
  }
  datastring[6 + strlen(device_name) + 1 + strlen(media_title)] = 0x1d;
  // media_artist + separator
  for(int i=0; i<strlen(media_artist); i++) {
    datastring[6 + strlen(device_name) + 1 + strlen(media_title) + 1 + i] = media_artist[i];
  }
  datastring[6 + strlen(device_name) + 1 + strlen(media_title) + 1 + strlen(media_artist)] = 0x1d;
  // media_album + separator
  for(int i=0; i<strlen(media_album); i++) {
    datastring[6 + strlen(device_name) + 1 + strlen(media_title) + 1 + strlen(media_artist) + 1 + i] = media_album[i];
  }
  datastring[6 + strlen(device_name) + 1 + strlen(media_title) + 1 + strlen(media_artist) + 1 + strlen(media_album)] = 0x1d;
}

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
  // Turn bluetooth on
  if(strcmp(temp, "BLUETOOTH ON") == 0) {
    a2dp_sink.start(BT_DEVICE_NAME);
    bluetooth_mode = true;
    connection_state = 2; // DISCONNECTED
    Serial.println("Bluetooth ON");
  }
  // Turn bluetooth off
  else if(strcmp(temp, "BLUETOOTH OFF") == 0) {
    a2dp_sink.end();
    bluetooth_mode = false;
    connection_state = 0; // OFF
    Serial.println("Bluetooth OFF");
  }
  // Change current packet number, syntax "PACKET<byte>" where <byte> is packet index
  else if(strncmp(temp, "PACKET", 6) == 0) {
    packet_index = (uint8_t)temp[6];
    Serial.printf("Packet number: %d\n", packet_index);
  }
  else {
    Serial.print("Invalid signal received: ");
    Serial.println(temp);
  }
}

// Function when sending info to master
void onRequest() {
  // Update if it is the start of data
  if(packet_index == 0) {
    update_datastring();
  }

  // sending data sequentially
  Wire.write(packet_index); // packet index
  Serial.printf("%02x ", packet_index);
  Wire.write(packet_num); // total packet number
  Serial.printf("%02x ", packet_num);
  // datastring from index 30*packet_index, padding with 0x00
  for(int i=0; i<30; i++) {
    if(30*packet_index + i < data_length) {
      Wire.write(datastring[30*packet_index + i]);
      Serial.printf("%02x ", datastring[30*packet_index + i]);
    }
    else {
      Wire.write(0x00);
      Serial.printf("%02x ", 0x00);
    }
  }
  Serial.println("");
}

// Change in bluetooth connection state
void connection_state_change(esp_a2d_connection_state_t state, void *obj) {
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      Serial.println("Connection state: CONNECTED");
      connection_state = 1;
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

// Change in bluetooth playback state
void playback_state_change(esp_avrc_playback_stat_t state) {
  switch (state) {
    case ESP_AVRC_PLAYBACK_STOPPED:
      Serial.println("Playback state: STOPPED");
      playback_state = 0;
      break;
    case ESP_AVRC_PLAYBACK_PLAYING:
      Serial.println("Playback state: PLAYING");
      playback_state = 1;
      break;
    case ESP_AVRC_PLAYBACK_PAUSED:
      Serial.println("Playback state: PAUSED");
      playback_state = 2;
      break;
    case ESP_AVRC_PLAYBACK_FWD_SEEK:
      Serial.println("Playback state: FWD_SEEK");
      playback_state = 3;
      break;
    case ESP_AVRC_PLAYBACK_REV_SEEK:
      Serial.println("Playback state: REV_SEEK");
      playback_state = 4;
      break;
    case ESP_AVRC_PLAYBACK_ERROR:
      Serial.println("Playback state: ERROR");
      playback_state = 5;
      break;
    default:
      Serial.println("Invalid bluetooth playback state.");
      break;
  }
}

// Change in metadata
void metadata_update(uint8_t id, const uint8_t *data) {
  const char* data_string = (const char*)data;
  // Title
  if(id == ESP_AVRC_MD_ATTR_TITLE) {
    if(strcmp(media_title, data_string) != 0) {
      free(media_title);
      // Allocate new memory for the new string
      media_title = (char*)malloc(strlen(data_string) + 1); // Allocate memory, +1 for the null terminator
      // Copy the new value to global variable
      strcpy(media_title, data_string);

      Serial.print("Media title: ");
      Serial.println(media_title);
    }
  }
  // Artist
  else if(id == ESP_AVRC_MD_ATTR_ARTIST) {
    if(strcmp(media_artist, data_string) != 0) {
      free(media_artist);
      // Allocate new memory for the new string
      media_artist = (char*)malloc(strlen(data_string) + 1); // Allocate memory, +1 for the null terminator
      // Copy the new value to global variable
      strcpy(media_artist, data_string);

      Serial.print("Media artist: ");
      Serial.println(media_artist);
    }
  }
  // Album
  else if(id == ESP_AVRC_MD_ATTR_ALBUM) {
    if(strcmp(media_album, data_string) != 0) {
      free(media_album);
      // Allocate new memory for the new string
      media_album = (char*)malloc(strlen(data_string) + 1); // Allocate memory, +1 for the null terminator
      // Copy the new value to global variable
      strcpy(media_album, data_string);

      Serial.print("Media album: ");
      Serial.println(media_album);
    }
  }
}

// Update device name
void device_name_update() {
  // Get device name
  const char* retrieved_name = a2dp_sink.get_peer_name();
  // If device name is different
  if(strcmp(device_name, retrieved_name) != 0) {
    free(device_name);
    // Allocate new memory for the new string
    device_name = (char*)malloc(strlen(retrieved_name) + 1); // Allocate memory, +1 for the null terminator
    // Copy the new value to global variable
    strcpy(device_name, retrieved_name);

    Serial.print("Device name: ");
    Serial.println(device_name);
  }
}

void setup() {
  // Initialize serial output
  Serial.begin(115200);

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
    ESP_AVRC_MD_ATTR_ALBUM
  );

  // Global variable async updates
  a2dp_sink.set_on_connection_state_changed(connection_state_change); // Connection state changed
  a2dp_sink.set_avrc_rn_playstatus_callback(playback_state_change); // Playback state changed
  a2dp_sink.set_avrc_metadata_callback(metadata_update); // Metadata state changed

  // I2C config
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin(SLAVE_ADDRESS);

  // // TEMPORARY, TESTING PURPOSES
  // temp_btn.begin(TEMP_BTN);
  // a2dp_sink.start("Wireless_speaker");
  // bluetooth_mode = true;
  // connection_state = 2; // DISCONNECTED
}

void loop() {
  if(connection_state == 1) {
    device_name_update();
  }
  delay(100);
}