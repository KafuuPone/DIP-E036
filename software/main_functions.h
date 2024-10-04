#ifndef main_functions_h
#define main_functions_h

#include "Wire.h"                 // I2C Communication
#include "nvs_flash.h"
#include "nvs.h"                  // File storage (Non volatile storage)
#include "LiquidCrystal_I2C.h"    // LCD I2C library

#include "constants.h"

// difference between 2 numbers
int difference(int a, int b) {
  if(a > b) {
    return a - b;
  }
  else {
    return b - a;
  }
}

// round to integer
int round_int(float x) {
  if(x - (int)x < 0.5) {
    return (int)x;
  }
  else {
    return (int)x + 1;
  }
}

// Conversion from frequency to individual bits
uint8_t freq_byte1(int frequency) {
  uint8_t channel = frequency - FREQ_MIN;
  return (channel >> 2);
}
uint8_t freq_byte2(int frequency) {
  uint8_t channel = frequency - FREQ_MIN;
  return (channel & 0b11) << 6; // gets last 2 bits, then shifts left 6 bits
}

// Changing frequency in datastream (frequency = MHz / 0.1MHz) (arr=tuned_config[])
void change_freq(uint8_t* arr, int frequency) {
  if(FREQ_MIN <= frequency && frequency <= FREQ_MAX) {
    arr[2] = freq_byte1(frequency);
    arr[3] = freq_byte2(frequency) | (arr[3] & 0b111111); // (2 bits frequency and 6 empty bits) OR (last 6 bits of original 4th bit of arr)


    Wire.beginTransmission(RDA5807M_ADDRESS);
    Wire.write(arr, 4);
    Wire.endTransmission();

    Serial.print("Tuned to frequency ");
    Serial.print(frequency / 10); Serial.print("."); Serial.print(frequency % 10); Serial.println("MHz.");
  }
  else {
    Serial.println("[ERROR] change_freq(): Frequency out of range.");
  }
}

// Change volume output for RDA5807 (arr=tuned_config[])
void change_vol(const uint8_t* arr, uint8_t volume) {
  // take last 4 bits
  uint8_t volume_bytes = 0b1111 & volume;

  uint8_t vol_config[] = {
    // register 0x02
    arr[0], arr[1],
    // register 0x03
    arr[2], arr[3],
    // register 0x04
    0b00001110, 0b00000000,
    // register 0x05
    0b10001000, volume_bytes | 0b10000000
    // VOLUME(last 4 bits, 0000-1111)
  };

  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.write(vol_config, 8);
  Wire.endTransmission();

  Serial.print("Changed volume to "); Serial.println(volume);
}

// Autotune command for RDA5807 (arr=tuned_config[])
void autotune(const uint8_t* arr, bool seekup) {
  uint8_t seekup_config[] = {seekup ? 0b11000011 : 0b11000001, arr[1]};

  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.write(seekup_config, 2);
  Wire.endTransmission();

  Serial.println(seekup ? "Autotune up started." : "Autotune down started.");
}

// save frequency to storage (arr=saved_channels[], frequency=curr_freq/curr_vol (depends on use case))
void save_channel(int* arr, int frequency, int chn_num) {
  // saving channels
  if(1 <= chn_num && chn_num <= 6) {
    uint8_t channel = frequency - FREQ_MIN;

    // check if this frequency is saved to other channels
    for(int i=1; i<=6; i++) {
      // remove this frequency from old channel num
      if((i != chn_num) && (arr[i-1] == channel)) {
        const char key[] = {'0' + i, '\0'}; // strings "1" to "6"

        // Temporal data
        arr[i-1] = 0xff;

        // Initialize NVS
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);

        // Open NVS storage with read/write access
        nvs_handle_t nvs_handle;
        err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
        ESP_ERROR_CHECK(err);

        // Write a byte to NVS
        err = nvs_set_u8(nvs_handle, key, 0xff);
        ESP_ERROR_CHECK(err);

        // Commit the changes
        err = nvs_commit(nvs_handle);
        ESP_ERROR_CHECK(err);

        // Close NVS handle
        nvs_close(nvs_handle);

        Serial.print(0xff); Serial.print(" saved to channel "); Serial.println(chn_num);
      }
    }

    // save the frequency to new channel num, if the new channel num has another freq
    if (arr[chn_num-1] != channel) {
      const char key[] = {'0' + chn_num, '\0'}; // strings "1" to "6"

      // Temporal data
      arr[chn_num-1] = channel;

      // Initialize NVS
      esp_err_t err = nvs_flash_init();
      if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          err = nvs_flash_init();
      }
      ESP_ERROR_CHECK(err);

      // Open NVS storage with read/write access
      nvs_handle_t nvs_handle;
      err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
      ESP_ERROR_CHECK(err);

      // Write a byte to NVS
      err = nvs_set_u8(nvs_handle, key, channel);
      ESP_ERROR_CHECK(err);

      // Commit the changes
      err = nvs_commit(nvs_handle);
      ESP_ERROR_CHECK(err);

      // Close NVS handle
      nvs_close(nvs_handle);

      Serial.print(channel); Serial.print(" saved to channel "); Serial.println(chn_num);
    }
  }
  // saving current frequency as last session frequency
  else if(chn_num == 7) {
    uint8_t channel = frequency - FREQ_MIN;

    if (arr[chn_num-1] != channel) {
      const char key[] = "7";

      // Temporal data
      arr[chn_num-1] = channel;

      // Initialize NVS
      esp_err_t err = nvs_flash_init();
      if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          err = nvs_flash_init();
      }
      ESP_ERROR_CHECK(err);

      // Open NVS storage with read/write access
      nvs_handle_t nvs_handle;
      err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
      ESP_ERROR_CHECK(err);

      // Write a byte to NVS
      err = nvs_set_u8(nvs_handle, key, channel);
      ESP_ERROR_CHECK(err);

      // Commit the changes
      err = nvs_commit(nvs_handle);
      ESP_ERROR_CHECK(err);

      // Close NVS handle
      nvs_close(nvs_handle);

      Serial.print(channel); Serial.print(" saved to channel "); Serial.println(chn_num);
    }
  }
  // saving current volume as last current volume
  else if(chn_num == 8) {
    uint8_t volume = frequency;

    if(arr[chn_num-1] != volume) {
      const char key[] = "8";

      // Temporal data
      arr[chn_num-1] = volume;

      // Initialize NVS
      esp_err_t err = nvs_flash_init();
      if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          err = nvs_flash_init();
      }
      ESP_ERROR_CHECK(err);

      // Open NVS storage with read/write access
      nvs_handle_t nvs_handle;
      err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
      ESP_ERROR_CHECK(err);

      // Write a byte to NVS
      err = nvs_set_u8(nvs_handle, key, volume);
      ESP_ERROR_CHECK(err);

      // Commit the changes
      err = nvs_commit(nvs_handle);
      ESP_ERROR_CHECK(err);

      // Close NVS handle
      nvs_close(nvs_handle);

      Serial.print(volume); Serial.print(" saved to channel "); Serial.println(chn_num);
    }
  }
  else {
    Serial.println("[ERROR] save_channel(): Channel number out of range.");
  }
}

// reads channel number from storage (channel = frequency - FREQ_MIN)
uint8_t read_channel(int chn_num) {
  if(1 <= chn_num && chn_num <= 8) {
    const char key[] = {'0' + chn_num, '\0'};
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open NVS storage with read access
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    ESP_ERROR_CHECK(err);

    // Setup variables
    uint8_t my_byte = 0;
    uint8_t output;
    // set default value
    if(chn_num <= 6) {
      output = 0xff;
    }
    else if(chn_num == 7) {
      output = FREQ_DEFAULT - FREQ_MIN;
    }
    else {
      output = VOL_DEFAULT;
    }

    // read the value from NVS
    err = nvs_get_u8(nvs_handle, key, &my_byte);
    if (err == ESP_OK) {
      output = my_byte;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
      Serial.println("NVS not found.");
    } else {
      Serial.println("Error reading from NVS.");
    }

    // Close NVS handle
    nvs_close(nvs_handle);

    Serial.print(key); Serial.print(" read: "); Serial.println(output);
    return output;
  }
}

// requests registry 0x0A onwards from RDA5807 module (arr=requested_data)
void request_data(uint8_t* arr) {
  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.requestFrom(RDA5807M_ADDRESS, 12);
  
  for(int i=0; i<12; i++) {
    arr[i] = Wire.read();
  }

  Wire.endTransmission();
}

// determine if device is seeking (arr=requested_data) *CHANGING VOLUME also triggers it
bool seeking(const uint8_t* arr) {
  uint8_t byte1 = arr[0];

  // SEEK-TUNE complete: 2nd bit, if completed = 1
  return ((byte1 & 0b01000000) != 0b01000000);
}

// update frequency from device (arr=requested_data, global_freq=&curr_freq)
void update_freq(const uint8_t* arr, int* global_freq) {
  // byte 2
  int channel = arr[1];
  int frequency = channel + FREQ_MIN;

  // update current frequency
  *global_freq = frequency;
}

// display frequency on top(lcd_ptr = &lcd)
void display_freq(int frequency, LiquidCrystal_I2C* lcd_ptr) {
  (*lcd_ptr).setCursor(3, 0);
  if(frequency < 1000) {
    (*lcd_ptr).print(" ");
  }

  // print frequency (xxx.xMHz)
  (*lcd_ptr).print(frequency / 10);
  (*lcd_ptr).print(".");
  (*lcd_ptr).print(frequency % 10);
  (*lcd_ptr).print("MHz");
}

// display signal strength on top (arr=requested_data)
void display_signal(const uint8_t* arr, LiquidCrystal_I2C* lcd_ptr) {
  // get high byte of 0x0B
  uint8_t byte3 = arr[2];

  // whether is it tuned to station
  bool station = ((byte3 & 0b1) == 0b1);
  uint8_t strength = byte3 >> 1; // maximum is 0b1111111, RSSI

  // only display signal if tuned to station
  (*lcd_ptr).setCursor(12, 0);
  if(station) {
    //  0- 42 1 block
    // 43- 84 2 block
    // 85-127 3 block(full)
    (*lcd_ptr).write(0); // antenna symbol
    (*lcd_ptr).write(1); // 1 block
    // 2 block
    if(strength > 42) {
      (*lcd_ptr).write(2);
    }
    else {
      (*lcd_ptr).print(" ");
    }
    // 3 block
    if(strength > 84) {
      (*lcd_ptr).write(3);
    }
    else {
      (*lcd_ptr).print(" ");
    }
  }
  else {
    (*lcd_ptr).print("    ");
  }
}

// clear RDS radiotext, default char is space (arr_A=radiotext_A, arr_B=radiotext_B)
void clear_radiotext(char* arr_A, char* arr_B, char version = ' ') {
  // version A
  if(version != 'B') {
    for(int i=0; i<64; i++) {
      arr_A[i] = ' ';
    }
  }
  // version B
  if(version != 'A') {
    for(int i=0; i<32; i++) {
      arr_B[i] = ' ';
    }
  }
}

// void radiotext byte to char
char rds_byte_to_char(uint8_t input) {
  char output;
  if(input == 0x0d) {
    output = '\n';
  }
  else if ((input >= 0x20) && (input <= 0x7e)) {
    output = (char)input;
  }
  else {
    output = ' ';
  }
  return output;
}

// decode and update RDS radiotext from read data of RDA5807 (arr=requested_data, arr_A=radiotext_A, arr_B=radiotext_B)
void update_radiotext(const uint8_t* arr, char* arr_A, char* arr_B, bool version) {
  uint8_t segment_address = arr[7] & 0b1111;

  // version A radiotext
  if(version == true) {
    for(int i=0; i<4; i++) {
      arr_A[segment_address*4 + i] = rds_byte_to_char(arr[8+i]);
    }
  }
  // version B radiotext
  else {
    for(int i=0; i<2; i++) {
      arr_B[segment_address*2 + i] = rds_byte_to_char(arr[10+i]);
    }
  }
}

// converts analog signal from knob to frequency, resolution to closest 1MHz since knob is too sensitive
int convert_freq(int analog) {
  return 10 * round_int((((FREQ_MAX - FREQ_MIN) * (float)analog) / AMAX + FREQ_MIN) / 10);
}

// converts analog signal from knob to volume
int convert_vol(int analog) {
  return round_int((15.0 * analog) / AMAX);
}

#endif