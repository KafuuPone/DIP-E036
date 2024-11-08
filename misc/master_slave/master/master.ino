// ESP32S3
#include "Wire.h"
#include "button.h"

#define I2C_DEV_ADDR 0x55

bool master_status = false;
char slave_status = '0';

unsigned long loop_num = 0;
Button main_button;

void bluetooth_on() {
  Wire.beginTransmission(I2C_DEV_ADDR);
  Wire.printf("TURN ON");
  Wire.endTransmission();
}

void bluetooth_off() {
  Wire.beginTransmission(I2C_DEV_ADDR);
  Wire.printf("TURN OFF");
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);
  
  // Setup button on pin 4
  main_button.begin(4);
  
  // Starts I2C on default pins
  Wire.begin();
}

void loop() {
  main_button.update();

  // If button detected
  // Send info to slave, toggle internal button state
  if(main_button.release()) {
    Serial.println("Button pressed");
    master_status = !master_status;

    if(master_status) {
      bluetooth_on();
    }
    else {
      bluetooth_off();
    }
  }

  // Read 1 char byte from the slave
  uint8_t bytesReceived = Wire.requestFrom(I2C_DEV_ADDR, 1);

  //If received more than zero bytes
  if ((bool)bytesReceived) {
    uint8_t temp[bytesReceived];
    Wire.readBytes(temp, bytesReceived);
    
    slave_status = temp[0];
  }

  // Indicator of slave status, should be either '1' and '0'
  if(loop_num % 10 == 0) {
    Serial.println(slave_status);
  }

  loop_num += 1;
  delay(10);
}