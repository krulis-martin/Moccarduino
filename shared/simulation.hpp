#ifndef MOCCARDUINO_SHARED_SIMULATION_HPP
#define MOCCARDUINO_SHARED_SIMULATION_HPP

#include <emulator.hpp>

#include <map>
#include <string>
#include <algorithm>
#include <initializer_list>

class ArduinoSimulationController
{
public:
	/**
	 * Records one change of the value of the pin.
	 */
	struct PinEvent {
	public:
		logtime_t time;	///< time of the change
		int value;		///< new value of the pin
		pin_t pin;		///< identifier of the pin

		PinEvent(logtime_t t, int v, pin_t p) : time(t), value(v), pin(p) {}
	};


private:
	ArduinoEmulator& mEmulator;

	/**
	 * Mapping between actual emulator flags and their names (which can be provided in text configuration).
	 */
	std::map<std::string, bool*> mEnableMethodFlags;

	void setMethodEnableFlag(const std::string& name, bool enabled)
	{
		auto it = mEnableMethodFlags.find(name);
		if (it == mEnableMethodFlags.end()) {
			throw ArduinoEmulatorException("Invalid API function name '" + name + "'.");
		}
		*(it->second) = enabled;
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
	 * Get current pin value.
	 */
	int getPinValue(pin_t pin)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		return arduinoPin.mValue;
	}

	/**
	 * Forceufly set an explicit value to a pin.
	 */
	void setPinValue(pin_t pin, int value)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		arduinoPin.mValue = value;
	}

	/**
	 * Enqueue a change of given pin using pin events (works only on input pins).
	 * The event is scheduled at current time (with optional delay).
	 */
	void enqueuePinValueChange(pin_t pin, int value, logtime_t delay = 0)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		arduinoPin.addEvent(mEmulator.mCurrentTime + delay, value);
	}

	/**
	 * Return const ref to events queue of particular pin so it can be inspected.
	 */
	const std::deque<ArduinoPin::Event>& getPinEventsRaw(pin_t pin) const
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		return arduinoPin.getEvents();
	}

	/**
	 * Return pin events (possibly merged from multiple pins) and return them in our PinEvent structures.
	 * @param events output parameter (a vector that will be filled)
	 * @param pins a list of pins the events of which are collected
	 */
	void getPinEvents(std::vector<PinEvent> &events, std::initializer_list<pin_t> pins) const
	{
		events.clear();

		// aggregate all events from all pins and wrap them into our PinEvent struct
		for (auto&& pin : pins) {
			for (auto&& ev : getPinEventsRaw(pin)) {
				events.emplace_back(ev.time, ev.value, pin);
			}
		}

		// sort events by time (primary) and pin (secondary)
		std::sort(events.begin(), events.end(), [](const PinEvent& a, const PinEvent& b) {
			if (a.time == b.time) {
				return a.time < b.time;
			}
			else {
				return a.pin <= b.pin;
			}
		});
	}

	/**
	 * Clear all events for pin's queue.
	 */
	void clearPinEvents(pin_t pin)
	{
		auto& arduinoPin = mEmulator.getPin(pin);
		arduinoPin.clearEvents();
	}

	/**
	 * Invoke the setup function.
	 * @param setupDelay How much is internal clock advanced after the setup.
	 */
	void runSetup(logtime_t setupDelay = 1)
	{
		mEmulator.invokeSetup();
		mEmulator.advanceCurrentTime(setupDelay);
	}

	/**
	 * Invoke one iteration of the loop function.
	 * @param loopDelay How much is internal clock advanced after every loop.
	 */
	void runSingleLoop(logtime_t loopDelay = 1)
	{
		mEmulator.invokeLoop();
		mEmulator.advanceCurrentTime(loopDelay);
	}

	/**
	 * Run the loop given number of times.
	 * @param loopDelay How much is internal clock advanced after every loop.
	 */
	void runMultipleLoops(std::size_t count = 1, logtime_t loopDelay = 1)
	{
		while (count > 0) {
			runSingleLoop();
			--count;
		}
	}
};

#endif

