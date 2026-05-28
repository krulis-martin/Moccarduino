// Funshield constants
// v2.0.0 01.03.2026

#ifndef FUNSHIELD_CONSTANTS_H__
#define FUNSHIELD_CONSTANTS_H__

// convenience constants for switching on/off
constexpr int ON = LOW;
constexpr int OFF = HIGH;

// buzzer
constexpr int BEEP_PIN = 3;

// LEDs
constexpr int LED1_PIN = 13;
constexpr int LED2_PIN = 12;
constexpr int LED3_PIN = 11;
constexpr int LED4_PIN = 10;

// buttons
constexpr int BUTTON1_PIN = A1;
constexpr int BUTTON2_PIN = A2;
constexpr int BUTTON3_PIN = A3;

// trimmer
constexpr int TRIMMER_PIN = A0;

// 7-segs
// control pins
constexpr int SEG7_LATCH_PIN = 4;
constexpr int SEG7_CLOCK_PIN = 7;
constexpr int SEG7_DATA_PIN = 8;

constexpr byte SEG7_EMPTY_GLYPH = 0xff;

// numerical digits
constexpr byte SEG7_DIGIT_GLYPHS[10] = { 0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90 };

// map of letter glyphs
constexpr byte SEG7_LETTER_GLYPHS[] {
  0b10001000,   // A
  0b10000011,   // b
  0b11000110,   // C
  0b10100001,   // d
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
  0b10010001,   // Y
  0b10100100,   // Z
};

#endif

