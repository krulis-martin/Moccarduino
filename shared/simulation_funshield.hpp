#ifndef MOCCARDUINO_SHARED_SIMULATION_FUNSHIELD_HPP
#define MOCCARDUINO_SHARED_SIMULATION_FUNSHIELD_HPP

#include <simulation.hpp>
#include <led_display.hpp>
#include <constants.hpp>
#include <funshield.h>

/**
 * Simulation controller for funshield which is attached to Arduino simulator.
 */
class FunshieldSimulationController
{
private:
	/**
	 * Underlying arduino simulator used for lowlevel operations and code invocation.
	 */
	ArduinoSimulationController& mArduino;

	/**
	 * Shield buttons pin references.
	 */
	std::vector<pin_t> mButtionPins = { button1_pin, button2_pin, button3_pin };

	/**
	 * Shield independent LEDs pin references.
	 */
	std::vector<pin_t> mLedPins = { led1_pin, led2_pin, led3_pin, led4_pin };

	/**
	 * Objects representing individual digits of 7seg display.
	 */
	std::vector<LedDisplayDigit> mDisplayDigits;

	/**
	 * Shift register used to control the display via serial line.
	 */
	ShiftRegister mDisplayShiftRegister;

	// Simulation parameters

	/**
	 * Loop invocation overhead (how much is the time advanced after every loop).
	 */
	logtime_t mLoopDelay;

	/**
	 * When button bouncing (jitter) is simulated, this is the delay between two state changes.
	 * If zero, no bouncing is employed.
	 */
	logtime_t mButtonBouncingDelay;

public:
	/**
	 * Funshield needs Arduino simulation controller which is used for low-level simulation operations.
	 */
	FunshieldSimulationController(ArduinoSimulationController& arduino) :
		mArduino(arduino),
		mDisplayDigits(4),
		mDisplayShiftRegister(16),
		mLoopDelay(100),
		mButtonBouncingDelay(0) // 0 = disabled
	{
		// initialize pins
		for (auto pin : mButtionPins) {
			mArduino.registerPin(pin, INPUT);
		}
		for (auto pin : mLedPins) {
			mArduino.registerPin(pin, OUTPUT);
		}

		// serial interface for the 7seg LEDs
		mArduino.registerPin(latch_pin, OUTPUT);
		mArduino.registerPin(clock_pin, OUTPUT);
		mArduino.registerPin(data_pin, OUTPUT);
	}

	// accessor for underlying arduino simulator
	ArduinoSimulationController& getArduino()
	{
		return mArduino;
	}

	// const accessor for underlying arduino simulator
	const ArduinoSimulationController& getArduino() const
	{
		return mArduino;
	}

	/**
	 * Press button (schedule event).
	 * @param button zero-based index of the button (0 is button1)
	 * @param afterDelay schedule the button to be pressed after given amount of (logical) time
	 * @param bouncing flag indicates whether bouncing effect will be applied (also the bouncing delay must be > 0)
	 */
	void buttonDown(std::size_t button, logtime_t afterDelay = 0, bool bouncing = true)
	{
		mArduino.enqueuePinValueChange(mButtionPins[button], LOW, afterDelay);

		if (bouncing && mButtonBouncingDelay > 0) {
			logtime_t delay = afterDelay;
			for (std::size_t i = 1; i <= 3; ++i) {
				delay += mButtonBouncingDelay;
				buttonUp(button, delay, false);
				delay += mButtonBouncingDelay;
				buttonDown(button, delay, false);
			}
		}
	}

	/**
	 * Release button (schedule event).
	 * @param button zero-based index of the button (0 is button1)
	 * @param afterDelay schedule the button to be pressed after given amount of (logical) time
	 * @param bouncing flag indicates whether bouncing effect will be applied (also the bouncing delay must be > 0)
	 */
	void buttonUp(std::size_t button, logtime_t afterDelay = 0, bool bouncing = true)
	{
		mArduino.enqueuePinValueChange(mButtionPins[button], HIGH, afterDelay);

		if (bouncing && mButtonBouncingDelay > 0) {
			logtime_t delay = afterDelay;
			for (std::size_t i = 1; i <= 3; ++i) {
				delay += mButtonBouncingDelay;
				buttonDown(button, delay, false);
				delay += mButtonBouncingDelay;
				buttonUp(button, delay, false);
			}
		}
	}

	/**
	 * Schedlue button click events (button goes down and up again).
	 * @param button zero-based index of the button (0 is button1)
	 * @param duration how long the button stays down (100ms is default)
	 * @param afterDelay schedule the button to be pressed after given amount of (logical) time
	 */
	void buttonClick(std::size_t button, logtime_t duration = 100000, logtime_t afterDelay = 0)
	{
		bool bouncing = mButtonBouncingDelay * 10 <= duration;
		buttonDown(button, afterDelay, bouncing);
		buttonUp(button, afterDelay + duration, bouncing);
	}

	/**
	 * Get current status of given independent LED.
	 * @param led zero-based index (0 is led1)
	 * @return true if the LED is lit, false when dimmed
	 */
	bool getLedStatus(std::size_t led) const
	{
		return mArduino.getPinValue(mLedPins[led]) == LOW; // LOW = LED is lit
	}

	/**
	 * Get aggregated state of all independent LEDs 
	 * @return byte containing LED states as individual bits (bit 0 ~ LED 0, 1 = LED is lit)
	 */
	unsigned int getAllLedsStatus() const
	{
		unsigned int res = 0;
		for (std::size_t i = 0; i < mLedPins.size(); ++i) {
			res = res << 1;
			if (getLedStatus(i)) {
				res |= 1;
			}
		}
		return res;
	}

	void processDisplayControlPins()
	{
		TimeSeries<ArduinoPin::PinState> events;
		events.merge({
			mArduino.getPinEvents(latch_pin),
			mArduino.getPinEvents(clock_pin),
			mArduino.getPinEvents(data_pin),
		});

	}

	/**
	 *
	 */
	void runLoop(logtime_t timeLimit = 1000000, std::size_t countLimit = ~(std::size_t)0)
	{
		for (auto&& ledPin : mLedPins) {
			mArduino.clearPinEvents(ledPin);
		}

		mArduino.runSingleLoop(mLoopDelay);
	}
};

#endif
