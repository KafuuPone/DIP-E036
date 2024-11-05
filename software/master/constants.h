#ifndef constants_h
#define constants_h

// Frequency limits
#define FREQ_MAX 1080
#define FREQ_MIN 870

// I2C addresses
#define RDA5807M_ADDRESS 0x10
#define LCD_ADDRESS 0x27
#define SLAVE_ADDRESS 0x55

// Button pin numbers
#define LEFT_BUTTON 18
#define RIGHT_BUTTON 17
#define CH1 4
#define CH2 5
#define CH3 6
#define CH4 7
#define CH5 15
#define CH6 16
#define SETTINGS_BUTTON 10

// knob pin numbers
#define CLK 11
#define DT 12

// other pin numbers
#define ANALOG_SWITCH 14

// default frequency and volume levels
#define FREQ_DEFAULT 870
#define VOL_DEFAULT 4

// Text auto-scrolling update interval in ms
#define RDS_SCROLL 500
#define TITLE_SCROLL 500

// Wi-Fi
#define WIFI_SSID "DIP-E036 Speaker"
#define WIFI_PW "123456789"

#endif