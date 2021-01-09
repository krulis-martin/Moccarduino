#ifndef MOCCARDUINO_SHARED_LED_DISPLAY_HPP
#define MOCCARDUINO_SHARED_LED_DISPLAY_HPP

#include <emulator.hpp>
#include <constants.hpp>
#include <funshield.h>

#include <string>
#include <deque>
#include <stdexcept>

constexpr std::uint8_t LED_7SEG_EMPTY_SPACE = 0b11111111;
constexpr std::uint8_t LED_7SEG_DECIMAL_DOT = 0b10000000;

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

	/**
	 * Detect a pattern on LED display and return corresponding digit.
	 * @param detectSpaceAsZero if true, empty display is reported as zero.
	 * @return int detected digit (0-9) or -1 if the digit was not recognized
	 */
	static int detectDigit(state_t state, bool detectSpaceAsZero = false)
	{
		static std::map<state_t, int> lookupTable;
		if (lookupTable.empty()) {
			for (int i = 0; i < 10; ++i) {
				lookupTable[LED_7SEG_DIGITS_MAP[i]] = i;
			}
		}

		state = state | LED_7SEG_DECIMAL_DOT; // mask out the decimal dot
		if (detectSpaceAsZero && state == LED_7SEG_EMPTY_SPACE) {
			return 0; // special case, space is reported as 0
		}

		auto it = lookupTable.find(state);
		return it == lookupTable.end() ? -1 : it->second;
	}

	/**
	 * Detect a pattern on LED display and return the corresponding character (letter, digit, space).
	 * All letters are reported lowercased, empty character is space.
	 * @return char detected character or zero char if the state was not recognized
	 */
	static char detectCharacter(state_t state)
	{
		static std::map<state_t, char> lookupTable;
		if (lookupTable.empty()) {
			for (int i = 0; i < 10; ++i) {
				lookupTable[LED_7SEG_DIGITS_MAP[i]] = '0' + i;
			}
			for (int i = 0; i < 26; ++i) {
				lookupTable[LED_7SEG_LETTERS_MAP[i]] = 'a' + i;
			}
			lookupTable[LED_7SEG_EMPTY_SPACE] = ' ';
		}

		state = state | LED_7SEG_DECIMAL_DOT; // mask out the decimal dot
		auto it = lookupTable.find(state);
		return it == lookupTable.end() ? '\0' : it->second;
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

	/**
	 * Detect a pattern in the last (current) state and return corresponding digit.
	 * @param detectSpaceAsZero if true, empty display is reported as zero.
	 * @return int detected digit (0-9) or -1 if the digit was not recognized
	 */
	int detectLastStateDigit(bool detectSpaceAsZero = false) const
	{
		return detectDigit(mLastState, detectSpaceAsZero);
	}

	/**
	 * Detect a pattern in the last (current) state and return the corresponding character (letter, digit, space).
	 * All letters are reported lowercased, empty character is space.
	 * @return char detected character or zero char if the state was not recognized
	 */
	char detectLastStateCharacter() const
	{
		return detectCharacter(mLastState);
	}

	/**
	 * Detect a pattern in the demultiplexed state and return corresponding digit.
	 * @param detectSpaceAsZero if true, empty display is reported as zero.
	 * @return int detected digit (0-9) or -1 if the digit was not recognized
	 */
	int detectDemultiplexedDigit(bool detectSpaceAsZero = false) const
	{
		return detectDigit(mDemultiplexedState, detectSpaceAsZero);
	}

	/**
	 * Detect a pattern in the demultiplexed state and return the corresponding character (letter, digit, space).
	 * All letters are reported lowercased, empty character is space.
	 * @return char detected character or zero char if the state was not recognized
	 */
	char detectDemultiplexedCharacter() const
	{
		return detectCharacter(mDemultiplexedState);
	}

	/**
	 * State of the decimal dot in the last digit state.
	 */
	bool lastStateHasDecimalDot() const
	{
		return (mLastState & LED_7SEG_DECIMAL_DOT) == 0; // 0 = LED is lit
	}

	/**
	 * State of the decimal dot in the demultiplexed state.
	 */
	bool demultiplexedHasDecimalDot() const
	{
		return (mDemultiplexedState & LED_7SEG_DECIMAL_DOT) == 0; // 0 = LED is lit
	}
};


/**
 * Helper class that simulates a shift register of fixed size.
 */
class ShiftRegister
{
private:
	std::deque<bool> mRegister;

public:
	ShiftRegister(std::size_t size) : mRegister(size) {}

	/**
	 * Push another bit and shif the register.
	 * @return bit that was pushed out (carry)
	 */
	bool push(bool bit)
	{
		mRegister.push_front(bit);
		bool res = mRegister.back();
		mRegister.pop_back();
		return res;
	}

	/**
	 * Size of the register (number of bits)
	 */
	std::size_t size() const
	{
		return mRegister.size();
	}

	/**
	 * Regular accessor for arbirary bit. Bit 0 is the last one pushed in.
	 */
	bool operator[](std::size_t idx) const
	{
		return mRegister[idx];
	}

	/**
	 * Retrieve a sequence of bits as unsigned integral type.
	 * Size of the result type determines also alignment of the index.
	 * @param idx index in multiples of T
	 * @return value as T (filled with consecutive bits at index location)
	 */
	template<typename T>
	T get(std::size_t idx) const
	{
		std::size_t len = sizeof(T) * 8;
		T mask = (T)1 << (len - 1);
		idx *= len; // word index to index of first bit
		T res = 0;
		std::size_t endIdx = std::min(idx + len, mRegister.size());
		while (idx < endIdx) {
			res = res >> 1;
			res |= mRegister[idx] ? mask : 0;
			++idx;
		}
		return res;
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

	/**
	 * Decode a number being shown on the display (after last demultiplexing).
	 * @param result output argument where the actual number will be stored
	 * @param divisor output argument that stores position of decimal dot in powers of 10
	 *                (result / divisor is the actual value being displayed)
	 * @return true if a valid number is being presented
	 */
	bool getNumber(int &result, int &divisor)
	{
		int res = 0;				// result accumulator
		int div = 1;				// if no decimal dot is present, divisor is 1 by default
		int divCandidate = 1000;	// candidate divisor value (divided by 10 in every iteration)

		// find index of the first nonempty glyph
		std::size_t idx = 0;
		while (idx < mDigits.size() && mDigits[idx].getDemultiplexedState() == LED_7SEG_EMPTY_SPACE) {
			++idx;
			divCandidate /= 10;
		}
		if (idx >= mDigits.size()) {
			return false; // no digits available
		}

		// process remaining digits
		while (idx < mDigits.size()) {
			// update res accumulator
			res *= 10;
			int digit = mDigits[idx].detectDemultiplexedDigit();
			if (digit < 0) {
				return false; // current glyph is not numerical digit
			}
			res += digit;

			// check for decimal dots
			if (mDigits[idx].demultiplexedHasDecimalDot()) {
				if (divCandidate == 0) {
					return false; // second decimal dot was detected
				}
				div = divCandidate;
				divCandidate = 0;
			}

			++idx;
			divCandidate /= 10;
		}

		// success (let's write the results)
		result = res;
		divisor = div;
		return true;
	}
};


#endif
