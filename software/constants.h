#ifndef constants_h
#define constants_h

// Frequency limits
#define FREQ_MAX 1080
#define FREQ_MIN 870

#define RDA5807M_ADDRESS 0x10
#define LCD_ADDRESS 0x27

// analogRead() highest value, 2^bits-1
#define AMAX 4095

// knob pin numbers
#define CLK 11
#define DT 12

// default frequency and volume levels
#define FREQ_DEFAULT 870
#define VOL_DEFAULT 4

// RDS text auto-scrolling update interval in ms
#define RDS_SCROLL 500

// Wi-Fi
#define WIFI_SSID "Webserver-demo"
#define WIFI_PW "123456789"

#endif