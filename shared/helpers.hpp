#ifndef MOCCARDUINO_SHARED_HELPERS_HPP
#define MOCCARDUINO_SHARED_HELPERS_HPP

#include <deque>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cstdint>


/**
 * Represents an array of bits of fixed size and offers some special functions.
 * Useful for LED bars, matrices, and 7-seg displays to represent intermediate state.
 */
template<int N>
class BitArray
{
private:
	template<int NN>
	friend std::ostream& operator<<(std::ostream& os, const BitArray<NN>& ba);

	/**
	 * Internal data are stored in fixed-sized array of bytes (the most compact way).
	 */
	std::array<std::uint8_t, (N + 7) / 8> mData;

	/**
	 * Internal writer.
	 */
	void setBit(bool value, std::size_t idx)
	{
		std::uint8_t mask = 1 << (idx % 8);
		mData[idx / 8] = value ? (mData[idx / 8] | mask) : (mData[idx / 8] & (~mask));
	}

	/**
	 * Internal reader.
	 */
	bool getBit(std::size_t idx) const
	{
		return (mData[idx / 8] >> (idx % 8)) & 0x01;
	}

public:
	/**
	 * Constructor also initializes all bits to given value (zero by default).
	 */
	BitArray(bool initialValue = false)
	{
		fill(initialValue);
	}

	/**
	 * Fill the entire bit array with given bit.
	 */
	void fill(bool value)
	{
		mData.fill(value ? 0xff : 0x00);
	}

	/**
	 * Return raw value of given stored bit (0 = LED is on).
	 */
	bool operator[](std::size_t idx) const
	{
		if (idx >= N) {
			throw std::runtime_error("Index ouf of range.");
		}

		return getBit(idx);
	}

	bool operator==(const BitArray<N>& ba) const
	{
		// lets make sure == operator compares only relevant bits
		for (std::size_t i = 0; i < N / 8; ++i) {
			if (mData[i] != ba.mData[i]) return false;
		}

		for (std::size_t i = (N/8) * 8; i < N; ++i) {
			if ((*this)[i] != ba[i]) return false;
		}
		return true;
	}

	bool operator!=(const BitArray<N>& ba) const
	{
		return !operator==(ba);
	}

	/**
	 * Retrieves a sequence of bits and save it into scalar value.
	 * If the size of the return type exceeds number of available bits, the result is cropped.
	 * @tparam T type of the return value (unsigned int of desired size)
	 * @param offset lowest bit of the array being returned
	 * @param count number of bits returned (full size of T by default)
	 * @return bits encoded in result type (bit at offset is the lowest bit)
	 */
	template<typename T>
	T get(std::size_t offset = 0, std::size_t count = sizeof(T) * 8) const
	{
		T res = 0;
		if (offset >= N) {
			return res;
		}

		count = std::min(count, sizeof(T) * 8);
		count = std::min(count, N - offset);
	
		while (count > 0) {
			--count;
			res = res << 1;
			res |= (*this)[offset + count] ? 1 : 0;
		}
		return res;
	}

	/**
	 * Set a sequence of bits from given value.
	 * If the size of the return type exceeds number of available bits, the result is cropped.
	 * @tparam T type of the input value (unsigned int of desired size)
	 * @param input value that will be filled in (lowest bit goes to offset)
	 * @param offset lowest bit of the array from which we fill the data
	 * @param count number of bits written (full size of T by default)
	 */
	template<typename T>
	void set(T input, std::size_t offset = 0, std::size_t count = sizeof(T)*8)
	{
		if (offset >= N) {
			return;
		}

		count = std::min(count, sizeof(T) * 8);
		std::size_t end = std::min(offset + count, (std::size_t)N);
		while (offset < end) {
			setBit(input & (T)1, offset);
			input = input >> 1;
			++offset;
		}
	}
};

template<int N>
std::ostream& operator<<(std::ostream& os, const BitArray<N>& ba)
{
	for (std::size_t i = 0; i < N; ++i) {
		os << (ba[i] ? 1 : 0);
	}
	return os;
}

/**
 * Shift register simulator of fixed size.
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


#endif