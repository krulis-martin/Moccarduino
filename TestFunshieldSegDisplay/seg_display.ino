/*
 * The testing code is intentionally screwed.
 * 1) It is a small precaution to prevent students copy-pasting this code.
 * 2) We want to test the limits of our framework.
 * DO NOT COPY-PASTE THIS CODE !!!
 */

#include <funshield.h>

void setup() {
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
}

int lastButton = -1;

void loop() {
  bool newStates[] = { digitalRead(button1_pin), digitalRead(button2_pin), digitalRead(button3_pin) } ;
  for (int i = 0; i < 3; ++i) {
    if (newStates[i] == false) {
      lastButton = i;
    }
  }

  if (lastButton == 0) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10001000); // a
    shiftOut(data_pin, clock_pin, MSBFIRST, 1);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10000011); // b
    shiftOut(data_pin, clock_pin, MSBFIRST, 2);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11000110); // c
    shiftOut(data_pin, clock_pin, MSBFIRST, 4);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10100001); // d
    shiftOut(data_pin, clock_pin, MSBFIRST, 8);
    digitalWrite(latch_pin, HIGH);
  }
  else if (lastButton == 1) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10000110); // e
    shiftOut(data_pin, clock_pin, MSBFIRST, 1);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10001110); // f
    shiftOut(data_pin, clock_pin, MSBFIRST, 2);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10000010); // g
    shiftOut(data_pin, clock_pin, MSBFIRST, 4);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10001001); // h
    shiftOut(data_pin, clock_pin, MSBFIRST, 8);
    digitalWrite(latch_pin, HIGH);
  }
  else if (lastButton == 2) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11111001); // i
    shiftOut(data_pin, clock_pin, MSBFIRST, 1);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11100001); // j
    shiftOut(data_pin, clock_pin, MSBFIRST, 2);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b10000101); // k
    shiftOut(data_pin, clock_pin, MSBFIRST, 4);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11000111); // l
    shiftOut(data_pin, clock_pin, MSBFIRST, 8);
    digitalWrite(latch_pin, HIGH);
  }
}