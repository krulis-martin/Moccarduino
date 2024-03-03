/*
 * The testing code is intentionally screwed.
 * 1) It is a small precaution to prevent students copy-pasting this code.
 * 2) We want to test the limits of our framework.
 * DO NOT COPY-PASTE THIS CODE !!!
 */

#include <funshield.h>

constexpr int leds[] = {led1_pin, led2_pin, led3_pin, led4_pin};
const int ledsCount = 4;

void setup() {
  for (int i = 0; i < ledsCount; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], OFF);
  }
  pinMode(latch_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
  Serial.begin(9600);
}

bool btnStates[] = { OFF, OFF, OFF } ;
bool showLeds = false;
bool showSegs = false;
int counter = 0;

void loop() {
  Serial.println("Look at me, I'm looping...");
  bool newStates[] = { digitalRead(button1_pin), digitalRead(button2_pin), digitalRead(button3_pin) } ;
  for (int i = 0; i < 3; ++i) {
    if (newStates[i] == ON && btnStates[i] == OFF) {
        if (i == 0) {
            showLeds = !showLeds;
        } else if (i == 1) {
            showSegs = !showSegs;
        } else {
            ++counter;
        }
    }
    btnStates[i] = newStates[i];
  }

  if (showLeds) {
    int x = counter;
    digitalWrite(leds[0], (x & 1) ? ON : OFF);
    x = x >> 1;
    digitalWrite(leds[1], (x & 1) ? ON : OFF);
    x = x >> 1;
    digitalWrite(leds[2], (x & 1) ? ON : OFF);
    x = x >> 1;
    digitalWrite(leds[3], (x & 1) ? ON : OFF);
  } else {
    digitalWrite(leds[0], OFF);
    digitalWrite(leds[1], OFF);
    digitalWrite(leds[2], OFF);
    digitalWrite(leds[3], OFF);
  }
  
  if (showSegs) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[counter % 10]);
    shiftOut(data_pin, clock_pin, MSBFIRST, 1);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[counter/10 % 10]);
    shiftOut(data_pin, clock_pin, MSBFIRST, 2);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[counter/100 % 10]);
    shiftOut(data_pin, clock_pin, MSBFIRST, 4);
    digitalWrite(latch_pin, HIGH);
  
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, digits[counter/1000 % 10]);
    shiftOut(data_pin, clock_pin, MSBFIRST, 8);
    digitalWrite(latch_pin, HIGH);
  }
  else {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11111111);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b11111111);
    digitalWrite(latch_pin, HIGH);
  }

  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch >= '0' && ch <= '9') {
        counter = (counter * 10 + (int)(ch - '0')) % 10000;
    }
  }
}
