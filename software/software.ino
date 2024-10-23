// External libraries
#include "string"
#include "Wire.h"                 // I2C Communication
#include "LiquidCrystal_I2C.h"    // LCD I2C library
#include "ESPAsyncWebServer.h"    // Hosting web server
// #include "AudioTools.h"           // Digital audio library
// #include "BluetoothA2DPSink.h"    // Bluetooth library
// Local libraries
#include "constants.h"            // Constants header
#include "main_functions.h"       // Main functions for RDA5807M
#include "button.h"               // Button detection and debouncing
#include "lcd_symbols.h"          // Containing custom symbols
#include "wifi_functions.h"       // Functions for Wi-Fi and web server

// Setup global variables
unsigned long last_vol_adj = 0;
unsigned long loop_num = 1;
int saved_channels[] = {
  read_channel(1), read_channel(2), read_channel(3), read_channel(4), read_channel(5), read_channel(6), 
  read_channel(7), read_channel(8)
}; // last session frequency and volume, chn_num 7, 8, needs to keep updating every 100 loops
int curr_freq = saved_channels[6] + FREQ_MIN; // default frequency = 87.0MHz
uint8_t curr_vol = saved_channels[7]; // default volume = 4 (0-15)
int prev_freq = curr_freq;
bool knob_state = true; // true - frequency, false - volume
bool scan_ongoing = false;
bool ready_state = true;
int wifi_freq_update = 0xff; uint8_t wifi_vol_update = 0xff; String wifi_tune_update = "Nan"; // True if user on wifi wants to change volume or frequency
String RDS_radiotext = "";
volatile uint8_t clk_state = 0b11111000;
volatile uint8_t dt_state = 0b11111000;
volatile int direction = 0;
bool bluetooth_mode = false; // Default is radio mode
bool settings_mode = false;
bool rds_enabled = true;

// channel = (frequency in MHz - 87.0) / 0.1
// using 0.1Mhz as channel spacing

// LCD
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2); // 16x2 LCD

// Bluetooth
// I2SStream i2s;
// BluetoothA2DPSink a2dp_sink(i2s);

// Buttons
Button l_key, r_key;
Button chn_button[6];
Button knob_switch;
Button settings;

// Knob ISR function
void clockwise_ISR();
void anticlockwise_ISR();

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

// Web server
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200); // serial communication speed 115200 bps

  // Initialize buttons
  l_key.begin(4); r_key.begin(5);
  chn_button[0].begin(6); chn_button[1].begin(7); chn_button[2].begin(15);
  chn_button[3].begin(16); chn_button[4].begin(17); chn_button[5].begin(18);
  settings.begin(10);

  // Initialize knob button
  knob_switch.begin(13, false);
  pinMode(CLK, INPUT); pinMode(DT, INPUT);

  // Pin to toggle analog switch
  pinMode(ANALOG_SWITCH, OUTPUT);
  digitalWrite(ANALOG_SWITCH, LOW);

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
  lcd.createChar(6, sym_bluetooth);

  // Welcome screen
  lcd.setCursor(4, 0); // (col index, row index)
  lcd.print("DIP E036");
  lcd.setCursor(2, 1);
  lcd.print("FM Receiver");

  // clear RDS text just in case
  clear_radiotext(radiotext_A, radiotext_B);

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
  request_data(requested_data);

  // Delay a while and clear LCD
  delay(3000);
  lcd.clear();
  // unmute
  change_vol(tune_config, curr_vol);

  // Open web server
  WifiAP_begin();
  ServerBegin(&server, &curr_freq, &curr_vol, &ready_state, &wifi_freq_update, &wifi_vol_update, &wifi_tune_update, &RDS_radiotext);

  // Initialize knob
  attachInterrupt(digitalPinToInterrupt(CLK), updatestate_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT), updatestate_ISR, CHANGE);
}

// Loops while device is running
void loop() {
  // Always detect settings button no matter mode
  settings.update();

  // Settings button pressed
  if(settings.release()) {
    // toggle setting mode
    settings_mode = !settings_mode;

    lcd.clear();
  }
  // Settings mode, can't control radio functions in settings mode
  if(settings_mode) {
    // buttons left/right, 1-6 still working
    l_key.update(); r_key.update();
    for(int i=1; i<=6; i++) {
      chn_button[i-1].update();
    }

    // LCD display settings menu
    // top row
    lcd.setCursor(0, 0);
    lcd.print("1 ");
    // Toggle bluetooth mode
    if(bluetooth_mode) lcd.print("Radio mode");
    else lcd.print("Bluetooth mode");
    // bottom row, only when radio mode
    if(!bluetooth_mode) {
      lcd.setCursor(0, 1);
      lcd.print("2 ");
      // Toggle RDS mode
      if(rds_enabled) lcd.print("Disable RDS");
      else lcd.print("Enable RDS");
    }

    // Button functions
    // button 1 pressed, toggle radio/bluetooth mode
    if(chn_button[0].release()) {
      bluetooth_mode = !bluetooth_mode;
      // Turn on/off bluetooth
      if(bluetooth_mode) {
        // Enable bluetooth
        digitalWrite(ANALOG_SWITCH, HIGH);
        Serial.println("Bluetooth mode enabled.");
        // a2dp_sink.start("Wireless_speaker_demo");
      }
      else {
        // Disable bluetooth
        digitalWrite(ANALOG_SWITCH, LOW);
        Serial.println("Bluetooth mode disabled.");
        // a2dp_sink.stop();
      }
      // Exit settings
      settings_mode = !settings_mode;
      lcd.clear();

      // Display 'bluetooth/radio mode' for 2 seconds
      lcd.setCursor(0, 0);
      if(bluetooth_mode) lcd.print("Bluetooth Mode");
      else lcd.print("Radio Mode");
      delay(2000);
      lcd.clear();
    }
    // button 2 pressed, only toggle RDS when in radio mode
    else if(!bluetooth_mode && chn_button[1].release()) {
      rds_enabled = !rds_enabled;
      Serial.println("RDS display toggled.");

      // Clear rds memory
      clear_radiotext(radiotext_A, radiotext_B);

      // Exit settings
      settings_mode = !settings_mode;
      lcd.clear();
    }
  }
  // Usual operation modes
  else {
    // Bluetooth mode
    if(bluetooth_mode) {
      // bluetooth symbol
      lcd.setCursor(0, 0);
      lcd.write(6);
      lcd.print(" ");

      // If no device connected, top display 'not connected'
      // Else, top display device name(scroll if char length > 13)
      // If device connected, if song playing bottom display song name, else bottom display 'Nothing playing'
    }

    // Radio mode
    else {
      // ignores all operations if device is scanning
      ready_state = !seeking(requested_data);
      if(ready_state == true) {
        //-----------UPDATE BUTTON AND KNOB STATES------//
        l_key.update(); r_key.update();
        for(int i=1; i<=6; i++) {
          chn_button[i-1].update();
        }
        knob_switch.update();

        scan_ongoing = false;

        //----------------KNOB----------------//
        // Memory clear activated
        if(knob_switch.start_custom_longpress()) {
          // Clear memory
          clear_memory();

          // Clear the temp storage
          for(int i=0; i<8; i++) {
            saved_channels[i] = read_channel(i+1);
          }

          // Progress bar interval 10%, 0.25 sec
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Clearing memory");
          for(int i=0; i<=10; i++) {
            // progress bar
            if(i > 0) {
              lcd.setCursor(i-1, 1);
              lcd.write(5);
            }
            // percent
            lcd.setCursor(10, 1);
            lcd.print(i*10); lcd.print("%");
            delay(250);
          }

          lcd.clear();
        }
        // Toggle knob state if pressed and released
        else if(knob_switch.release()) {
          knob_state = !knob_state;
          if(knob_state) {
            Serial.println("Knob mode: frequency");
          }
          else {
            Serial.println("Knob mode: volume");
            // display volume symbol
            last_vol_adj = millis();
          }
        }
        // clockwise detected
        else if(direction == 1) {
          Serial.println("Knob turned clockwise");
          // frequency mode
          if(knob_state) {
            // update current freq
            if(curr_freq == FREQ_MAX) {
              curr_freq = FREQ_MIN;
            }
            else {
              curr_freq += 1;
            }

            // update ic freq
            change_freq(tune_config, curr_freq);
          }
          // volume mode
          else {
            last_vol_adj = millis();

            if(curr_vol != 15) {
              curr_vol += 1;
            }
            change_vol(tune_config, curr_vol);
          }

          direction = 0;
        }
        // anticlockwise detected
        else if(direction == -1) {
          Serial.println("Knob turned anticlockwise");

          // frequency mode
          if(knob_state) {
            // update current freq
            if(curr_freq == FREQ_MIN) {
              curr_freq = FREQ_MAX;
            }
            else {
              curr_freq -= 1;
            }

            // update ic freq
            change_freq(tune_config, curr_freq);
          }
          // volume mode
          else {
            last_vol_adj = millis();

            if(curr_vol != 0) {
              curr_vol -= 1;
            }
            change_vol(tune_config, curr_vol);
          }

          direction = 0;
        }


        //--------------BUTTONS--------------//

        // press detected for left button, short press-tune, long press-scan
        else if(l_key.debounce()) {
          Serial.println("Left key pressed.");

          // update current freq
          if(curr_freq == FREQ_MIN) {
            curr_freq = FREQ_MAX;
          }
          else {
            curr_freq -= 1;
          }

          // update ic freq
          change_freq(tune_config, curr_freq);
        }
        // transition to long press
        else if(l_key.start_longpress()) {
          Serial.println("Left key long pressed.");
          // tells ic to scan downwards - false: downward
          scan_ongoing = true;
          autotune(tune_config, false);
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
          change_freq(tune_config, curr_freq);
        }
        // transition to long press
        else if(r_key.start_longpress()) {
          Serial.println("Right key long pressed.");
          // tells ic to scan upwards - true: upward
          scan_ongoing = true;
          autotune(tune_config, true);
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
              change_freq(tune_config, curr_freq);
            }
            // if no frequency saved there, do nothing

            break;
          }
          // transition to long press
          else if(chn_button[chn_num-1].start_longpress()) {
            Serial.print("Channel "); Serial.print(chn_num); Serial.println(" long pressed.");
            // writes current freq to ch1
            save_channel(saved_channels, curr_freq, chn_num);

            break;
          }
        }

        //----------------WIFI OPERATIONS----------------//
        if(wifi_freq_update != 0xff) {
          curr_freq = wifi_freq_update;
          change_freq(tune_config, curr_freq);

          // After changing frequency
          wifi_freq_update = 0xff;
        }
        else if(wifi_vol_update != 0xff) {
          curr_vol = wifi_vol_update;
          change_vol(tune_config, curr_vol);

          // After changing volume
          wifi_vol_update = 0xff;
        }
        else if(wifi_tune_update == "up") {
          scan_ongoing = true;
          autotune(tune_config, true);

          // After tuning up
          wifi_tune_update = "Nan";
        }
        else if(wifi_tune_update == "down") {
          scan_ongoing = true;
          autotune(tune_config, false);

          // After tuning down
          wifi_tune_update = "Nan";
        }

        //------------------DISPLAY OPERATIONS--------------//

        // after all control operations
        // Read registry data of RDA5807
        request_data(requested_data);

        // display current_freq (top)
        update_freq(requested_data, &curr_freq);
        display_freq(curr_freq, &lcd);
        // display signal strngth (top)
        display_signal(requested_data, &lcd);
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
        if(rds_enabled) {
          // Clear RDS radio text if changed frequency
          if(curr_freq != prev_freq) {
            Serial.println("Frequency changed, clearing RDS data.");

            clear_radiotext(radiotext_A, radiotext_B);
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
                  clear_radiotext(radiotext_A, radiotext_B, 'A');
                }
                // type B
                else {
                  clear_radiotext(radiotext_A, radiotext_B, 'B');
                }
              }
              // update rds radiotext
              update_radiotext(requested_data, radiotext_A, radiotext_B, rds_version);
              // update prev_type_a/b
              rds_prev_typeflag = rds_typeflag;
            }
          }
          // determine text length
          int rds_length = 16;
          String radio_text = "";
          // version A radiotext
          if(rds_version == true) {
            for(int i=0; i<64; i++) {
              if(radiotext_A[i] == '\n') {
                rds_length = i;
                break;
              }
              radio_text += radiotext_A[i];
            }
          }
          // version B radiotext
          else {
            for(int i=0; i<32; i++) {
              if(radiotext_B[i] == '\n') {
                rds_length = i;
                break;
              }
              radio_text += radiotext_B[i];
            }
          }
          RDS_radiotext = radio_text;
        }

        // volume (when adjusting volume, will delay for 1s after done adjustment)
        if(millis() - last_vol_adj < 2000) {
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

        // Display the RDS text if not adjusting volume and RDS mode is on
        else if(rds_enabled) {
          // Starts printing
          lcd.setCursor(0, 1);
          // Short text, no scrolling
          if(rds_length <= 16) {
            for(int i=0; i<16; i++) {
              if(i < rds_length) {
                lcd.print(radio_text[i]);
              }
              else {
                lcd.print(" ");
              }
            }
          }
          // Scrolling
          else {
            int offset = (millis() / RDS_SCROLL) % (rds_length + 3); // 3 spaces between end and beginning
            for(int i=0; i<16; i++) {
              int index = (i + offset) % (rds_length + 3);
              if(index < rds_length) {
                lcd.print(radio_text[index]);
              }
              else {
                lcd.print(" ");
              }
            }
          }
        }

        //------------END OF LOOP OPERATIONS-------------//

        // Check if volume or frequency is out of range
        if(curr_freq < FREQ_MIN || curr_freq > FREQ_MAX) {
          curr_freq = FREQ_DEFAULT;
        }
        if(curr_vol < 0 || curr_vol > 15) {
          curr_vol = VOL_DEFAULT;
        }

        // update previous freq as current freq
        prev_freq = curr_freq;
      }
      else {
        Serial.println("Not ready");

        // Read registry data of RDA5807
        request_data(requested_data);

        // Top row update current frequency (read from ic) and signal
        update_freq(requested_data, &curr_freq);
        display_freq(curr_freq, &lcd);
        display_signal(requested_data, &lcd);

        // Update tune_config for current frequency
        tune_config[2] = freq_byte1(curr_freq);
        tune_config[3] = freq_byte2(curr_freq) | (tune_config[3] & 0b111111);
        
        if(scan_ongoing) {
          Serial.println("Scanning...");      

          // Put "Scanning..." if scan is ongoing
          lcd.setCursor(0, 1);
          if(millis() % 400 < 100) {
            lcd.print("Scanning        ");
          }
          else if(millis() % 400 < 200) {
            lcd.print("Scanning.       ");
          }
          else if(millis() % 400 < 300) {
            lcd.print("Scanning..      ");
          }
          else {
            lcd.print("Scanning...     ");
          }
        }
      }

      // save current volume and frequency into NVS every 100 loops
      if(loop_num % 100 == 0) {
        save_channel(saved_channels, curr_freq, 7);
        save_channel(saved_channels, curr_vol, 8);
      }
      loop_num++;
    }
  }
  
  // delay
  delay(1);
}

void updatestate_ISR() {
  // Knob is only needed in radio mode
  if(!bluetooth_mode) {
    uint8_t clk_input = digitalRead(CLK);
    uint8_t dt_input = digitalRead(DT);
    // New input is different than past input
    if((clk_state & 1) != clk_input || (dt_state & 1) != dt_input) {
      // Update states
      clk_state = clk_state << 1 | clk_input | 0b11111000;
      dt_state = dt_state << 1 | dt_input | 0b11111000;
      // Clockwise detected
      if(clk_state == 0b11111100 && dt_state == 0b11111110) {
        direction = 1;
        // Reset state
        clk_state = 0b11111000; dt_state = 0b11111000;
      }
      // Anticlockwise detected
      else if(clk_state == 0b11111110 && dt_state == 0b11111100) {
        direction = -1;
        // Reset state
        clk_state = 0b11111000; dt_state = 0b11111000;
      }
    }
  }
  // Do nothing, ensure variables are in default state
  else {
    if(direction != 0) direction = 0;
    if(clk_state != 0b11111000) clk_state = 0b11111000;
    if(dt_state != 0b11111000) dt_state = 0b11111000;
  }
}