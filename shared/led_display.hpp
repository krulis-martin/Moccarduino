#ifndef MOCCARDUINO_SHARED_LED_DISPLAY_HPP
#define MOCCARDUINO_SHARED_LED_DISPLAY_HPP

#include <emulator.hpp>
#include <helpers.hpp>
#include <constants.hpp>
#include <funshield.h>

#include <string>
#include <deque>
#include <stdexcept>

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
		auto res = mState.get<std::uint8_t>(idx * 8);
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
 * DEPRECATED - TODO remove this (integrate generic demultiplexing in time series) 
 * Represents state of one digit of 7-seg LED display.
 * Also provides means for state demultiplexing and pattern detection.
 */
class LedDisplayDigit
{
public:
	using state_t = std::uint8_t;

private:
	/**
	 * Records one change of the value of the pin.
	 */
	struct Event {
	public:
		logtime_t time;	///< time of the change
		state_t state;	///< new state of the display (one bit representing one LED)

		Event(logtime_t t, state_t s) : time(t), state(s) {}
	};

	logtime_t mLastTime;			///< Time when last state was consolidated.
	state_t mLastState;			///< Current state of the LED display.
	state_t mDemultiplexedState;	///< Last state aggregated when time multiplexing is considered.
	std::deque<Event> mEvents;

	/**
	 * Update active times collected stats by given state.
	 * @param activeTimes array[8] of accumulated times for each bit
	 * @param state current state added to the stats
	 * @param dt how long was current state active.
	 */
	void updateActiveTimesFromState(logtime_t* activeTimes, state_t state, logtime_t dt) const
	{
		for (std::size_t bit = 0; bit < 8; ++bit) {
			activeTimes[bit] += bitRead(mLastState, bit) == ON ? dt : 0;
		}
	}


public:
	LedDisplayDigit() : mLastTime(0), mLastState(0xff), mDemultiplexedState(0xff) {}

	logtime_t getLastTime() const
	{
		return mLastTime;
	}

	state_t getLastState() const
	{
		return mLastState;
	}

	state_t getDemultiplexedState() const
	{
		return mDemultiplexedState;
	}


	void addNewState(logtime_t time, state_t state)
	{
		if (!mEvents.empty()) {
			if (mEvents.back().time > time) {
				throw ArduinoEmulatorException("LED state recording detected violation of causality. Probably internal emulator error.");
			}
			if (mEvents.back().time == time) {
				mEvents.back().state = state;
				return;
			}
		}
		mEvents.push_back(Event(time, state));
	}


	/**
	 * Perform event demultiplexing and consolidate last state.
	 * @param currentTime The currentTime-lastTime gap defines the consolidation window. Also this becomes new lastTime.
	 * @param threshold Minimal time a LED had to be ON in consolidation window to consider it lit (in demultiplexed state).
	 * @param maxTimeSlice If greater than 0, it restricts the maximal size of the consolidation window.
	 * @return New demultiplexing state.
	 */
	state_t demultiplexing(logtime_t currentTime, std::size_t threshold, logtime_t maxTimeSlice = 0)
	{
		if (currentTime < mLastTime) {
			return mDemultiplexedState;
		}

		// crop all parameters to avoid undefined situations...
		maxTimeSlice = std::min(maxTimeSlice, currentTime);
		logtime_t fromTime = std::max(mLastTime, currentTime - maxTimeSlice);
		threshold = std::min(threshold, currentTime - fromTime);

		// make sure we have the right starting state
		while (!mEvents.empty() && mEvents.front().time < fromTime) {
			mLastState = mEvents.front().state;
			mEvents.pop_front();
		}

		// compute how long was each LED on in total
		logtime_t activeTimes[8];
		std::fill(activeTimes, activeTimes + 8, 0);
		while (!mEvents.empty() && mEvents.front().time <= currentTime) {
			// add time delta to all LEDs which have been lit for that duration
			updateActiveTimesFromState(activeTimes, mEvents.front().state, mEvents.front().time - fromTime);

			fromTime = mEvents.front().time;
			mLastState = mEvents.front().state;
			mEvents.pop_front();
		}

		// add last trailing time slot to active times
		if (fromTime < currentTime) {
			updateActiveTimesFromState(activeTimes, mLastState, currentTime - fromTime);
		}

		// update demuxed state based on collected active times and given threshold
		mDemultiplexedState = 0;
		for (std::size_t bit = 0; bit < 8; ++bit) {
			bitWrite(mDemultiplexedState, bit, activeTimes[bit] >= threshold ? ON : OFF);
		}

		mLastTime = currentTime;
		return mDemultiplexedState;
	}
};


/**
 * Led display controlled by a serial line (data & clock pins).
 */
class SerialLedDisplay
{
private:
	/**
	 * Display digits (0 is the leftmost digit)
	 * TODO replace this with time series that keep the whole state
	 */
	std::vector<LedDisplayDigit> mDigits;

	/**
	 * Register that accumulates data from the serial line.
	 */
	ShiftRegister mShiftRegister;

	/**
	 * Last value of data pin that feeds the register.
	 */
	bool mDataInput;

	/**
	 * Last value of clock pin that controls when data pin moves to the register.
	 */
	bool mClockInput;

	/**
	 * State of the output latch. When open, the shift register data are passed out to the display.
	 * When closed, the outputs are frozen (register changes will not affect the display).
	 */
	bool mLatchOpen;

	/**
	 * Update the states of all digits based on the data in the shift register.
	 * @param time actual logical time of the event that triggered this update
	 */
	void updateDigits(logtime_t time)
	{
		if (!mLatchOpen) {
			return; // latch is closed, no update
		}

		auto digits = mShiftRegister.get<std::uint8_t>(0);
		auto state = mShiftRegister.get<LedDisplayDigit::state_t>(1);

		// update the states based on the current data from shift register
		for (std::size_t i = 0; i < mDigits.size(); ++i) {
			mDigits[i].addNewState(time, bitRead(digits, i) ? state : LED_7SEG_EMPTY_SPACE);
		}
	}

public:
	SerialLedDisplay() :
		mDigits(4),
		mShiftRegister(16),
		mDataInput(false),
		mClockInput(false),
		mLatchOpen(true)
	{}

	/**
	 * Method called by the simulator to feed the input (changes on the serial line).
	 */
	void processPinEvent(pin_t pin, int value, logtime_t time)
	{
		bool binValue = value == HIGH ? true : false;

		// yes, this if-else is not ideal, but what the heck, there are only 3 pins of interest
		if (pin == clock_pin) {
			if (mClockInput && !binValue) {
				// clock pin confirms data pin when going from HIGH (current value) to LOW (new value)
				mShiftRegister.push(mDataInput);
				updateDigits(time);
			}
			mClockInput = binValue;
		}
		else if (pin == data_pin) {
			mDataInput = binValue;
		}
		else if (pin == latch_pin) {
			mLatchOpen = binValue;
			updateDigits(time);
		}
		else {
			throw std::runtime_error("Unknown pin number " + std::to_string(pin) + ".");
		}
	}

	/**
	 * Get the number of digits of the display.
	 */
	std::size_t digits() const
	{
		return mDigits.size();
	}

	/**
	 * Get an object representing one display digit.
	 * @param idx index of the digit (0 = leftmost one)
	 */
	LedDisplayDigit& getDigit(std::size_t idx)
	{
		return mDigits[idx];
	}

	/**
	 * Get an object representing one display digit.
	 * @param idx index of the digit (0 = leftmost one)
	 */
	const LedDisplayDigit& getDigit(std::size_t idx) const
	{
		return mDigits[idx];
	}

	/**
	 * Perform demultiplexing on the whole display. Consolidate states and get most likely glyphs being displayed.
	 * @param currentTime The currentTime-lastTime gap defines the consolidation window. Also this becomes new lastTime.
	 * @param threshold Minimal time a LED had to be ON in consolidation window to consider it lit (in demultiplexed state).
	 * @param maxTimeSlice If greater than 0, it restricts the maximal size of the consolidation window.
	 */
	void demultiplexing(logtime_t currentTime, std::size_t threshold, logtime_t maxTimeSlice = 0)
	{
		for (auto&& digit : mDigits) {
			digit.demultiplexing(currentTime, threshold, maxTimeSlice);
		}
	}
};


#endif
