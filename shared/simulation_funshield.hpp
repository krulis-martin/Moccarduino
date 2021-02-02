#ifndef MOCCARDUINO_SHARED_SIMULATION_FUNSHIELD_HPP
#define MOCCARDUINO_SHARED_SIMULATION_FUNSHIELD_HPP

#include "simulation.hpp"
#include "led_display.hpp"
#include "constants.hpp"
#include "funshield.h"

/**
 * Simulation controller for the funshield which is attached to Arduino simulator.
 */
class FunshieldSimulationController
{
public:
	using leds_display_t = LedDisplay<4>;
	using seg_display_t = SerialSegLedDisplay<4>;
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
	 * A collection of 4 LEDs (a simple display).
	 */
	leds_display_t mLeds;

	/**
	 * Four-digit 7-seg LED display controlled by a serial line and a shift register.
	 */
	seg_display_t mSegDisplay;

	// Simulation parameters

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

		// attach displays (event consumers)
		mLeds.attachToSimulation(mArduino, mLedPins);
		mSegDisplay.attachToSimulation(mArduino, data_pin, clock_pin, latch_pin);
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
	 * Get LEDs grouped into simple display object.
	 */
	leds_display_t& getLeds()
	{
		return mLeds;
	}

	/**
	 * Get LEDs grouped into simple display object.
s	 */
	const leds_display_t& getLeds() const
	{
		return mLeds;
	}

	/**
	 * Get an object representing 7-seg LED display.
	 */
	seg_display_t& getSegDisplay()
	{
		return mSegDisplay;
	}

	/**
	 * Get an object representing 7-seg LED display.
	 */
	const seg_display_t& getSegDisplay() const
	{
		return mSegDisplay;
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
		bool bouncing = mButtonBouncingDelay > 0 && mButtonBouncingDelay * 10 <= duration;
		buttonDown(button, afterDelay, bouncing);
		buttonUp(button, afterDelay + duration, bouncing);
	}
};

#endif
