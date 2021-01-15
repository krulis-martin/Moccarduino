#include <helpers.hpp>

#include "../test.hpp"

#include <cstdint>

class BitArrayTest : public MoccarduinoTest
{
public:
	BitArrayTest() : MoccarduinoTest("helpers/bit-array") {}

	virtual void run() const
	{
		BitArray<30> ba;
		ASSERT_EQ(ba.get<std::uint32_t>(0), 0, "bit array is not empty");

		std::uint32_t magic = 0xdeadbeef & 0x3fffffff;
		ba.set<std::uint32_t>(magic);

		ASSERT_EQ(ba.get<std::uint32_t>(), magic, "bit array does not hold, what we previously set");
		ASSERT_EQ((int)ba.get<std::uint8_t>(0), 0xef, "bit array does not hold, what we previously set");
		ASSERT_EQ((int)ba.get<std::uint8_t>(8), 0xbe, "bit array does not hold, what we previously set");
		ASSERT_EQ((int)ba.get<std::uint8_t>(16), 0xad, "bit array does not hold, what we previously set");
		ASSERT_EQ((int)ba.get<std::uint8_t>(24), 0xde & 0x3f, "bit array does not hold, what we previously set");
	}
};

BitArrayTest _bitArrayTest;


class ShiftRegisterTest : public MoccarduinoTest
{
public:
	ShiftRegisterTest() : MoccarduinoTest("helpers/shift-register") {}

	virtual void run() const
	{
		ShiftRegister reg(32);
		ASSERT_EQ(reg.get<std::uint32_t>(0), 0, "new register is not empty");
		ASSERT_EQ(reg.size(), 32, "register size is not what we set");

		std::uint32_t magic = 0xdeadbeef, m = magic;
		for (std::size_t i = 0; i < 32; ++i) {
			reg.push((m & 0x80000000) > 0);
			m = m << 1;
		}
		ASSERT_EQ(m, 0, "");
		ASSERT_EQ(reg.get<std::uint32_t>(0), magic, "register does not hold, what we shifted in");
		ASSERT_EQ((int)reg.get<std::uint8_t>(0), 0xef, "register does not hold, what we shifted in");
		ASSERT_EQ((int)reg.get<std::uint8_t>(1), 0xbe, "register does not hold, what we shifted in");
		ASSERT_EQ((int)reg.get<std::uint8_t>(2), 0xad, "register does not hold, what we shifted in");
		ASSERT_EQ((int)reg.get<std::uint8_t>(3), 0xde, "register does not hold, what we shifted in");
	}
};

ShiftRegisterTest _shiftRegisterTest;
