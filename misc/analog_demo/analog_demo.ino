#include "Wire.h"                 // I2C Communication
#include "nvs_flash.h"
#include "nvs.h"                  // File storage (Non volatile storage)
#include "LiquidCrystal_I2C.h"    // LCD I2C library

#include "button.h"               // Button detection and debouncing
#include "lcd_symbols.h"          // Containing custom symbols

// Frequency limits
#define FREQ_MAX 1080
#define FREQ_MIN 870

#define RDA5807M_ADDRESS 0x10
#define LCD_ADDRESS 0x27

// analogRead() highest value, 2^bits-1
#define AMAX 4095

// pin numbers
#define FRQ_TUNE 10
#define VOL_ADJS 11

// Global function declarations
uint8_t freq_byte1(int frequency);
uint8_t freq_byte2(int frequency);
void change_freq(int frequency);
void change_vol(uint8_t volume);
void autotune(bool seekup);
void save_channel(int frequency, int chn_num);
uint8_t read_channel(int chn_num);
void request_data();
bool seeking();
void update_freq();
void display_freq(int frequency);
void display_signal();
void update_radiotext(int mode);
void clear_radiotext(char version = ' ');
char rds_byte_to_char(uint8_t input);
int convert_freq(int analog);
int convert_vol(int analog);
int difference(int a, int b);
int round_int(float x);

// Setup global variables
// float alpha = 0.2;
// int curr_analogfreq; int prev_analogfreq; float temp_analogfreq;
// int curr_analogvol; int prev_analogvol;
unsigned long last_vol_adj = 0;
int saved_channels[] = {read_channel(1), read_channel(2), read_channel(3), read_channel(4), read_channel(5), read_channel(6)};
int curr_freq = 870; // default frequency = 87.0MHz
uint8_t curr_vol = 4; // volume range = 0-15 (RDA5807 only has volume resolution of 0-15)
int prev_freq = curr_freq;

// channel = (frequency in MHz - 87.0) / 0.1
// using 0.1Mhz as channel spacing

// LCD
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2); // 16x2 LCD

// Buttons
Button l_key, r_key;
Button chn_button[6];

// Initialize device
uint8_t init_config[] = {
  // register 0x02
  0b10000000, 0b00001011,
    // DHIZ audio output high-z disable                   1: normal operation
    // DMUTE mute disable                                 0: mute
    // MONO mono select                                   0: stereo
    // BASS bass boost                                    0: disabled
    // RCLK NON-CALIBRATE MODE                            0: RCLK is always supplied
    // RCLK DIRECT INPUT MODE                             0: RCLK is not in direct input mode
    // SEEKUP                                             0: seek down
    // SEEK                                               0: disable/stop seek (when seek operation completes, SEEK->0, STC->1)
    //
    // SKMODE seek mode                                   0: wrap at upper or lower band limit and contiue seeking
    // CLK_MODE clock mode                              000: 32.768kHZ clock rate (match the watch cystal on the module)
    // RDS_EN radio data system enable                    1: enable radio data system
    // NEW_METHOD                                         0: disabled new demodulate method for improved sensitivity (can try enabling)
    // SOFT_RESET                                         1: perform a reset
    // ENABLE power up enable                             1: enabled, device ready to operate

  // register 0x03
  0b00000000, 0b00000000,
    // CHAN channel 8MSB/10                        00000000: initialize at 87.0MHz (channel = 0)
    //
    // CHAN channel 2LSB/10                              00: initialize at 87.0MHz (channel = 0)
    // DIRECT MODE used only when test                    0: disabled by default
    // TUNE tune operation                                0: disable (don't tune to selected channel)
    // BAND band select                                  00: 87–108 MHz (US/Europe)
    // SPACE channel spacing                             00: spacing of 0.1MHz

  // register 0x04
  0b00001110, 0b00000000,
    // RESERVED                                           0: default
    // STCIEN Seek/Tune Complete Interrupt                0: disable Interrupt
    // RBDS                                               0: RDS mode only (RBDS only used in US)
    // RDS_FIFO_EN                                        0: RDS fifo mode disable (can try enabling)
    // DE De-emphasis                                     1: 50us (Americas and South Korea - 75 μs, rest of the world - 50us)
    // RDS_FIFO_CLR                                       1: clear RDS fifo (default setting)
    // SOFTMUTE_EN                                        1: soft mute enabled (gradual reduce of audio volume when signal quality drops)
    // AFCD automatic frequency control disable           0: AFC enabled
    // 
    // RESERVED                                           0: default
    // I2S_ENABLE                                         0: disable (might want to enable when integrating bluetooth/Wi-Fi)
    // GPIO general purpose I/O                      000000: default values

  // register 0x05
  0b10000010, 0b10000111,
    // INT_MODE                                           1: interrupt last until read reg 0x0C, default setting
    // SEEK_MODE                                         00: Default value; 10 enables RSSI seek mode (older)
    // RESERVED                                           0: by assumption
    // SEEKTH seek SNR threshold                       0100: default threshold, 71dB
    //
    // LNA_PORT_SEL LNA input port                       10: LNAP, default setting
    // LNA_ICSEL_BIT LNA working current                 00: 1.8mA, default setting
    // VOLUME                                          0111: default volume (0000=mute, 1111=max, logarithmic)

  // register 0x06
  0b00000000, 0b00000000,
    // RESERVED                                             0: default
    // OPEN_MODE open reserved registers mode              00: default setting
    // SLAVE_MASTER I2S slave or master                     0: master, default setting
    // WS_LR audio channel indication for I2S               0: default setting
    // SCLK_I_EDGE                                          0: default setting
    // DATA_SIGNED                                          0: I2S output unsigned 16-bit audio data, default setting
    // WS_I_EDGE                                            0: default setting
    //
    // I2S_SW_CNT (Only valid in master mode)            0000: default setting
    // SW_O_EDGE                                            0: default setting
    // SCLK_O_EDGE                                          0: default setting
    // L_DELY                                               0: default setting
    // R_DELY                                               0: default setting

  // register 0x07
  0b01000010, 0b00000010,
    // RESERVED                                             0: default
    // TH_SOFRBLEND threshhold for noise soft blend     10000: default setting
    // 65M_50M MODE                                         1: default setting
    // RESERVED                                             0: default
    //
    // SEEK_TH_OLD seek threshold for old seek mode    000000: default setting
    // SOFTBLEND_EN soft blend enable                       1: default setting
    // FREQ_MODE                                            0: default setting
};

// Tune to specific channel
uint8_t tune_config[] = {
  // register 0x02
  0b10000000, 0b00001001,
    // DHIZ audio output high-z disable                   1: normal operation
    // DMUTE mute disable                                 0: mute
    // MONO mono select                                   0: stereo
    // BASS bass boost                                    0: disabled
    // RCLK NON-CALIBRATE MODE                            0: RCLK is always supplied
    // RCLK DIRECT INPUT MODE                             0: RCLK is not in direct input mode
    // SEEKUP                                             0: seek down
    // SEEK                                               0: disable/stop seek (when seek operation completes, SEEK->0, STC->1)
    //
    // SKMODE seek mode                                   0: wrap at upper or lower band limit and contiue seeking
    // CLK_MODE clock mode                              000: 32.768kHZ clock rate (match the watch cystal on the module)
    // RDS_EN radio data system enable                    1: enable radio data system
    // NEW_METHOD                                         0: disabled new demodulate method for improved sensitivity (can try enabling)
    // SOFT_RESET                                         0: perform a reset
    // ENABLE power up enable                             1: enabled, device ready to operate

  // register 0x03
  freq_byte1(curr_freq), freq_byte2(curr_freq) | 0b010000
    // Tuning frequency
    // DIRECT MODE used only when test                    0: disabled by default
    // TUNE tune operation                                1: enable
    // BAND band select                                  00: 87–108 MHz (US/Europe)
    // SPACE channel spacing                             00: spacing of 0.1MHz
};

// Current read data from RDA5807
uint8_t requested_data[12];

// RDS Radio Text
char radiotext_A[64];
char radiotext_B[32];
bool rds_version = false; // true - version A, false - version B
bool rds_typeflag = false;
bool rds_prev_typeflag = false;

void setup() {
  Serial.begin(115200); // serial communication speed 115200 bps
  analogReadResolution(12); // Set analog read resolution: 16 bits

  // Initialize buttons
  l_key.begin(4); r_key.begin(5);
  chn_button[0].begin(6); chn_button[1].begin(7); chn_button[2].begin(15);
  chn_button[3].begin(16); chn_button[4].begin(17); chn_button[5].begin(18);

  // Startup I2C
  Wire.begin(); // (SDA, SCL)

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, sym_antenna);
  lcd.createChar(1, sym_lowsignal);
  lcd.createChar(2, sym_midsignal);
  lcd.createChar(3, sym_highsignal);
  lcd.createChar(4, sym_volume);
  lcd.createChar(5, sym_full);

  // Welcome screen
  lcd.setCursor(4, 0); // (col index, row index)
  lcd.print("DIP E036");
  lcd.setCursor(2, 1);
  lcd.print("FM Receiver");

  // // Setup values
  // curr_analogfreq = convert_freq(analogRead(FRQ_TUNE));
  // prev_analogfreq = curr_analogfreq;
  // temp_analogfreq = curr_analogfreq;
  // curr_analogvol = analogRead(VOL_ADJS);
  // prev_analogvol = curr_analogvol;

  // clear RDS text just in case
  clear_radiotext();

  // Initialize device
  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.write(init_config, 12);
  Wire.endTransmission();
  Serial.println("Initialization complete.");

  // Tune to default channel
  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.write(tune_config, 4); 
  Wire.endTransmission();
  Serial.println("Tuning complete.");

  // Read registry data of RDA5807
  request_data();

  // Delay a while and clear LCD
  delay(3000);
  lcd.clear();
  // unmute
  tune_config[0] = 0b11000000;
  change_vol(curr_vol);
}

// Loops while device is running
void loop() {
  // ignores all operations if device is scanning
  if(seeking() == false) {
    // Frequency knob read, with smoothing (channel)
    // temp_analogfreq += 0.25 * (convert_freq(analogRead(FRQ_TUNE)) - temp_analogfreq);
    // curr_analogfreq = 10 * round_int(temp_analogfreq / 10);
    // // volume knob read, with smoothing (reading value)
    // curr_analogvol += round_int(0.5 * (analogRead(VOL_ADJS) - curr_analogvol));
    // update all button states per loop
    l_key.update(); r_key.update();
    for(int i=1; i<=6; i++) {
      chn_button[i-1].update();
    }

    //----------------KNOBS---------------//

    // // change in frequency knob
    // if(curr_analogfreq != prev_analogfreq) {
    //   // update current frequency
    //   curr_freq = curr_analogfreq;
    //   change_freq(curr_freq);
    // }
    // // change in volume knob
    // else if(convert_vol(curr_analogvol) != convert_vol(prev_analogvol)) {
    //   last_vol_adj = millis();

    //   // adjust volume, update current vol
    //   curr_vol = convert_vol(curr_analogvol);
    //   change_vol(curr_vol);
    // }

    //--------------BUTTONS--------------//

    // press detected for left button, short press-tune, long press-scan
    // else if(l_key.debounce()) {
    if(l_key.debounce()) {
      Serial.println("Left key pressed.");

      // update current freq
      if(curr_freq == FREQ_MIN) {
        curr_freq = FREQ_MAX;
      }
      else {
        curr_freq -= 1;
      }

      // update ic freq
      change_freq(curr_freq);
    }
    // transition to long press
    else if(l_key.start_longpress()) {
      Serial.println("Left key long pressed.");
      // tells ic to scan downwards - false: downward
      autotune(false);
    }

    // press detected for right button
    else if(r_key.debounce()) {
      Serial.println("Right key pressed.");
      
      // update current freq
      if(curr_freq == FREQ_MAX) {
        curr_freq = FREQ_MIN;
      }
      else {
        curr_freq += 1;
      }

      // update ic freq
      change_freq(curr_freq);
    }
    // transition to long press
    else if(r_key.start_longpress()) {
      Serial.println("Right key long pressed.");
      // tells ic to scan upwards - true: upward
      autotune(true);
    }

    // channel saving and tuning
    // short press: tune to frequency, long press: save current freq as channel
    for(int chn_num = 1; chn_num <= 6; chn_num++) {
      // short press
      if(chn_button[chn_num-1].release()) {

        // check if there is any frequency saved there
        Serial.print("Channel "); Serial.print(chn_num); Serial.println(" released.");
        uint16_t channel = saved_channels[chn_num-1];
        if(channel <= (FREQ_MAX - FREQ_MIN) && channel != (curr_freq - FREQ_MIN)) {
          // update current frequency
          curr_freq = channel + FREQ_MIN;
          change_freq(curr_freq);
        }
        // if no frequency saved there, do nothing

        break;
      }
      // transition to long press
      else if(chn_button[chn_num-1].start_longpress()) {
        Serial.print("Channel "); Serial.print(chn_num); Serial.println(" long pressed.");
        // writes current freq to ch1
        save_channel(curr_freq, chn_num);

        break;
      }
    }

    // after all control operations
    // Read registry data of RDA5807
    request_data();

    // display current_freq (top)
    update_freq();
    display_freq(curr_freq);
    // display signal strngth (top)
    display_signal();
    // if said frequency is one of the saved channels, display number (top)
    lcd.setCursor(0, 0);
    for(int i=1; i<=6; i++) {
      int saved_freq = saved_channels[i-1] + FREQ_MIN;
      if(curr_freq == saved_freq) {
        lcd.print("C");
        lcd.print(i);
        break;
      }
      // no successful channels
      if(i == 6) {
        lcd.print("  ");
      }
    }

    // determine what to display at the bottom
    // RDS signal reading
    // Clear RDS radio text if changed frequency
    if(curr_freq != prev_freq) {
      Serial.println("Frequency changed.");

      clear_radiotext();
    }
    // display id RDSR = 1, new RDS group is ready
    if((requested_data[0] >> 7) == 0b1) {
      // only update if data type is radiotext, group type code = 0010
      if((requested_data[6] >> 4) == 0b0010) {
        // check version, a = true
        rds_version = ((requested_data[6] & 0b1000) == 0b0);
        // check type a/b, a = true
        rds_typeflag = ((requested_data[7] & 0b10000) == 0b0);
        // if type flag updated(NOT THE SAME AS VERSION), clear current type radio text
        if(rds_typeflag != rds_prev_typeflag) {
          // type A
          if(rds_version == true) {
            clear_radiotext('A');
          }
          // type B
          else {
            clear_radiotext('B');
          }
        }
        // update rds radiotext
        update_radiotext(rds_version);
        // update prev_type_a/b
        rds_prev_typeflag = rds_typeflag;
      }
    }

    // volume (when adjusting volume, will delay for 1s after done adjustment)
    if(millis() - last_vol_adj < 1000) {
      lcd.setCursor(0, 1);
      lcd.write(4); // volume symbol
      for(int i=1; i<=15; i++) {
        if(i <= curr_vol) {
          lcd.write(5);
        }
        else {
          lcd.print(" ");
        }
      }
    }

    // Display the text if not adjusting volume (FOR NOW: only display first 16 characters, implement scroll display later)
    // TO DO LATER: only display text when all the chars are received
    else {
      lcd.setCursor(0, 1);
      for(int i=0; i<16; i++) {
        char print_char = '\n';
        // version A radiotext
        if(rds_version == true) {
          print_char = radiotext_A[i];
        }
        // version B radiotext
        else {
          print_char = radiotext_B[i];
        }

        // ends print when hits char '\n'
        if(print_char == '\n') {
          print_char = ' ';
        }

        lcd.print(print_char);
      }
    }

    // update previous freq as current freq
    prev_freq = curr_freq;
    // prev_analogfreq = curr_analogfreq;
    // prev_analogvol = curr_analogvol;
  }
  else {
    Serial.println("Not ready");

    // Read registry data of RDA5807
    request_data();
    
    // in seek mode, top row update current frequency (read from ic) only
    update_freq();
    display_freq(curr_freq);

    // Clear bottom row
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  // delay
  delay(1);
}

// Misc functions

// Conversion from frequency to individual bits
uint8_t freq_byte1(int frequency) {
  uint8_t channel = frequency - FREQ_MIN;
  return (channel >> 2);
}
uint8_t freq_byte2(int frequency) {
  uint8_t channel = frequency - FREQ_MIN;
  return (channel & 0b11) << 6; // gets last 2 bits, then shifts left 6 bits
}

// Changing frequency in datastream (frequency = MHz / 0.1MHz)
void change_freq(int frequency) {
  if(FREQ_MIN <= frequency && frequency <= FREQ_MAX) {
    tune_config[2] = freq_byte1(frequency);
    tune_config[3] = freq_byte2(frequency) | (tune_config[3] & 0b111111); // (2 bits frequency and 6 empty bits) OR (last 6 bits of original 4th bit of tune_config)


    Wire.beginTransmission(RDA5807M_ADDRESS);
    Wire.write(tune_config, 4);
    Wire.endTransmission();

    Serial.print("Tuned to frequency ");
    Serial.print(frequency / 10); Serial.print("."); Serial.print(frequency % 10); Serial.println("MHz.");
  }
  else {
    Serial.println("[ERROR] change_freq(): Frequency out of range.");
  }
}

// Change volume output for RDA5807
void change_vol(uint8_t volume) {
  // take last 4 bits
  uint8_t volume_bytes = 0b1111 & volume;

  uint8_t vol_config[] = {
    // register 0x02
    tune_config[0], tune_config[1],
    // register 0x03
    tune_config[2], tune_config[3],
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

// Autotune command for RDA5807
void autotune(bool seekup) {
  uint8_t seekup_config[] = {seekup ? 0b11000011 : 0b11000001, tune_config[1]};

  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.write(seekup_config, 2);
  Wire.endTransmission();

  Serial.println(seekup ? "Autotune up started." : "Autotune down started.");
}

// save frequency to storage
void save_channel(int frequency, int chn_num) {
  if(1 <= chn_num && chn_num <= 6) {
    uint8_t channel = frequency - FREQ_MIN;

    // check if this frequency is saved to other channels
    for(int i=1; i<=6; i++) {
      // remove this frequency from old channel num
      if((i != chn_num) && (saved_channels[i-1] == channel)) {
        const char key[] = {'0' + i, '\0'}; // strings "1" to "6"

        // Temporal data
        saved_channels[i-1] = 0xff;

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
    if (saved_channels[chn_num-1] != channel) {
      const char key[] = {'0' + chn_num, '\0'}; // strings "1" to "6"

      // Temporal data
      saved_channels[chn_num-1] = channel;

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
  else {
    Serial.println("[ERROR] save_channel(): Channel number out of range.");
  }
}

// reads channel number from storage (channel number = frequency - FREQ_MIN)
uint8_t read_channel(int chn_num) {
  if(1 <= chn_num && chn_num <= 6) {
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

    // Read a byte from NVS
    uint8_t my_byte = 0;
    uint8_t output;
    err = nvs_get_u8(nvs_handle, key, &my_byte);
    if (err == ESP_OK) {
      output = my_byte;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
      Serial.println("NVS not found.");
      output = 0xff;
    } else {
      Serial.println("Error reading from NVS.");
      output = 0xff;
    }

    // Close NVS handle
    nvs_close(nvs_handle);

    Serial.print(key); Serial.print(" read: "); Serial.println(output);
    return output;
  }
  else {
    Serial.println("[ERROR] read_channel(): Channel number out of range.");
  }
}

// requests registry 0x0A onwards from RDA5807 module
void request_data() {
  Wire.beginTransmission(RDA5807M_ADDRESS);
  Wire.requestFrom(RDA5807M_ADDRESS, 12);
  
  for(int i=0; i<12; i++) {
    requested_data[i] = Wire.read();
  }

  Wire.endTransmission();
}

// determine if device is seeking
bool seeking() {
  uint8_t byte1 = requested_data[0];

  // SEEK-TUNE complete: 2nd bit, if completed = 1
  return ((byte1 & 0b01000000) != 0b01000000);
}

// update frequency from device
void update_freq() {
  // byte 2
  int channel = requested_data[1];
  int frequency = channel + FREQ_MIN;

  // update current frequency
  curr_freq = frequency;
}

// display frequency on top
void display_freq(int frequency) {
  lcd.setCursor(3, 0);
  if(frequency < 1000) {
    lcd.print(" ");
  }

  // print frequency (xxx.xMHz)
  lcd.print(frequency / 10);
  lcd.print(".");
  lcd.print(frequency % 10);
  lcd.print("MHz");
}

// display signal strength on top
void display_signal() {
  // get high byte of 0x0B
  uint8_t byte3 = requested_data[2];

  // whether is it tuned to station
  bool station = ((byte3 & 0b1) == 0b1);
  uint8_t strength = byte3 >> 1; // maximum is 0b1111111, RSSI

  // only display signal if tuned to station
  lcd.setCursor(12, 0);
  if(station) {
    //  0- 42 1 block
    // 43- 84 2 block
    // 85-127 3 block(full)
    lcd.write(0); // antenna symbol
    lcd.write(1); // 1 block
    // 2 block
    if(strength > 42) {
      lcd.write(2);
    }
    else {
      lcd.print(" ");
    }
    // 3 block
    if(strength > 84) {
      lcd.write(3);
    }
    else {
      lcd.print(" ");
    }
  }
  else {
    lcd.print("    ");
  }
}

// decode and update RDS radiotext from read data of RDA5807
void update_radiotext(bool version) {
  uint8_t segment_address = requested_data[7] >> 4;

  // version A radiotext
  if(version == true) {
    for(int i=0; i<4; i++) {
      radiotext_A[segment_address*4 + i] = rds_byte_to_char(requested_data[8+i]);
    }
  }
  // version B radiotext
  else {
    for(int i=0; i<2; i++) {
      radiotext_A[segment_address*2 + i] = rds_byte_to_char(requested_data[10+i]);
    }
  }
}

// clear RDS radiotext, default char is space
void clear_radiotext(char version) {
  // version A
  if(version != 'B') {
    for(int i=0; i<64; i++) {
      radiotext_A[i] = ' ';
    }
  }
  // version B
  if(version != 'A') {
    for(int i=0; i<32; i++) {
      radiotext_B[i] = ' ';
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

// converts analog signal from knob to frequency, resolution to closest 1MHz since knob is too sensitive
int convert_freq(int analog) {
  return 10 * round_int((((FREQ_MAX - FREQ_MIN) * (float)analog) / AMAX + FREQ_MIN) / 10);
}

// converts analog signal from knob to volume
int convert_vol(int analog) {
  return round_int((15.0 * analog) / AMAX);
}

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