#ifndef constants_h
#define constants_h

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

// default frequency and volume levels
#define FREQ_DEFAULT 870
#define VOL_DEFAULT 4

// Wi-Fi
#define WIFI_SSID "Webserver-demo"
#define WIFI_PW "123456789"

#endif