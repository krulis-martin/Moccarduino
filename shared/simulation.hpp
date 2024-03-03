#ifndef MOCCARDUINO_SHARED_SIMULATION_HPP
#define MOCCARDUINO_SHARED_SIMULATION_HPP

#include "emulator.hpp"

#include <map>
#include <string>
#include <algorithm>
#include <deque>


/**
 * Handles arduino simulation using an instance of arduino emulator (which holds the state).
 * Controller basically provides external interface for emulator which for the purposes
 * of testing the implementation.
 */
class ArduinoSimulationController
{
private:
	ArduinoEmulator& mEmulator;

	/**
	 * Mapping between actual emulator flags and their names (which can be provided in text configuration).
	 */
	std::map<std::string, bool*> mEnableMethodFlags;

	/**
	 * Input buffers (future time series) that are used to store input events.
	 * These buffers are created and attached as event consumers to input pins.
	 */
	std::map<pin_t, FutureTimeSeries<ArduinoPinState>> mInputBuffers;

	/**
	 * Registered simulation inputs, strings that will be sent as serial data (at given time)
	 */
	std::deque<std::pair<logtime_t, std::string>> mSerialInput;

	void setMethodEnableFlag(const std::string& name, bool enabled)
	{
		auto it = mEnableMethodFlags.find(name);
		if (it == mEnableMethodFlags.end()) {
			throw ArduinoEmulatorException("Invalid API function name '" + name + "'.");
		}
		*(it->second) = enabled;
	}

	void advanceCurrentTimeBy(logtime_t time)
	{
		logtime_t currentTime = mEmulator.advanceCurrentTimeBy(time);
		while (!mSerialInput.empty() && mSerialInput.front().first <= currentTime) {
			mEmulator.addSerialData(mSerialInput.front().second);
			mSerialInput.pop_front();
		}
	}

public:
	ArduinoSimulationController(ArduinoEmulator& emulator) : mEmulator(emulator)
	{
		removeAllPins();
		mEmulator.reset();

		mEnableMethodFlags["pinMode"] = &emulator.mEnablePinMode;
		mEnableMethodFlags["digitalWrite"] = &emulator.mEnableDigitalWrite;
		mEnableMethodFlags["digitalRead"] = &emulator.mEnableDigitalRead;
		mEnableMethodFlags["analogRead"] = &emulator.mEnableAnalogRead;
		mEnableMethodFlags["analogReference"] = &emulator.mEnableAnalogReference;
		mEnableMethodFlags["analogWrite"] = &emulator.mEnableAnalogWrite;
		mEnableMethodFlags["millis"] = &emulator.mEnableMillis;
		mEnableMethodFlags["micros"] = &emulator.mEnableMicros;
		mEnableMethodFlags["delay"] = &emulator.mEnableDelay;
		mEnableMethodFlags["delayMicroseconds"] = &emulator.mEnableDelayMicroseconds;
		mEnableMethodFlags["pulseIn"] = &emulator.mEnablePulseIn;
		mEnableMethodFlags["pulseInLong"] = &emulator.mEnablePulseInLong;
		mEnableMethodFlags["shiftOut"] = &emulator.mEnableShiftOut;
		mEnableMethodFlags["shiftIn"] = &emulator.mEnableShiftIn;
		mEnableMethodFlags["tone"] = &emulator.mEnableTone;
		mEnableMethodFlags["noTone"] = &emulator.mEnableNoTone;
		mEnableMethodFlags["serial"] = &emulator.mEnableSerial;

		// enable all methods at the beginning
		for (auto& [_, flagPtr] : mEnableMethodFlags) {
			*flagPtr = true;
		}
	}

	/**
	 * Returns current simulation (logical) time.
	 */
	logtime_t getCurrentTime() const
	{
		return mEmulator.mCurrentTime;
	}

	/**
	 * Enable given method in emulator. At the beginning, all methods are enabled.
	 */
	void enableMethod(const std::string& name)
	{
		setMethodEnableFlag(name, true);
	}

	/**
	 * Disable given method in emulator. If the tested code tries to call it, exception is thrown.
	 */
	void disableMethod(const std::string& name)
	{
		setMethodEnableFlag(name, false);
	}

	/**
	 * Remove all registered pins.
	 */
	void removeAllPins()
	{
		mEmulator.removeAllPins();
	}

	/**
	 * Registers a new pin with given index and wiring settings.
	 */
	void registerPin(pin_t pin, int wiring = ArduinoPin::UNDEFINED)
	{
		mEmulator.registerPin(pin, wiring);
	}

	/**
	 * Attach an event consumer to an output pin. The cosumer receives all events produced by the pin.
	 */
	void attachPinEventsConsumer(pin_t pin, EventConsumer<ArduinoPinState> &consumer)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		arduinoPin.lastConsumer()->attachNextConsumer(consumer);
	}

	/**
	 * Get current pin value.
	 */
	int getPinValue(pin_t pin) const
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		return arduinoPin.mState.value;
	}

	/**
	 * Enqueue a change of given pin using pin events (works only on input pins).
	 * The event is scheduled at current time (with optional delay).
	 */
	void enqueuePinValueChange(pin_t pin, int value, logtime_t delay = 0)
	{
		bool needsRegistration = mInputBuffers.find(pin) == mInputBuffers.end();
		mInputBuffers[pin].addFutureEvent(mEmulator.mCurrentTime + delay, ArduinoPinState(pin, value));

		if (needsRegistration) {
			mEmulator.registerPinInput(pin, mInputBuffers[pin]);
		}
	}

	void enqueueSerialInputEvent(const std::string& input, logtime_t delay = 0)
	{
		logtime_t time = mEmulator.mCurrentTime + delay;
		if (!mSerialInput.empty() && mSerialInput.back().first > time) {
			throw ArduinoEmulatorException("Adding serial input event at " + std::to_string(time)
				+ " would violate ordering, since last event is already scheduled at "
				+ std::to_string(mSerialInput.back().first) + ".");
		}
		mSerialInput.emplace_back(std::make_pair(time, input));
	}


	/**
	 * Clear all events for pin's queue.
	 */
	void clearPinEvents(pin_t pin)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		arduinoPin.clear();
	}

	/**
	 * Remove all scheduled serial events.
	 */
	void clearSerialInputEvents()
	{
		mSerialInput.clear();
	}

	/**
	 * Invoke the setup function.
	 * @param setupDelay How much is internal clock advanced after the setup.
	 */
	void runSetup(logtime_t setupDelay = 1)
	{
		mEmulator.invokeSetup();
		advanceCurrentTimeBy(setupDelay);
	}

	/**
	 * Invoke one iteration of the loop function.
	 * @param loopDelay How much is internal clock advanced after every loop.
	 */
	void runSingleLoop(logtime_t loopDelay = 1)
	{
		mEmulator.invokeLoop();
		advanceCurrentTimeBy(loopDelay);
	}

	/**
	 * Run the loop given number of times.
	 * @param count
	 * @param loopDelay How much is internal clock advanced after every loop.
	 * @param callback function that takes current time and returns bool (when returns false, the loops are interrupted)
	 */
	void runMultipleLoops(std::size_t count = 1, logtime_t loopDelay = 1,
		std::function<bool(logtime_t)> callback = [](logtime_t) { return true; })
	{
		while (count > 0) {
			runSingleLoop(loopDelay);
			--count;

			if (!callback(getCurrentTime())) {
				break;
			}
		}
	}

	/**
	 * Run loops for given time period.
	 * @param period How long whould we loop.
	 * @param loopDelay How much is internal clock advanced after every loop.
	 */
	void runLoopsForPeriod(logtime_t period, logtime_t loopDelay = 1,
		std::function<bool(logtime_t)> callback = [](logtime_t) { return true; })
	{
		logtime_t endTime = getCurrentTime() + period;
		while (getCurrentTime() < endTime) {
			runSingleLoop(loopDelay);

			if (!callback(getCurrentTime())) {
				break;
			}
		}
	}
};

#endif

