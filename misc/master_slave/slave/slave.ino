// ESP-WROOM-32
#include "Wire.h"
#include "cstring"

#define I2C_DEV_ADDR 0x55

bool device_state = false;

void onRequest() {
  // Info to send to master
  if(device_state) {
    Wire.print('1');
  }
  else {
    Wire.print('0');
  }
}

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
  if (strcmp(temp, "TURN ON") == 0) {
    device_state = true;
  }
  else if (strcmp(temp, "TURN OFF") == 0) {
    device_state = false;
  }
  else {
    Serial.print("Invalid signal received: ");
    Serial.println(temp);
  }
}

void setup() {
  Serial.begin(115200);
  
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);

  Wire.begin(I2C_DEV_ADDR);

/*#if CONFIG_IDF_TARGET_ESP32
  char message[64];
  snprintf(message, 64, "%lu Packets.", i++);
  Wire.slaveWrite((uint8_t *)message, strlen(message));
  Serial.print('Printing config %lu', i);
#endif*/
}

void loop() {
  
}