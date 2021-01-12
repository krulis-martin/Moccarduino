#ifndef MOCCARDUINO_SHARED_TIME_SERIES_HPP
#define MOCCARDUINO_SHARED_TIME_SERIES_HPP

#include <vector>
#include <deque>
#include <algorithm>
#include <stdexcept>
#include <cstdint>

using logtime_t = std::uint64_t;

/**
 * This is de-facto a queue of time-marked events. It provides a similar interface like deque
 * (which is also used as internal storage) and additionaly some analytical functions that
 * might help with behavioral assertions.
 * @tparam VALUE the inner value of each event (e.g., a state of a pin)
 * @tparam TIME type used for logical time stamps
 */
template<typename VALUE, typename TIME = logtime_t> 
class TimeSeries
{
public:
	/**
	 * Internal structure that wraps all time series events.
	 */
	struct Event {
	public:
		TIME time;		///< when the event happen
		VALUE value;	///< associated value of the event (new state)

		Event(TIME t, VALUE v) : time(t), value(v) {}

		// make the sorting algorithm great again!
		inline bool operator<(const Event& e) const
		{
			return time < e.time || (time == e.time && value < e.value);
		}

		inline bool operator==(const Event& e) const
		{
			return time == e.time && value == e.value;
		}
	};

	/**
	 * Range of indices of time series events. Basically an interval [start, end).
	 */
	class Range {
	private:
		std::size_t mStart;	///< starting index (inclusive)
		std::size_t mEnd;	///< terminal index (exclusive)

	public:
		Range(std::size_t start = 0, std::size_t end = ~(std::size_t)0)
			: mStart(std::min(start, end)), mEnd(std::max(start,end))
		{}

		std::size_t start() const
		{
			return mStart;
		}

		std::size_t end() const
		{
			return mEnd;
		}

		std::size_t length() const
		{
			return mEnd - mStart;
		}

		bool empty() const
		{
			return length() == 0;
		}

		void set(std::size_t start, std::size_t end)
		{
			mStart = std::min(start, end);
			mEnd = std::max(start, end);
		}

		// make the sorting algorithm great again!
		inline bool operator<(const Range& r) const
		{
			return mStart < r.mStart || (mStart == r.mStart && mEnd < r.mEnd);
		}

		inline bool operator==(const Range& r) const
		{
			return mStart == r.mStart && mEnd == r.mEnd;
		}

		inline bool overlap(const Range& r) const
		{
			return mStart < r.mEnd && mEnd > r.mStart;
		}
	};

private:
	/**
	 * Internal data structure that actually holds the events.
	 * The events are sorted by their time in ascending order.
	 */
	std::deque<Event> mEvents;

public:
	
	// interface that simulates deque

	std::size_t size() const
	{
		return mEvents.size();
	}

	const Event& operator[](std::size_t idx) const
	{
		return mEvents[idx];
	}

	bool empty() const
	{
		return mEvents.empty();
	}

	const Event& front() const
	{
		if (empty()) {
			throw std::runtime_error("The time series is empty. Unable to reach first item.");
		}
		return mEvents.front();
	}

	const Event& back() const
	{
		if (empty()) {
			throw std::runtime_error("The time series is empty. Unable to reach last item.");
		}
		return mEvents.back();
	}

	void clear()
	{
		mEvents.clear();
	}

	/**
	 * Add another event to the time series. The event must respect the causality of this series.
	 * @param time when the event was recorded
	 * @param value of the event
	 */
	void addEvent(TIME time, VALUE value)
	{
		if (!mEvents.empty() && mEvents.back().time > time) {
			throw std::runtime_error("The added event violated causality!");
		}

		mEvents.emplace_back(time, value);
	}

	/**
	 * Inject an event out of order (shuffle it to the right positon).
	 * @param time when the event was recorded
	 * @param value of the event
	 */
	void insertRawEvent(TIME time, VALUE value)
	{
		std::size_t idx = mEvents.size();
		mEvents.emplace_back(time, value);
		while (idx > 0 && mEvents[idx] < mEvents[idx - 1]) {
			std::swap(mEvents[idx], mEvents[idx - 1]);
			--idx;
		}
	}

	/**
	 * Remove first event from the time series.
	 */
	void consumeEvent()
	{
		if (empty()) {
			throw std::runtime_error("Unable to consume an event of empty time series.");
		}
		mEvents.pop_front();
	}

	/**
	 * Remove first event from the time series.
	 * @param lastEvent output argument - the event being removed is saved here
	 */
	void consumeEvent(Event &lastEvent)
	{
		if (empty()) {
			throw std::runtime_error("Unable to consume an event of empty time series.");
		}
		lastEvent = mEvents.front();
		mEvents.pop_front();
	}

	/**
	 * Remove all events from the time series which are strictly before givent time.
	 * @param time that marks all older events for removal
	 */
	void consumeEventsUntil(TIME time)
	{
		while (!empty() && front().time < time) {
			mEvents.pop_front();
		}
	}

	/**
	 * Remove all events from the time series which are strictly before givent time.
	 * @param time that marks all older events for removal
	 * @param lastEvent output argument - the last event being removed before given time marker
	 */
	void consumeEventsUntil(TIME time, Event &lastEvent)
	{
		while (!empty() && front().time < time) {
			lastEvent = front();
			mEvents.pop_front();
		}
	}

	/**
	 * Take given list of time series and merge all their events into this time series.
	 * This time series is cleared before merge.
	 */
	void merge(const std::vector<TimeSeries<VALUE, TIME>>& series)
	{
		clear();
		for (auto&& s : series) {
			for (auto&& e : s.mEvents) {
				mEvents.push_back(e);
			}
		}
		std::sort(mEvents.begin(), mEvents.end());
	}


	/*
	 * Analytical functions
	 */

	/**
	 * Get the difference between first and last event in given range.
	 */
	TIME getRangeDuration(const Range& range) const
	{
		if (range.length() < 2) {
			return 0.0;
		}
		else {
			return mEvents[range.end() - 1] - mEvents[range.start()];
		}
	}

	/**
	 * Examine event time stamps in given range and return the mean delay between subsequent events.
	 */
	double getDeltasMean(const Range& range) const
	{
		if (range.length() < 2) {
			return 0.0;
		}

		TIME deltas = 0;
		TIME lastTime = mEvents[range.start()].time;

		for (std::size_t i = range.start() + 1; i < range.end(); ++i) {
			deltas += mEvents[i].time - lastTime;
			lastTime = mEvents[i].time;
		}

		return (double)deltas / (double)(range.length() - 1);
	}

	/**
	 * Examine event time stamps in given range and return the variance (std. dev ^2)
	 * of delays between subsequent events.
	 */
	double getDeltasVariance(const Range& range) const
	{
		if (range.length() < 2) {
			return 0.0;
		}

		TIME deltas = 0;
		TIME squareDeltas = 0;
		TIME lastTime = mEvents[range.start()].time;

		for (std::size_t i = range.start() + 1; i < range.end(); ++i) {
			auto dt = mEvents[i].time - lastTime;
			deltas += dt;
			squareDeltas += dt * dt;
			lastTime = mEvents[i].time;
		}

		double count = (double)(range.length() - 1);
		double mean = (double)deltas / count;
		return ((double)squareDeltas / count) - (mean * mean);	// E(X^2) - (EX)^2;
	}

	/**
	 * Tries to find the first occurence of a continuous sequence in the time series.
	 * If no such sequence exists, tries to return the longest prefix.
	 * @param sequence of event values to search for
	 * @return range of indiced where the occurence was found (empty range if nothing was found)
	 */
	Range findSubsequence(const std::vector<VALUE>& sequence) const
	{
		if (sequence.empty()) {
			throw std::runtime_error("Empty sequence given as needle for search.");
		}

		if (empty()) {
			return Range(0, 0);
		}

		// simple N x K comparison (may be replaced with Rabin-Karp or Knuth-Morris-Pratt in the future)
		Range bestFit(0, 0);
		for (std::size_t start = 0; start < size() - bestFit.length(); ++start) {
			std::size_t len = 0;
			while (len < sequence.size() && sequence[len] == mEvents[start + len].value) {
				++len;
			}
			if (len > bestFit.length()) {
				bestFit.set(start, start + len);
			}
		}

		return bestFit;
	}

	/**
	 * Tries to find the longest repetition of given sequence as a continuous subsequence.
	 * If no such sequence exists, tries to return the longest prefix.
	 * @param sequence of event values to search for
	 * @return range of indiced where the occurence was found (empty range if nothing was found)
	 */
	Range findRepetitiveSubsequence(const std::vector<VALUE>& sequence) const
	{
		if (sequence.empty()) {
			throw std::runtime_error("Empty sequence given as needle for search.");
		}

		if (sequence.size() > size()) {
			return Range(0, 0); // sequence is longer than the current time series (no possible match)
		}

		std::vector<bool> isStartingPoint(mEvents.size());	// true for each index where a the search sequence starts
		std::vector<std::size_t> startingPoints;			// list of all indices (ordered) where a starting point is

		// simple N x K comparison (may be replaced with Rabin-Karp or Knuth-Morris-Pratt in the future)
		for (std::size_t start = 0; start <= size() - sequence.size(); ++start) {
			std::size_t len = 0;
			while (len < sequence.size() && sequence[len] == mEvents[start + len].value) {
				++len;
			}

			bool entireSequenceMatched = (len == sequence.size());
			isStartingPoint[start] = entireSequenceMatched;
			if (entireSequenceMatched) {
				startingPoints.push_back(start);
			}
		}

		// assemble the longest repetitive sequence from collected starting points
		Range bestFit(0, 0);
		for (auto start : startingPoints) {
			std::size_t len = 0;
			while (start + len < size() && isStartingPoint[start + len]) {
				len += sequence.size();
			}
			if (len > bestFit.length()) {
				bestFit.set(start, start + len);
			}
		}

		return bestFit;
	}
};

#endif
