#ifndef MOCCARDUINO_SHARED_EMULATOR_HPP
#define MOCCARDUINO_SHARED_EMULATOR_HPP

#include <time_series.hpp>
#include <constants.hpp>

#include <deque>
#include <map>
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


class ArduinoEmulator;

/**
 * Represents one digital arduino pin (input or output).
 */
class ArduinoPin
{
friend class ArduinoSimulationController;
friend class ArduinoEmulator;

public:
	/**
	 * Records one change of the value of the pin.
	 */
	struct PinState {
	public:
		pin_t pin;		///< pin identifier
		int value;		///< new value (either written or received as input)

		PinState() : pin(~(pin_t)0), value(-1) {}
		PinState(pin_t p, int v) : pin(p), value(v) {}

		inline bool operator<(const PinState& ps)
		{
			return pin < ps.pin || (pin == ps.pin && value < ps.value);
		}
	};

	using Event = TimeSeries<PinState>::Event;

private:
	static const int UNDEFINED = -1;

	pin_t mPin;		///< pin identification
	int mWiring;	///< how the pin is actually wired (INPUT/OUTPUT)
	int mMode;		///< current operating mode (INPUT/OUTPUT)
	int mValue;		///< current value of the pin (LOW/HIGH)

	/**
	 * Output pins record write events of the application in this queue.
	 * Input pins use this as pre-recorded events the will be used to emulate changes in input value.
	 */
	TimeSeries<PinState> mEvents;

	/*
	 * Interface for the simulator.
	 */

	void reset()
	{
		clearEvents();
		mMode = UNDEFINED;
		mValue = UNDEFINED;
	}

	/**
	 * Return const ref to events queue so it can be inspected.
	 */
	const TimeSeries<PinState>& getEvents() const
	{
		return mEvents;
	}

	/**
	 * Remove all events from the queue.
	 */
	void clearEvents()
	{
		mEvents.clear();
	}

	/**
	 * Add event into the queue by the simulator (do not perform any checks).
	 */
	void addEvent(logtime_t time, int value)
	{
		mEvents.addEvent(time, PinState(mPin, value));
	}

public:
	ArduinoPin(pin_t pin, int wiring = UNDEFINED) : mPin(pin), mWiring(wiring), mMode(UNDEFINED), mValue(UNDEFINED) {}

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
		mEvents.clear(); // just to be sure

		if (mMode == INPUT && mValue == UNDEFINED) {
			mValue = HIGH;
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

		return mValue;
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

		if (!mEvents.empty() && mEvents.back().time > time) {
			throw ArduinoEmulatorException("Pin writing procedure violated causality. Probably internal emulator error.");
		}

		mValue = value;
		mEvents.addEvent(time, PinState(mPin, value));
	}

	/**
	 * The simulation time has advanced, process input queue.
	 * @param newTime read all input values up to newTime and make the last one the current one
	 */
	void updateTime(logtime_t newTime)
	{
		if (mMode == INPUT) {
			Event lastEvent(0, PinState(mPin, mValue));
			mEvents.consumeEventsUntil(newTime + 1, lastEvent); // time+1 effectively changes "until" from "<" to "<="
		}
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

	void reset()
	{
		mCurrentTime = 0;
		for (auto& [_, arduinoPin] : mPins) {
			arduinoPin.reset();
		}
	}

	/**
	 * Advances the Arduino emulator time forward by given number of microseconds.
	 */
	void advanceCurrentTime(logtime_t byMicroseconds)
	{
		mCurrentTime += byMicroseconds;
		for (auto& [_, arduinoPin] : mPins) {
			arduinoPin.updateTime(mCurrentTime);
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
		mPins.emplace(pin, wiring);
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
		mEnableNoTone(true)
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
		advanceCurrentTime(1);
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
		advanceCurrentTime(1);
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
		advanceCurrentTime(1);
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
		advanceCurrentTime(100); // delay taken from documentation
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

		advanceCurrentTime((logtime_t)1000 * (logtime_t)ms);
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

		advanceCurrentTime((logtime_t)us);
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
};

#endif
