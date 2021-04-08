// additional code to test C/C++ compilation compatibility
float sicota(float x) {
	return (sin(x) + cos(x)) / tan(x);
}

int clamp(int x, int maximum, int minimum)
{
	return min(maximum, max(minimum, x));
}


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // additional code to test C/C++ compilation compatibility
  size_t st = 1;
  byte b1 = 0b11110000;
  byte b2 = 0xfa;
  float sq2 = sqrt(2.0);
  float pe = pow(2.7, sq2);
  int y = -42;
  y = abs(y);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
