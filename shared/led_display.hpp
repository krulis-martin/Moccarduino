#ifndef MOCCARDUINO_SHARED_LED_DISPLAY_HPP
#define MOCCARDUINO_SHARED_LED_DISPLAY_HPP

#include "simulation.hpp"
#include "emulator.hpp"
#include "helpers.hpp"
#include "constants.hpp"
#include "funshield.h"

#include <string>
#include <deque>
#include <stdexcept>
#include <limits>
#include <cstdint>

constexpr std::uint8_t LED_7SEG_EMPTY_SPACE = 0b11111111;
constexpr std::uint8_t LED_7SEG_DASH = 0b10111111;
constexpr std::uint8_t LED_7SEG_DECIMAL_DOT = 0b01111111;

constexpr std::uint8_t LED_7SEG_DIGITS_MAP[]{
  0b11000000,	// 0
  0b11111001,	// 1
  0b10100100,	// 2
  0b10110000,	// 3
  0b10011001,	// 4
  0b10010010,	// 5
  0b10000010,	// 6
  0b11111000,	// 7
  0b10000000,	// 8
  0b10010000,	// 9
};

constexpr std::uint8_t LED_7SEG_LETTERS_MAP[]{
  0b10001000,   // A
  0b10000011,   // b
  0b11000110,   // C
  0b10100001,   // d
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
  0b10010001,   // Y
  0b10100100,   // Z
};


/**
 * Wrapper class for Leds state (bit array) that interprets digits and symbols on the display.
 */
template<int DIGITS>
class Led7SegInterpreter
{
public:
	using state_t = BitArray<DIGITS * 8>;
	static const int INVALID_NUMBER = -1;
	static const char INVALID_CHAR = 0x7f;

private:
	state_t mState;

public:
	Led7SegInterpreter(const state_t &state) : mState(state) {}

	/**
	 * Return raw LED data of given digit (7bits per segment + 1bit decimal dot).
	 * @param idx index of the digit (0..DIGITS-1, from left to right)
	 */
	std::uint8_t getDigitRaw(std::size_t idx, bool maskDecimalDot = false) const
	{
		auto res = mState.template get<std::uint8_t>(idx * 8);
		if (maskDecimalDot) {
			res = res | ~LED_7SEG_DECIMAL_DOT; // mask out
		}
		return res;
	}

	/**
	 * Check whether there is a decimal dot lit at given digit position.
	 * @param idx index of the digit (0..DIGITS-1, from left to right)
	 */
	bool hasDecimalDot(std::size_t idx) const
	{
		return (getDigitRaw(idx) & ~LED_7SEG_DECIMAL_DOT) == 0; // 0 ~ LED is active
	}

	/**
	 * Returns true if there is more than one decimal digit active on the display.
	 */
	bool decimalDotAmbiguous() const
	{
		std::size_t count = 0;
		for (std::size_t digit = 0; digit < DIGITS; ++digit) {
			if (hasDecimalDot(digit)) {
				++count;
			}
		}
		return count > 1;
	}

	/**
	 * Return index of the leftmost decimal digit that is active.
	 * @return index from 0 (leftmost) to DIGIT-1 (rightmost) range;
	           DIGIT-1 is returned when no dot is active (implicit decimal position)
	 */
	std::size_t decimalDotPosition() const
	{
		for (std::size_t digit = 0; digit < DIGITS; ++digit) {
			if (hasDecimalDot(digit)) {
				return digit;
			}
		}
		return DIGITS - 1; // last position is implicit decimal position even if no dot is present
	}

	/**
	 * Detect a nunmeric pattern at given position of 7-seg display.
	 * @param idx index of the digit (0..DIGITS-1, from left to right)
	 * @param detectSpaceAsZero if true, empty display is reported as zero
	 * @return int detected digit (0-9) or INVALID_NUMBER if the digit was not recognized
	 */
	int getDigit(std::size_t idx, bool detectSpaceAsZero = false) const
	{
		char ch = getCharacter(idx, true); // true = prefer digits over letters
		if (detectSpaceAsZero && ch == ' ') {
			return 0; // special case, space is reported as 0
		}

		if (ch >= '0' && ch <= '9') {
			return ch - '0';
		}

		return INVALID_NUMBER;
	}

	/**
	 * Detect an alpha-numeric pattern at given position of 7-seg display and return the corresponding character (letter, digit, space).
	 * All letters are reported lowercased, empty character is space.
	 * @param idx index of the digit (0..DIGITS-1, from left to right)
	 * @return char detected character or INVALID_CHAR constant if the state was not recognized
	 */
	char getCharacter(std::size_t idx, bool preferDigitsOverLetters = false) const
	{
		// internal cached reverse lookup table built from constants of glyphs
		static std::map<std::uint8_t, char> lookupDigits;
		static std::map<std::uint8_t, char> lookupOthers;
		if (lookupDigits.empty()) {
			for (int i = 0; i < 10; ++i) {
				lookupDigits[LED_7SEG_DIGITS_MAP[i]] = '0' + i;
			}
		}
		if (lookupOthers.empty()) {
			for (int i = 0; i < 26; ++i) {
				lookupOthers[LED_7SEG_LETTERS_MAP[i]] = 'a' + i;
			}
			lookupOthers[LED_7SEG_EMPTY_SPACE] = ' ';
			lookupOthers[LED_7SEG_DASH] = '-';
		}

		auto glyph = getDigitRaw(idx, true); // true = mask out the decimal dot (not interesting here)

		auto itDigit = lookupDigits.find(glyph);
		auto itOther = lookupOthers.find(glyph);
		
		if (itDigit != lookupDigits.end() && itOther != lookupOthers.end()) {
			// ambiguous situation, glyph matches both number and not-number
			return preferDigitsOverLetters ? itDigit->second : itOther->second;
		}

		if (itDigit != lookupDigits.end()) {
			return itDigit->second;
		}

		if (itOther != lookupOthers.end()) {
			return itOther->second;
		}

		return INVALID_CHAR;
	}

	/**
	 * Decode a number being shown on the whole display. The decimal dots are ignored, but negative numbers are recognized.
	 * @return the number being displayed or INVALID_NUMBER if it cannot be properly decoded
	 */
	int getNumber() const
	{
		bool negative = false;
		int res = 0;				// result accumulator

		// find index of the first nonempty glyph
		std::size_t idx = 0;
		while (idx < DIGITS && getDigitRaw(idx, true) == LED_7SEG_EMPTY_SPACE) {
			++idx;
		}

		// detect possible '-' sign and move on
		if (idx < DIGITS && getDigitRaw(idx, true) == LED_7SEG_DASH) {
			negative = true;
			++idx;
		}

		if (idx >= DIGITS) {
			return INVALID_NUMBER; // no digits available
		}

		// process remaining digits
		while (idx < DIGITS) {
			int digit = getDigit(idx);
			if (digit == INVALID_NUMBER) {
				return INVALID_NUMBER; // current glyph is not numerical digit
			}
			res = (res * 10) + digit;
			++idx;
		}

		return negative ? -res : res;
	}

	/**
	 * Get text content of the display and return it as a string.
	 * @param invalidCharsReplacement if not zero (char) then invalid characters are replaced with this char,
	 *                                otherwise any invalid char causes return of an empty string
	 * @return sequence of chars as std string, empty string on failure
	 */
	std::string getText(char invalidCharsReplacement = '\0') const
	{
		std::string res;
		for (std::size_t i = 0; i < DIGITS; ++i) {
			char ch = getCharacter(i);
			if (ch == INVALID_CHAR) {
				if (invalidCharsReplacement != '\0') {
					ch = invalidCharsReplacement; // we can patch it
				}
				else {
					return ""; // we cannot patch it -> report failure
				}
			}
			res.append({ ch });
		}
		return res;
	}
};


/**
 * Demultiplexes state changes by computing the time each LED has been lit
 * in given quantization intervals and 
 */
template<int LEDS>
class LedsEventsDemultiplexer : public EventConsumer<BitArray<LEDS>>
{
public:
	using state_t = BitArray<LEDS>;

private:
	/**
	 * Time window for demultiplexing.
	 */
	logtime_t mTimeWindow;

	/**
	 * Minimal time (in each time window) a LED has to be ON, so we consider it ON in demuxed state.
	 */
	logtime_t mThreshold;

	/**
	 * Time stamp where currently opened time window should be closed.
	 */
	logtime_t mNextMarker;

	/**
	 * Last encountered state set by addEvent().
	 */
	state_t mLastState;

	/**
	 * Demultiplexed state stored by last closed window.
	 */
	state_t mLastDemuxedState;

	/**
	 * Accumulated active times for each LED in the last time window.
	 */
	std::array<logtime_t, LEDS> mActiveTimes;

	/**
	 * Compute new demuxed state from the accumulated active times and reset active times in the process.
	 */
	state_t demuxState()
	{
		state_t newState(OFF);
		for (std::size_t i = 0; i < LEDS; ++i) {
			if (mActiveTimes[i] >= mThreshold) {
				newState.set(ON, i, 1); // the LED has been ON for sufficient amount of time
			}
			mActiveTimes[i] = 0;
		}
		return newState;
	}

	/**
	 * Use last know state and increase time accumulators for all LEDs which are currently ON.
	 * @param dt how much time have passed since the last update
	 */
	void accumulateActiveTimes(logtime_t dt)
	{
		for (std::size_t i = 0; i < LEDS; ++i) {
			if (mLastState[i] == ON) {
				mActiveTimes[i] += dt;
			}
		}
	}

	/**
	 * Returns true if currently a event processing window is open.
	 */
	bool isWindowOpen() const
	{
		return this->mLastTime < mNextMarker;
	}

	/**
	 * Update or even close currently opened window usign given timestamp.
	 * @param time actual time (given either by advance time or when event is added)
	 */
	void updateOpenedWindow(logtime_t time)
	{
		if (!isWindowOpen()) {
			return;
		}

		if (time >= mNextMarker) {
			// time has passed the closing marker...

			// traliling time fragment needs to be accumulated
			accumulateActiveTimes(mNextMarker - this->mLastTime);

			this->mLastTime = mNextMarker; // everyting up to the marker is resolved

			// process the last opened window
			auto demuxedState = demuxState(); // assemble new demuxed state from the window
			if (mLastDemuxedState != demuxedState) {
				// demuxed state has changed
				mLastDemuxedState = demuxedState;
				if (this->nextConsumer() != nullptr) {
					// emit event for following consumers
					this->nextConsumer()->addEvent(mNextMarker, demuxedState);
				}
				mNextMarker += mTimeWindow; // time window shifts one place
			}
			else {
				if (this->nextConsumer() != nullptr) {
					// no event -> just advance time for following consumers
					this->nextConsumer()->advanceTime(mNextMarker);
				}

				if (mLastDemuxedState != mLastState) {
					// if there is a potential the next window will change demuxed state...
					mNextMarker += mTimeWindow; // ...time window shifts one place
				}
			}

		}

		// Yes, there is no "else" here. The previous block may have shifted the window,
		// so we need to update it (unless the time was advanced much further into the future).

		if (time < mNextMarker) {
			// update the current window
			accumulateActiveTimes(time - this->mLastTime);
		}
	}


protected:
	void doAddEvent(logtime_t time, state_t state) override
	{
		do {
			updateOpenedWindow(time); // update, possibly close current window
		} while (isWindowOpen() && time >= mNextMarker);
		mLastState = state;
		if (!isWindowOpen()) {
			// the event triggers opening of a new window
			mNextMarker = time + mTimeWindow;
		}
	}

	void doAdvanceTime(logtime_t time)
	{
		do {
			updateOpenedWindow(time); // update, possibly close current window
		} while (isWindowOpen() && time >= mNextMarker);
		if (!isWindowOpen() && this->nextConsumer() != nullptr) {
			// if no window is open, we can pass time advances as usual
			this->nextConsumer()->advanceTime(time);
		}
	}

	void doClear() override
	{
		mNextMarker = this->mLastTime;
		mLastState.fill(OFF);
		mLastDemuxedState.fill(OFF);
		mActiveTimes.fill(0);
		EventConsumer<BitArray<LEDS>>::doClear();
	}

public:
	/**
	 * @param timeWindow period in which the changes are merged together and evaluated by thresholding
	 * @param threshold how long (inside a time window) a LED needs to be on in given period of time to be considered lit
	 */
	LedsEventsDemultiplexer(logtime_t timeWindow, logtime_t threshold) :
		mTimeWindow(timeWindow),
		mThreshold(threshold),
		mNextMarker(0),
		mLastState(OFF),
		mLastDemuxedState(OFF)
	{
		if (mTimeWindow == 0) {
			throw std::runtime_error("Demultiplexing time window must be greater than 0.");
		}

		if (mThreshold == 0 || mThreshold > mTimeWindow) {
			throw std::runtime_error("Given threshold is out of range of the time window.");
		}

		mActiveTimes.fill(0);
	}

	// Default timeWindow is 50ms and default threshold is 10% of the time window
	LedsEventsDemultiplexer(logtime_t timeWindow = 50000) : LedsEventsDemultiplexer(timeWindow, timeWindow / 10) {}
};


/**
 * Another filter typically used in combination with demultiplexer. It suppresses change events in rapid succession.
 * That might be necessary in situations when demuxer is doing its job, but could not distinguish between actual state
 * changes.
 * Therefore the recommended setup is to use demultiplexer with smaller window (e.g., 10ms) followed by aggregator with
 * larger window (50-100ms).
 */
template<int LEDS>
class LedsEventsAggregator : public EventConsumer<BitArray<LEDS>>
{
public:
	using state_t = BitArray<LEDS>;

private:
	/**
	 * Time window for aggregation.
	 */
	logtime_t mTimeWindow;

	/**
	 * Time stamp where currently opened time window should be closed.
	 */
	logtime_t mNextMarker;

	/**
	 * Last encountered state set by addEvent().
	 */
	state_t mLastState;

	/**
	 * Last state emitted to the next consumer.
	 */
	state_t mLastEmittedState;

	/**
	 * Returns true if currently a event processing window is open.
	 */
	bool isWindowOpen() const
	{
		return this->mLastTime < mNextMarker;
	}

	/**
	 * Update or even close currently opened window usign given timestamp.
	 * @param time actual time (given either by advance time or when event is added)
	 */
	void updateOpenedWindow(logtime_t time)
	{
		if (isWindowOpen() && time >= mNextMarker) {
			// time to shut the window, there is a draft here...

			this->mLastTime = mNextMarker; // everyting up to the marker is resolved

			// process the last opened window
			if (mLastState != mLastEmittedState) {
				mLastEmittedState = mLastState;
				if (this->nextConsumer() != nullptr) {
					// emit event for following consumers
					this->nextConsumer()->addEvent(mNextMarker, mLastEmittedState);
				}
				mNextMarker += mTimeWindow; // time window shifts one place
			}
			else if (this->nextConsumer() != nullptr) {
				// advance time for the following consumer
				this->nextConsumer()->advanceTime(mNextMarker);
			}
		}
	}


protected:
	void doAddEvent(logtime_t time, state_t state) override
	{
		updateOpenedWindow(time); // update, possibly close current window
		mLastState = state;
		if (!isWindowOpen()) {
			// the event triggers opening of a new window
			mNextMarker = time + mTimeWindow;
		}
	}

	void doAdvanceTime(logtime_t time)
	{
		updateOpenedWindow(time); // update, possibly close current window
		if (!isWindowOpen() && this->nextConsumer() != nullptr) {
			// if no window is open, we can pass time advances as usual
			this->nextConsumer()->advanceTime(time);
		}
	}

	void doClear() override
	{
		mNextMarker = this->mLastTime;
		mLastState.fill(OFF);
		mLastEmittedState.fill(OFF);
		EventConsumer<BitArray<LEDS>>::doClear();
	}

public:
	/**
	 * @param timeWindow period in which the changes are merged together and evaluated by thresholding
	 */
	LedsEventsAggregator(logtime_t timeWindow = 50000) :
		mTimeWindow(timeWindow),
		mNextMarker(0),
		mLastState(OFF),
		mLastEmittedState(OFF)
	{
		if (mTimeWindow == 0) {
			throw std::runtime_error("Aggregator time window must be greater than 0.");
		}
	}
};


/**
 * Simple display (a bunch of LEDs), each LED is controlled by its own Arduino pin.
 * @tparam LEDS number of LEDs the display has
 */
template<int LEDS>
class LedDisplay : public ForkedEventConsumer<ArduinoPinState, BitArray<LEDS>>
{
public:
	using state_t = BitArray<LEDS>;

private:
	/**
	 * Actual state of the LEDs
	 */
	state_t mState;

	/**
	 * Information about wiring. Translates pins to LED indices.
	 */
	std::map<pin_t, std::size_t> mLedPins;

protected:
	void doAddEvent(logtime_t time, ArduinoPinState state) override
	{
		auto it = mLedPins.find(state.pin);
		if (it == mLedPins.end()) {
			// ignore unknown pins, but we can advance time at least
			EventConsumer<ArduinoPinState>::doAdvanceTime(time);
			if (this->sproutConsumer() != nullptr) {
				this->sproutConsumer()->advanceTime(time);
			}
			return;	
		}

		// update the state
		auto idx = it->second;
		bool value = state.value == ON ? ON : OFF;
		if (mState[idx] != value) {
			// the state actually changes
			mState.set(value, idx, 1);
			if (this->sproutConsumer() != nullptr) {
				this->sproutConsumer()->addEvent(time, mState);
			}
		}

		EventConsumer<ArduinoPinState>::doAddEvent(time, state);
	}

public:
	LedDisplay() : mState(OFF) {}

	/**
	 * Attach the LED display to existing simulation (connect as event consumer to corresponding pins).
	 */
	void attachToSimulation(ArduinoSimulationController& simulation, const std::vector<pin_t> wiring)
	{
		if (wiring.size() != LEDS) {
			throw std::runtime_error("Display with " + std::to_string(LEDS) + " LEDs cannot be connected to "
				+ std::to_string(wiring.size()) + " pins.");
		}

		for (std::size_t i = 0; i < wiring.size(); ++i) {
			if (mLedPins.find(wiring[i]) != mLedPins.end()) {
				throw std::runtime_error("Pin " + std::to_string(wiring[i]) + " is attached to multiple LEDs.");
			}
			mLedPins[wiring[i]] = i;
			simulation.attachPinEventsConsumer(wiring[i], *this);
		}
	}

	state_t getState() const
	{
		return mState;
	}
};


/**
 * 7-seg LED display controlled by a serial line (data & clock pins).
 * The display is ArduinoPinState event consumer since it consumes pin events
 * from three different pins. It is also an event emitter (forked consumer);
 * display state events are produced off the sprout.
 */
template<int DIGITS>
class SerialSegLedDisplay : public ForkedEventConsumer<ArduinoPinState, BitArray<DIGITS * 8>>
{
public:
	using state_t = BitArray<DIGITS * 8>;

private:
	state_t mState;

	/**
	 * Register that accumulates data from the serial line.
	 */
	ShiftRegister mShiftRegister;

	// pin numbers of associated inputs
	pin_t mDataInputPin;
	pin_t mClockInputPin;
	pin_t mLatchPin;

	/**
	 * Last value of data pin that feeds the register.
	 */
	bool mDataInput;

	/**
	 * Last value of clock pin that controls when data pin moves to the register.
	 */
	bool mClockInput;

	/**
	 * State of the latch pin. The shift register updates outputs with raise edge on latch pin.
	 */
	bool mLatch;

	/**
	 * Update the states of all digits based on the data in the shift register.
	 * @param time actual logical time of the event that triggered this update
	 */
	void updateState(logtime_t time)
	{
		auto activeDigits = mShiftRegister.get<std::uint8_t>(0);
		auto glyph = mShiftRegister.get<std::uint8_t>(1);

		state_t newState(true);
		for (std::size_t d = 0; d < DIGITS; ++d) {
			if (bitRead(activeDigits, d)) {
				newState.template set<std::uint8_t>(glyph, d * 8);
			}
		}

		if (newState != mState) {
			mState = newState;
			if (this->sproutConsumer() != nullptr) {
				this->sproutConsumer()->addEvent(time, mState);
			}
		}
	}

protected:
	void doAddEvent(logtime_t time, ArduinoPinState state) override
	{
		bool pinValue = state.value == HIGH ? true : false;

		// yes, this if-else is not ideal, but what the heck, there are only 3 pins of interest
		if (state.pin == mClockInputPin) {
			if (mClockInput && !pinValue) {
				// clock pin confirms data pin when going from HIGH (current value) to LOW (new value)
				mShiftRegister.push(mDataInput);
			}
			mClockInput = pinValue;
		}
		else if (state.pin == mDataInputPin) {
			mDataInput = pinValue;
		}
		else if (state.pin == mLatchPin) {
			if (!mLatch && pinValue) {
				// latch goes from LOW to HIGH
				updateState(time);
			}
			mLatch = pinValue;
		}
		else {
			throw std::runtime_error("Unknown pin number " + std::to_string(state.pin) + ".");
		}

		// pass the event along
		EventConsumer<ArduinoPinState>::doAddEvent(time, state);

		if (this->sproutConsumer() != nullptr) {
			this->sproutConsumer()->advanceTime(time); // actual new events are emitted in updateState
		}
	}

public:
	SerialSegLedDisplay() :
		mState(OFF), // all LEDS are dimmed
		mShiftRegister(16),	// 8 (glyph mask) + one bit per digit (max 8 at this point)
		mDataInputPin(std::numeric_limits<pin_t>::max()), // invalid value
		mClockInputPin(std::numeric_limits<pin_t>::max()), // invalid value
		mLatchPin(std::numeric_limits<pin_t>::max()), // invalid value
		mDataInput(false),
		mClockInput(false),
		mLatch(false)
	{}

	/**
	 * Attach the LED display to existing simulation (connect as event consumer to corresponding pins).
	 */
	void attachToSimulation(ArduinoSimulationController& simulation,
		pin_t dataInputPin = data_pin, pin_t clockInputPin = clock_pin, pin_t latchPin = latch_pin)
	{
		mDataInputPin = dataInputPin;
		mClockInputPin = clockInputPin;
		mLatchPin = latchPin;
		simulation.attachPinEventsConsumer(dataInputPin, *this);
		simulation.attachPinEventsConsumer(clockInputPin, *this);
		simulation.attachPinEventsConsumer(latchPin, *this);
	}

	/**
	 * Return the raw current LED state.
	 */
	state_t getState() const
	{
		return mState;
	}
};


#endif
