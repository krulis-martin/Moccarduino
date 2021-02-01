/*
 * The testing code is intentionally screwed in several places.
 * 1) It is a small precaution to prevent students copy-pasting this code.
 * 2) We want to test the limits of our framework.
 * DO NOT COPY-PASTE THIS CODE !!!
 */

#include <funshield.h>

constexpr int leds[] = {led1_pin, led2_pin, led3_pin, led4_pin};
const int ledsCount = 4;
bool buttonStates[] = { true, true };

int activeLed = 0;

void setup() {
  for (int i = 0; i < ledsCount; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], OFF);
  }
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
}

void loop() {
  bool newStates[] = { digitalRead(button1_pin), digitalRead(button2_pin) } ;
  bool changed = false;

  if (buttonStates[0] != newStates[0]) {
    buttonStates[0] = newStates[0];
    changed = true;
    if (newStates[0] == false) {
      activeLed = (activeLed + 1) % ledsCount;   
    }
  }

  if (buttonStates[1] != newStates[1]) {
    buttonStates[1] = newStates[1];
    changed = true;
    if (newStates[1] == false) {
      activeLed = (activeLed + 3) % ledsCount;   
    }
  }

  // update LEDs state
  for (int i = 0; i < ledsCount; ++i) {
    digitalWrite(leds[i], OFF);
  }
  digitalWrite(leds[activeLed], ON);

  if (changed) {
    delay(10);
  }
}
