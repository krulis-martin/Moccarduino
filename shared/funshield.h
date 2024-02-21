// Funshield constants
// v1.0.1 18.2.2024

#ifndef FUNSHIELD_CONSTANTS_H__
#define FUNSHIELD_CONSTANTS_H__

// convenience constants for switching on/off
constexpr int ON = LOW;
constexpr int OFF = HIGH;

// 7-segs
constexpr int latch_pin = 4;
constexpr int clock_pin = 7;
constexpr int data_pin = 8;

// buzzer
constexpr int beep_pin = 3;

// LEDs
constexpr int led1_pin = 13;
constexpr int led2_pin = 12;
constexpr int led3_pin = 11;
constexpr int led4_pin = 10;

// buttons
constexpr int button1_pin = A1;
constexpr int button2_pin = A2;
constexpr int button3_pin = A3;

// trimmer
constexpr int trimmer_pin = A0;

// numerical digits for 7-segs
constexpr int digits[10] = { 0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90 };
constexpr int empty_glyph = 0xff;

#endif

