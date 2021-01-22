#include <led_display.hpp>
#include <time_series.hpp>

#include "../test.hpp"

#include <vector>
#include <cstdint>
#include <cmath>

class Led7SegInterpreterTest : public MoccarduinoTest
{
private:
	using leds_t = Led7SegInterpreter<4>;
	
	static leds_t create(const std::vector<std::uint8_t>& digits)
	{
		leds_t::state_t state(1);
		std::size_t offset = 0;
		for (std::uint8_t d : digits) {
			state.set(d, offset);
			offset += sizeof(d)*8;
		}
		return leds_t(state);
	}

	static leds_t createNum(int num, std::size_t decimalDot = ~(std::size_t)0)
	{
		bool negative = num < 0;
		num = std::abs(num);
		std::vector<std::uint8_t> data{ LED_7SEG_EMPTY_SPACE, LED_7SEG_EMPTY_SPACE, LED_7SEG_EMPTY_SPACE, LED_7SEG_EMPTY_SPACE };
		std::size_t idx = data.size();
		while (num != 0 && idx > 0) {
			--idx;
			data[idx] = LED_7SEG_DIGITS_MAP[num % 10];
			if (decimalDot == idx) {
				data[idx] &= LED_7SEG_DECIMAL_DOT;
			}
			num = num / 10;
		}

		if (idx > 0 && negative) {
			data[--idx] = LED_7SEG_DASH;
		}

		return create(data);
	}

	static leds_t createStr(const char *str)
	{
		std::vector<std::uint8_t> data;
		while (*str && data.size() < 4) {
			char ch = *str++;
			if (ch >= 'a' && ch <= 'z') {
				data.push_back(LED_7SEG_LETTERS_MAP[ch - 'a']);
			}
			else {
				data.push_back(LED_7SEG_EMPTY_SPACE);
			}
		}

		while (data.size() < 4) {
			data.push_back(LED_7SEG_EMPTY_SPACE);
		}
		return create(data);
	}

public:
	Led7SegInterpreterTest() : MoccarduinoTest("led_display/7seg-interpreter") {}

	virtual void run() const
	{
		auto d1 = createNum(123);
		ASSERT_EQ(d1.getNumber(), 123, "");
		ASSERT_EQ(d1.getText(), " iz3", "text interpretation of a number");
		for (std::size_t i = 0; i < 4; ++i) {
			ASSERT_EQ(d1.getDigit(i, true), (int)i, "");
			ASSERT_FALSE(d1.hasDecimalDot(i), "unexpected decimal dot");
		}
		ASSERT_EQ(d1.decimalDotPosition(), 3, "");
		ASSERT_FALSE(d1.decimalDotAmbiguous(), "decimal dot ambiguous");


		auto d2 = createNum(-123, 2);
		ASSERT_EQ(d2.getNumber(), -123, "");
		ASSERT_EQ(d2.getText(), "-iz3", "");
		ASSERT_EQ(d2.getCharacter(0), '-', "");
		ASSERT_EQ(d2.getDigit(1, true), 1, "");
		ASSERT_EQ(d2.getDigit(2, true), 2, "");
		ASSERT_EQ(d2.getDigit(3, true), 3, "");
		ASSERT_TRUE(d2.hasDecimalDot(2), "decimal dot not found");
		ASSERT_FALSE(d2.decimalDotAmbiguous(), "decimal dot ambiguous");
		ASSERT_EQ(d2.decimalDotPosition(), 2, "");

		auto d3 = create({ LED_7SEG_DECIMAL_DOT, LED_7SEG_DECIMAL_DOT, LED_7SEG_DECIMAL_DOT, LED_7SEG_DECIMAL_DOT });
		ASSERT_TRUE(d3.decimalDotAmbiguous(), "");

		auto d4 = createStr("hell");
		ASSERT_EQ(d4.getText(), "hell", "what the hell...?");
		ASSERT_EQ(d4.getNumber(), Led7SegInterpreter<4>::INVALID_NUMBER, "text cannot be parsed as number");
	}
};


Led7SegInterpreterTest _led7SegInterpreterTest;



class DemultiplexingTest : public MoccarduinoTest
{
public:
	using leds_t = BitArray<4>;

	DemultiplexingTest() : MoccarduinoTest("led_display/demultiplexing") {}

	virtual void run() const
	{
		FutureTimeSeries<leds_t> input;
		LedsEventsDemultiplexer<4> demuxer(20, 2);
		TimeSeries<leds_t> output;

		input.attachNextConsumer(demuxer);
		demuxer.attachNextConsumer(output);

		logtime_t ts = 1;

		std::vector<leds_t> leds = { leds_t(OFF), leds_t(OFF), leds_t(OFF), leds_t(OFF) };
		for (std::size_t i = 0; i < leds.size(); ++i) {
			leds[i].set(ON, i, 1);
		}

		while (ts < 1000) {
			input.addFutureEvent(ts++, leds[1]);
			input.addFutureEvent(ts++, leds[2]);
		}
		while (ts < 2000) {
			input.addFutureEvent(ts++, leds[0]);
			input.addFutureEvent(ts++, leds[3]);
		}

		input.advanceTime(ts);

		ASSERT_EQ(output.size(), 2, "two output events expected");
		ASSERT_LT(output[0].time, 22, "first event not in time");
		ASSERT_GT(output[1].time, 1000, "second event not in time");
		ASSERT_LT(output[1].time, 1022, "second event not in time");
		ASSERT_EQ(output[0].value.get<unsigned>(0), 0b1001, "first demuxed value is incorrect");
		ASSERT_EQ(output[1].value.get<unsigned>(0), 0b0110, "second demuxed value is incorrect");
	}
};

DemultiplexingTest _demultiplexingTest;
