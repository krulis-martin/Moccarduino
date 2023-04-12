#ifndef MOCCARDUINO_SHARED_INTERFACE_HPP
#define MOCCARDUINO_SHARED_INTERFACE_HPP

#include "constants.hpp"

#include <algorithm>
#include <cmath>


// Pins

/**
 * Configures the specified pin to behave either as an input or an output.
 * https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/
 * @param mode Should be INPUT or OUTPUT
 */
void pinMode(std::uint8_t pin, std::uint8_t mode);

/**
 * Write a HIGH or a LOW value to a digital pin.
 * https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/
 */
void digitalWrite(std::uint8_t pin, std::uint8_t val);

/**
 * Reads the value from a specified digital pin, either HIGH or LOW.
 * https://www.arduino.cc/reference/en/language/functions/digital-io/digitalread/
 */
int digitalRead(std::uint8_t pin);

/**
 * Reads the value from the specified analog pin.
 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/
 */
int analogRead(std::uint8_t pin);

/**
 * Configures the reference voltage used for analog input (i.e. the value used as the top of the input range).
 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
 */
void analogReference(std::uint8_t mode);

/**
 * Writes an analog value (PWM wave) to a pin.
 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/
 */
void analogWrite(std::uint8_t pin, int val);

// Timing

/**
 * Returns the number of milliseconds passed since the Arduino board began running the current program.
 * https://www.arduino.cc/reference/en/language/functions/time/millis/
 */
unsigned long millis(void);

/**
 * Returns the number of microseconds since the Arduino board began running the current program.
 * https://www.arduino.cc/reference/en/language/functions/time/micros/
 */
unsigned long micros(void);

/**
 * Pauses the program for the amount of time (in milliseconds) specified as parameter.
 * https://www.arduino.cc/reference/en/language/functions/time/delay/
 */
void delay(unsigned long ms);

/**
 * Pauses the program for the amount of time (in microseconds) specified by the parameter.
 * https://www.arduino.cc/reference/en/language/functions/time/delaymicroseconds/
 * @param us Currently, the largest value that will produce an accurate delay is 16383.
 */
void delayMicroseconds(unsigned int us);

// Advanced I/O

/**
 * Reads a pulse (either HIGH or LOW) on a pin.
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/pulsein/
 */
unsigned long pulseIn(std::uint8_t pin, std::uint8_t state, unsigned long timeout = 1000000L);

/**
 * An alternative to pulseIn() which is better at handling long pulse and interrupt affected scenarios.
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/pulseinlong/
 */
unsigned long pulseInLong(std::uint8_t pin, std::uint8_t state, unsigned long timeout = 1000000L);

/**
 * Shifts out a byte of data one bit at a time.
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/shiftout/
 */
void shiftOut(std::uint8_t dataPin, std::uint8_t clockPin, std::uint8_t bitOrder, std::uint8_t val);

/**
 * Shifts in a byte of data one bit at a time.
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/shiftin/
 */
std::uint8_t shiftIn(std::uint8_t dataPin, std::uint8_t clockPin, std::uint8_t bitOrder);

/**
 * Generates a square wave of the specified frequency (and 50% duty cycle) on a pin.
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/tone/
 */
void tone(std::uint8_t pin, unsigned int frequency, unsigned long duration = 0);

/**
 * Stops the generation of a square wave triggered by tone().
 * https://www.arduino.cc/reference/en/language/functions/advanced-io/notone/
 */
void noTone(std::uint8_t pin);

// Random numbers

/**
 * The random function generates pseudo-random numbers.
 * https://www.arduino.cc/reference/en/language/functions/random-numbers/random/
 */
long random(long max);

/**
 * The random function generates pseudo-random numbers.
 * https://www.arduino.cc/reference/en/language/functions/random-numbers/random/
 */
long random(long min, long max);

/**
 * Initializes the pseudo-random number generator, causing it to start at an arbitrary point in its random sequence.
 * https://www.arduino.cc/reference/en/language/functions/random-numbers/randomseed/
 */
void randomSeed(unsigned long seed);

// Math

/**
 * Re-maps a number from one range to another.
 * https://www.arduino.cc/reference/en/language/functions/math/map/
 */
long map(long value, long fromLow, long fromHigh, long toLow, long toHigh);

// Characters

bool isAlpha(int c);
bool isAlphaNumeric(int c);
bool isAscii(int c);
bool isControl(int c);
bool isDigit(int c);
bool isGraph(int c);
bool isHexadecimalDigit(int c);
bool isLowerCase(int c);
bool isPrintable(int c);
bool isPunct(int c);
bool isSpace(int c);
bool isUpperCase(int c);
bool isWhitespace(int c);

using std::min;
using std::max;

// Serial interface

enum SerialConfig {
	SERIAL_5N1,
	SERIAL_6N1,
	SERIAL_7N1,
	SERIAL_8N1,
	SERIAL_5N2,
	SERIAL_6N2,
	SERIAL_7N2,
	SERIAL_8N2,
	SERIAL_5E1,
	SERIAL_6E1,
	SERIAL_7E1,
	SERIAL_8E1,
	SERIAL_5E2,
	SERIAL_6E2,
	SERIAL_7E2,
	SERIAL_8E2,
	SERIAL_5O1,
	SERIAL_6O1,
	SERIAL_7O1,
	SERIAL_8O1,
	SERIAL_5O2,
	SERIAL_6O2,
	SERIAL_7O2,
	SERIAL_8O2,
};

enum SerialPrintFormat {
	BIN,
	OCT,
	DEC,
	HEX,
};

class SerialMock
{
public:
	operator bool() const;
	void begin(long speed, SerialConfig config = SERIAL_8N1);
	void print(long long int val, SerialPrintFormat format = DEC);
	void print(long long unsigned int val, SerialPrintFormat format = DEC);
	void print(double val);
	void print(const char* val);
	void println(long long int val, SerialPrintFormat format = DEC);
	void println (long long unsigned int val, SerialPrintFormat format = DEC);
	void println(double val);
	void println(const char* val);
};

extern SerialMock Serial;

#endif
