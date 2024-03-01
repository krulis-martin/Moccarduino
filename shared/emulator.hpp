#ifndef MOCCARDUINO_SHARED_EMULATOR_HPP
#define MOCCARDUINO_SHARED_EMULATOR_HPP

#include "time_series.hpp"
#include "constants.hpp"

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <random>

using pin_t = std::uint8_t;


class ArduinoSimulationController; // forward declaration, so we can befriend this class

// Tested code interface
void setup();
void loop();


/**
 * Exception thrown in case of any emulator failure.
 */
class ArduinoEmulatorException : public std::runtime_error
{
public:
	ArduinoEmulatorException(const char* msg) : std::runtime_error(msg) {}
	ArduinoEmulatorException(const std::string& msg) : std::runtime_error(msg) {}
	virtual ~ArduinoEmulatorException() noexcept {}
};


/**
 * Records one change of the value of the pin.
 */
struct ArduinoPinState {
public:
	pin_t pin;		///< pin identifier
	int value;		///< new value (either written or received as input)

	ArduinoPinState() : pin(~(pin_t)0), value(-1) {}
	ArduinoPinState(pin_t p, int v) : pin(p), value(v) {}

	inline bool operator<(const ArduinoPinState& ps) const
	{
		return pin < ps.pin || (pin == ps.pin && value < ps.value);
	}

	inline bool operator==(const ArduinoPinState& ps) const
	{
		return pin == ps.pin && value == ps.value;
	}

	/**
	 * Helper function for testing. Generates a sequence of PinState structs from variadic arguments.
	 * The user gives only actual state values, pin from template parameter is attached to every value.
	 */
	template<int PIN, typename...Args>
	inline static std::vector<ArduinoPinState> sequence(Args...args) {
		return { ArduinoPinState(PIN, args)... };
	}

	operator std::string() const
	{
		std::stringstream sstr;
		sstr << pin << ":" << value;
		return sstr.str();

	}
};


class ArduinoEmulator;

/**
 * Represents one digital arduino pin (input or output).
 */
class ArduinoPin : public EventConsumer<ArduinoPinState>
{
friend class ArduinoSimulationController;
friend class ArduinoEmulator;

public:
	using Event = TimeSeries<ArduinoPinState>::Event;

private:
	static const int UNDEFINED = -1;

	ArduinoPinState mState;

	int mWiring;	///< how the pin is actually wired (INPUT/OUTPUT)
	int mMode;		///< current operating mode (INPUT/OUTPUT)

	/*
	 * Interface for the simulator.
	 */

	void reset()
	{
		mMode = UNDEFINED;
		mState.value = UNDEFINED;
	}

protected:
	void doAddEvent(logtime_t time, ArduinoPinState state) override
	{
		if (mState.pin == state.pin) {
			mState.value = state.value;
		}
		EventConsumer<ArduinoPinState>::doAddEvent(time, state);
	}


public:
	ArduinoPin(pin_t pin, int wiring = UNDEFINED) : mState(pin, UNDEFINED), mWiring(wiring), mMode(UNDEFINED) {}

	/**
	 * Change the mode of the pin. This can be done only once (typically in setup).
	 */
	void setMode(int mode)
	{
		if (mode != INPUT && mode != OUTPUT) {
			throw ArduinoEmulatorException("Trying to set pin into invalid mode.");
		}
		if (mMode != UNDEFINED && mMode != mode) {
			// Regular applications cannot use one pin both as input and as output.
			throw ArduinoEmulatorException("Unable to change I/O mode of a pin at runtime.");
		}
		if (mWiring == INPUT && mMode == OUTPUT) {
			throw ArduinoEmulatorException("Attempting to switch input pin into output mode. That might result in shot circuit.");
		}

		mMode = mode;

		if (mMode == INPUT && mState.value == UNDEFINED) {
			mState.value = HIGH;
		}
	}

	/**
	 * Read binary the value of the pin. This is valid only for input pins.
	 */
	int read() const
	{
		if (mMode == UNDEFINED) {
			throw ArduinoEmulatorException("Pin mode has to be set before the pin is actually used.");
		}

		if (mMode != INPUT) {
			throw ArduinoEmulatorException("Unable to read data from an output pin.");
		}

		return mState.value;
	}

	/**
	 * Change the value of the pin. This is valid only for output pins.
	 */
	void write(int value, logtime_t time)
	{
		if (mMode == UNDEFINED) {
			throw ArduinoEmulatorException("Pin mode has to be set before the pin is actually used.");
		}

		if (mMode != OUTPUT) {
			throw ArduinoEmulatorException("Unable to write data to an input pin.");
		}

		mState.value = value;
		addEvent(time, mState);
	}
};


/**
 * The base emulator class. The code wrapper should be derived from this class
 * so the setup() and loop() methods have access to protected methods of this class
 * which emulate the Arduino API.
 */
class ArduinoEmulator
{
friend class ArduinoSimulationController;
private:
	/**
	 * Current timestamp in microseconds (time elapsed from the start).
	 * Also acts as logical time of the simulation.
	 */
	logtime_t mCurrentTime;

	/**
	 * Pins of the arduino and their state.
	 */
	std::map<pin_t, ArduinoPin> mPins;

	/**
	 * Cache for future time series that feed the input pins.
	 * These series need to be attached to emulator, so it can properly advance their time.
	 */
	std::map<pin_t, EventConsumer<ArduinoPinState>*> mInputs;

	// Guards that prevent certain function from being called.
	bool mEnablePinMode;
	bool mEnableDigitalWrite;
	bool mEnableDigitalRead;
	bool mEnableAnalogRead;
	bool mEnableAnalogReference;
	bool mEnableAnalogWrite;
	bool mEnableMillis;
	bool mEnableMicros;
	bool mEnableDelay;
	bool mEnableDelayMicroseconds;
	bool mEnablePulseIn;
	bool mEnablePulseInLong;
	bool mEnableShiftOut;
	bool mEnableShiftIn;
	bool mEnableTone;
	bool mEnableNoTone;
	bool mEnableSerial;

	// Timing parameters
	logtime_t mPinReadDelay;
	logtime_t mPinWriteDelay;
	logtime_t mPinSetModeDelay;

	std::deque<char> mSerialData;

	void reset()
	{
		mCurrentTime = 0;

		for (auto& [_, input] : mInputs) {
			input->clear();
		}

		for (auto& [_, arduinoPin] : mPins) {
			arduinoPin.reset();
		}
	}

	/**
	 * Advances the Arduino emulator time forward by given number of microseconds.
	 * @param us relative logical time in microseconds
	 */
	void advanceCurrentTimeBy(logtime_t us)
	{
		mCurrentTime += us;

		for (auto& [_, input] : mInputs) {
			input->advanceTime(mCurrentTime);
		}

		for (auto& [_, arduinoPin] : mPins) {
			arduinoPin.advanceTime(mCurrentTime);
		}
	}

	/**
	 * Get an object representing given pin or throw exception if no such pin exists.
	 */
	ArduinoPin& getPin(pin_t pin)
	{
		auto it = mPins.find(pin);
		if (it == mPins.end()) {
			throw ArduinoEmulatorException("Trying to reach pin which is not defined in the emulator.");
		}

		return it->second;
	}

	/**
	 * Remove all registered pins.
	 */
	void removeAllPins()
	{
		mInputs.clear();
		mPins.clear();
	}

	/**
	 * Registers a new pin with given index and wiring settings.
	 */
	void registerPin(pin_t pin, int wiring = ArduinoPin::UNDEFINED)
	{
		auto it = mPins.find(pin);
		if (it != mPins.end()) {
			throw ArduinoEmulatorException("Given pin already exists.");
		}
		mPins.emplace(pin,ArduinoPin(pin, wiring));
	}

	/**
	 * Register given event consumer as an input for particular pin.
	 * @param pin associated with the input
	 * @param input event consumer (i.e., producer in this context)
	 */
	void registerPinInput(pin_t pin, EventConsumer<ArduinoPinState> &input)
	{
		auto& arduinoPin = getPin(pin);
		if (arduinoPin.mWiring != INPUT) {
			throw std::runtime_error("Unable to attach input event provider to pin which is not wired as input.");
		}

		// detach old input chain first (if exists)
		auto it = mInputs.find(pin);
		if (it != mInputs.end()) {
			it->second->lastConsumer()->detachNextConsumer();
		}

		// attach the corresponding input pin at the end of consumer chain
		input.lastConsumer()->attachNextConsumer(arduinoPin);
		mInputs[pin] = &input;
	}

	/**
	 * Abstraction of setup() invocation used by the simulator.
	 */
	void invokeSetup()
	{
		setup();
	}

	/**
	 * Abstraction of setup() invocation used by the simulator.
	 */
	void invokeLoop()
	{
		loop();
	}

public:
	ArduinoEmulator() :
		mCurrentTime(0),
		mEnablePinMode(true),
		mEnableDigitalWrite(true),
		mEnableDigitalRead(true),
		mEnableAnalogRead(true),
		mEnableAnalogReference(true),
		mEnableAnalogWrite(true),
		mEnableMillis(true),
		mEnableMicros(true),
		mEnableDelay(true),
		mEnableDelayMicroseconds(true),
		mEnablePulseIn(true),
		mEnablePulseInLong(true),
		mEnableShiftOut(true),
		mEnableShiftIn(true),
		mEnableTone(true),
		mEnableNoTone(true),
		mEnableSerial(false),
		mPinReadDelay(20),
		mPinWriteDelay(20),
		mPinSetModeDelay(100)
	{}

	/*
	 * Interface available to the tested implementation. 
	 */
	
	// Pins

	/**
	 * Configures the specified pin to behave either as an input or an output.
	 * https://www.arduino.cc/reference/en/language/functions/digital-io/pinmode/
	 * @param mode Should be INPUT or OUTPUT
	 */
	void pinMode(pin_t pin, std::uint8_t mode)
	{
		if (!mEnablePinMode) {
			throw ArduinoEmulatorException("The pinMode() function is disabled in the emulator.");
		}

		auto& arduinoPin = getPin(pin);
		arduinoPin.setMode(mode);
		advanceCurrentTimeBy(mPinSetModeDelay);
	}

	/**
	 * Write a HIGH or a LOW value to a digital pin.
	 * https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/
	 */
	void digitalWrite(pin_t pin, std::uint8_t val)
	{
		if (!mEnableDigitalWrite) {
			throw ArduinoEmulatorException("The digitalWrite() function is disabled in the emulator.");
		}

		auto& arduinoPin = getPin(pin);
		arduinoPin.write(val, mCurrentTime);
		advanceCurrentTimeBy(mPinWriteDelay);
	}

	/**
	 * Reads the value from a specified digital pin, either HIGH or LOW.
	 * https://www.arduino.cc/reference/en/language/functions/digital-io/digitalread/
	 */
	int digitalRead(pin_t pin)
	{
		if (!mEnableDigitalRead) {
			throw ArduinoEmulatorException("The digitalRead() function is disabled in the emulator.");
		}

		auto& arduinoPin = getPin(pin);
		auto val = arduinoPin.read();
		advanceCurrentTimeBy(mPinReadDelay);
		return val;
	}

	/**
	 * Reads the value from the specified analog pin.
	 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/
	 */
	int analogRead(pin_t pin)
	{
		if (!mEnableAnalogRead) {
			throw ArduinoEmulatorException("The analogRead() function is disabled in the emulator.");
		}

		auto& arduinoPin = getPin(pin);
		auto val = arduinoPin.read();
		advanceCurrentTimeBy(mPinReadDelay); // delay taken from documentation
		return val * 1023;
	}

	/**
	 * Configures the reference voltage used for analog input (i.e. the value used as the top of the input range).
	 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
	 */
	void analogReference(std::uint8_t mode)
	{
		if (!mEnableAnalogReference) {
			throw ArduinoEmulatorException("The analogReference() function is disabled in the emulator.");
		}
		
		throw ArduinoEmulatorException("The analogReference() function is not implemented in the emulator yet.");
	}

	/**
	 * Writes an analog value (PWM wave) to a pin. 
	 * https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/
	 */
	void analogWrite(pin_t pin, int val)
	{
		if (!mEnableAnalogWrite) {
			throw ArduinoEmulatorException("The analogWrite() function is disabled in the emulator.");
		}

		if (!digitalPinHasPWM(pin)) {
			throw ArduinoEmulatorException("Only pins that support PWM can be used in analogWrite() function.");
		}

		throw ArduinoEmulatorException("The analogWrite() function is not implemented in the emulator yet.");
	}

	// Timing

	/**
	 * Returns the number of milliseconds passed since the Arduino board began running the current program.
	 * https://www.arduino.cc/reference/en/language/functions/time/millis/
	 */
	unsigned long millis(void)
	{
		if (!mEnableMillis) {
			throw ArduinoEmulatorException("The millis() function is disabled in the emulator.");
		}

		return (unsigned long)(mCurrentTime / 1000);
	}

	/**
	 * Returns the number of microseconds since the Arduino board began running the current program.
	 * https://www.arduino.cc/reference/en/language/functions/time/micros/
	 */
	unsigned long micros(void)
	{
		if (!mEnableMicros) {
			throw ArduinoEmulatorException("The micros() function is disabled in the emulator.");
		}

		return (unsigned long)mCurrentTime;
	}

	/**
	 * Pauses the program for the amount of time (in milliseconds) specified as parameter.
	 * https://www.arduino.cc/reference/en/language/functions/time/delay/
	 */
	void delay(unsigned long ms)
	{
		if (!mEnableDelay) {
			throw ArduinoEmulatorException("The delay() function is disabled in the emulator.");
		}

		advanceCurrentTimeBy((logtime_t)1000 * (logtime_t)ms);
	}

	/**
	 * Pauses the program for the amount of time (in microseconds) specified by the parameter.
	 * https://www.arduino.cc/reference/en/language/functions/time/delaymicroseconds/
	 * @param us Currently, the largest value that will produce an accurate delay is 16383.
	 */
	void delayMicroseconds(unsigned int us)
	{
		if (!mEnableDelayMicroseconds) {
			throw ArduinoEmulatorException("The delayMicroseconds() function is disabled in the emulator.");
		}

		advanceCurrentTimeBy((logtime_t)us);
	}

	// Advanced I/O

	/**
	 * Reads a pulse (either HIGH or LOW) on a pin.
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/pulsein/
	 */
	unsigned long pulseIn(pin_t pin, std::uint8_t state, unsigned long timeout = 1000000L)
	{
		if (!mEnablePulseIn) {
			throw ArduinoEmulatorException("The pulseIn() function is disabled in the emulator.");
		}

		throw ArduinoEmulatorException("The pulseIn() function is not implemented in the emulator yet.");
	}

	/**
	 * An alternative to pulseIn() which is better at handling long pulse and interrupt affected scenarios.
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/pulseinlong/
	 */
	unsigned long pulseInLong(pin_t pin, std::uint8_t state, unsigned long timeout = 1000000L)
	{
		if (!mEnablePulseInLong) {
			throw ArduinoEmulatorException("The pulseInLong() function is disabled in the emulator.");
		}

		throw ArduinoEmulatorException("The pulseInLong() function is not implemented in the emulator yet.");
	}

	/**
	 * Shifts out a byte of data one bit at a time.
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/shiftout/
	 */
	void shiftOut(pin_t dataPin, pin_t clockPin, std::uint8_t bitOrder, std::uint8_t val)
	{
		if (!mEnableShiftOut) {
			throw ArduinoEmulatorException("The shiftOut() function is disabled in the emulator.");
		}
		
		// Taken from Arduino codebase (wiring_shift.c)
		for (std::uint8_t i = 0; i < 8; i++) {
			if (bitOrder == LSBFIRST) {
				digitalWrite(dataPin, val & 1);
				val >>= 1;
			}
			else {
				digitalWrite(dataPin, (val & 128) != 0);
				val <<= 1;
			}

			digitalWrite(clockPin, HIGH);
			digitalWrite(clockPin, LOW);
		}
	}

	/**
	 * Shifts in a byte of data one bit at a time.
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/shiftin/
	 */
	std::uint8_t shiftIn(pin_t dataPin, pin_t clockPin, std::uint8_t bitOrder)
	{
		if (!mEnableShiftIn) {
			throw ArduinoEmulatorException("The shiftIn() function is disabled in the emulator.");
		}

		// Taken from Arduino codebase (wiring_shift.c)
		uint8_t value = 0;
		for (std::uint8_t i = 0; i < 8; ++i) {
			digitalWrite(clockPin, HIGH);
			if (bitOrder == LSBFIRST)
				value |= digitalRead(dataPin) << i;
			else
				value |= digitalRead(dataPin) << (7 - i);
			digitalWrite(clockPin, LOW);
		}
		return value;
	}

	/**
	 * Generates a square wave of the specified frequency (and 50% duty cycle) on a pin.
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/tone/
	 */
	void tone(pin_t pin, unsigned int frequency, unsigned long duration = 0)
	{
		if (!mEnableTone) {
			throw ArduinoEmulatorException("The tone() function is disabled in the emulator.");
		}

		throw ArduinoEmulatorException("The tone() function is not implemented in the emulator yet.");
	}

	/**
	 * Stops the generation of a square wave triggered by tone().
	 * https://www.arduino.cc/reference/en/language/functions/advanced-io/notone/
	 */
	void noTone(pin_t pin)
	{
		if (!mEnableNoTone) {
			throw ArduinoEmulatorException("The noTone() function is disabled in the emulator.");
		}

		throw ArduinoEmulatorException("The noTone() function is not implemented in the emulator yet.");
	}

	bool isSerialEnabled() const
	{
		return mEnableSerial;
	}

	/**
	 * Enqueue additional serial data to be read by the emulated code.
	 */
	void addSerialData(const std::string& str)
	{
		for (char c : str) {
			mSerialData.push_back(c);
		}
	}

	/**
	 * Return number of bytes enqueued in serial buffer.
	 */
	std::size_t serialDataAvailable() const
	{
		return mSerialData.size();
	}

	/**
	 * Return the next byte (char) to be read from the serial buffer.
	 */
	char peekSerial() const
	{
		if (mSerialData.empty()) {
			return '\0';
		}

		return mSerialData.front();
	}

	/**
	 * Return and pop one char from the serial data buffer.
	 */
	char readSerial()
	{
		if (mSerialData.empty()) {
			return '\0';
		}

		char res = mSerialData.front();
		mSerialData.pop_front();
		return res;
	}
};

#endif
