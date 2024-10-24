#ifndef lcd_symbols_h
#define lcd_symbols_h

uint8_t sym_antenna[] = {
  0b11111,
  0b01110,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};
uint8_t sym_lowsignal[] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111
};
uint8_t sym_midsignal[] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};
uint8_t sym_highsignal[] = {
  0b00000,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};
uint8_t sym_volume[] = {
  0b00001,
  0b00011,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b00011,
  0b00001
};

uint8_t sym_full[] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b00000,
  0b00000,
  0b00000
};

uint8_t sym_bluetooth[] = {
  0b00100,
  0b00110,
  0b10101,
  0b01110,
  0b01110,
  0b10101,
  0b00110,
  0b00100
};

#endif