#include "interface.hpp"
#include "emulator.hpp"

#include <stdexcept>
#include <random>
#include <cctype>

ArduinoEmulator emulator;

ArduinoEmulator& get_arduino_emulator_instance()
{
	// a safeguard that ensures this method is called only once (by our main).
	static std::size_t invocationCount = 0;
	if (invocationCount > 0) {
		throw std::runtime_error("Arduino emulator has been accessed multiple times. Someone is hacking the framework!");
	}
	++invocationCount;

	return emulator;
}

// Pins

void pinMode(std::uint8_t pin, std::uint8_t mode)
{
	emulator.pinMode(pin, mode);
}

void digitalWrite(std::uint8_t pin, std::uint8_t val)
{
	emulator.digitalWrite(pin, val);
}

int digitalRead(std::uint8_t pin)
{
	return emulator.digitalRead(pin);
}

int analogRead(std::uint8_t pin)
{
	return emulator.analogRead(pin);
}

void analogReference(std::uint8_t mode)
{
	emulator.analogReference(mode);
}

void analogWrite(std::uint8_t pin, int val)
{
	emulator.analogWrite(pin, val);
}

// Timing

unsigned long millis(void)
{
	return emulator.millis();
}

unsigned long micros(void)
{
	return emulator.micros();
}

void delay(unsigned long ms)
{
	emulator.delay(ms);
}

void delayMicroseconds(unsigned int us)
{
	emulator.delayMicroseconds(us);
}

// Advanced I/O

unsigned long pulseIn(std::uint8_t pin, std::uint8_t state, unsigned long timeout)
{
	return emulator.pulseIn(pin, state, timeout);
}

unsigned long pulseInLong(std::uint8_t pin, std::uint8_t state, unsigned long timeout)
{
	return emulator.pulseInLong(pin, state, timeout);
}

void shiftOut(std::uint8_t dataPin, std::uint8_t clockPin, std::uint8_t bitOrder, std::uint8_t val)
{
	emulator.shiftOut(dataPin, clockPin, bitOrder, val);
}

std::uint8_t shiftIn(std::uint8_t dataPin, std::uint8_t clockPin, std::uint8_t bitOrder)
{
	return emulator.shiftIn(dataPin, clockPin, bitOrder);
}

void tone(std::uint8_t pin, unsigned int frequency, unsigned long duration)
{
	emulator.tone(pin, frequency, duration);
}

void noTone(std::uint8_t pin)
{
	emulator.noTone(pin);
}

// Random numbers

std::default_random_engine random_engine;

long random(long min, long max)
{
	std::uniform_int_distribution<long> distribution(min, max);
	return distribution(random_engine);
}

long random(long max)
{
	return random(0, max);
}

void randomSeed(unsigned long seed)
{
	random_engine.seed(seed);
}

// Math

long map(long value, long fromLow, long fromHigh, long toLow, long toHigh)
{
	return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

// Characters

bool isAlpha(int c)
{
	return std::isalpha(c);
}

bool isAlphaNumeric(int c)
{
	return std::isalnum(c);
}

bool isAscii(int c)
{
	return c > 0 && c < 128;
}

bool isControl(int c)
{
	return std::iscntrl(c);
}

bool isDigit(int c)
{
	return std::isdigit(c);
}

bool isGraph(int c)
{
	return std::isgraph(c);
}

bool isHexadecimalDigit(int c)
{
	return std::isxdigit(c);
}

bool isLowerCase(int c)
{
	return std::islower(c);
}

bool isPrintable(int c)
{
	return std::isprint(c);
}

bool isPunct(int c)
{
	return std::ispunct(c);
}

bool isSpace(int c)
{
	return std::isspace(c);
}

bool isUpperCase(int c)
{
	return std::isupper(c);
}

bool isWhitespace(int c)
{
	return std::isblank(c);
}


// Serial

SerialMock::operator bool() const
{
	return emulator.isSerialEnabled();
}

void SerialMock::begin(long speed, SerialConfig config)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
}


#define SERIAL_MOCK_PRINT_GEN(TYPE)\
void SerialMock::print(TYPE val, SerialPrintFormat format)\
{\
	if (!emulator.isSerialEnabled()) {\
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");\
	}\
}\
\
void SerialMock::println(TYPE val, SerialPrintFormat format)\
{\
	if (!emulator.isSerialEnabled()) {\
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");\
	}\
}

SERIAL_MOCK_PRINT_GEN(char)
SERIAL_MOCK_PRINT_GEN(int)
SERIAL_MOCK_PRINT_GEN(long)
SERIAL_MOCK_PRINT_GEN(long long)
SERIAL_MOCK_PRINT_GEN(unsigned char)
SERIAL_MOCK_PRINT_GEN(unsigned int)
SERIAL_MOCK_PRINT_GEN(unsigned long)
SERIAL_MOCK_PRINT_GEN(unsigned long long)


void SerialMock::print(double val)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
}

void SerialMock::print(const char* val)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
}

void SerialMock::println(double val)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
}

void SerialMock::println(const char* val)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
}

std::size_t SerialMock::available() const
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
	return emulator.serialDataAvailable();
}

int SerialMock::peek() const
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
	if (emulator.serialDataAvailable() == 0) {
		return -1;
	}
	return emulator.peekSerial();
}

int SerialMock::read()
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
	if (emulator.serialDataAvailable() == 0) {
		return -1;
	}
	return emulator.readSerial();

}

std::size_t SerialMock::readBytes(char* buffer, std::size_t length)
{
	if (!emulator.isSerialEnabled()) {
		throw ArduinoEmulatorException("The Serial interface is disabled in the emulator.");
	}
	length = std::min(length, emulator.serialDataAvailable());
	for (std::size_t i = 0; i < length; ++i) {
		buffer[i] = emulator.readSerial();
	}
	return length;
}


SerialMock Serial;
