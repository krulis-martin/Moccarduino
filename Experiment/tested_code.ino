#include <funshield.h>

constexpr int leds[] = {led1_pin, led2_pin, led3_pin, led4_pin};
const int ledsCount = 4;

void setup() {
  for (int i = 0; i < ledsCount; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], OFF);
  }
}

void loop() {
}
