#ifndef MOCCARDUINO_SHARED_TIME_SERIES_HPP
#define MOCCARDUINO_SHARED_TIME_SERIES_HPP

#include <vector>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <limits>
#include <functional>
#include <string>
#include <sstream>
#include <type_traits>
#include <cstdint>
#include <cmath>

using logtime_t = std::uint64_t;


/**
 * Base class for all event consumers. Event consumer is an interface that fills events
 * into time series (in the simplest case) or handles event transformtions (e.g., in demultiplexing).
 * The consumers also implement chaining, so after processing, the event may be passed to another consumer.
 * Thanks to this chaining, some consumers may act as transformers or even sole producers.
 */
template<typename VALUE, typename TIME = logtime_t>
class EventConsumer
{
private:
	/**
	 * Reference to the next consumer in the chain. We are using shared pointers,
	 * since some consumers may merely exist only in the chain (and need to be deleted with the chain)
	 * whilst others may be shared (e.g., read by testing scenario).
	 */
	EventConsumer<VALUE, TIME> *mNextConsumer;

protected:
	/**
	 * Timestamp of last event (time notification) registered by this consumer.
	 */
	TIME mLastTime;

	/**
	 * Pass the new event to the next consumer in the chain.
	 */
	void nextAddEvent(TIME time, VALUE value)
	{
		if (mNextConsumer != nullptr) {
			mNextConsumer->addEvent(time, value);
		}
	}

	/**
	 * Notify the next consumer that tempus fugit!
	 */
	void nextAdvanceTime(TIME time)
	{
		if (mNextConsumer != nullptr) {
			mNextConsumer->advanceTime(time);
		}
	}

	/**
	 * Pass the clear notification to the next consumer in the chain.
	 */
	void nextClear()
	{
		if (mNextConsumer != nullptr) {
			mNextConsumer->clear();
		}
	}

	virtual void doAddEvent(TIME time, VALUE value)
	{
		// base class have no implementation, just a transparent throughput
		nextAddEvent(time, value);
	}

	virtual void doAdvanceTime(TIME time)
	{
		// base class have no implementation, just a transparent throughput
		nextAdvanceTime(time);
	}

	virtual void doClear()
	{
		// base class have no implementation, just a transparent throughput
		nextClear();
	}

public:
	EventConsumer() : mNextConsumer(nullptr), mLastTime(0) {}
	virtual ~EventConsumer() = default;  // just to enforce virtual destructor

	/**
	 * Get the next event consumer in the chain.
	 */
	EventConsumer<VALUE, TIME>* nextConsumer()
	{
		return mNextConsumer;
	}

	/**
	 * Get the next event consumer in the chain.
	 */
	const EventConsumer<VALUE, TIME>* nextConsumer() const
	{
		return mNextConsumer;
	}

	/**
	 * Get the last consumer in the chain. If this is the last one, returns *this*.
	 */
	EventConsumer<VALUE, TIME>* lastConsumer()
	{
		auto last = this;
		while (last->nextConsumer() != nullptr) {
			last = last->nextConsumer();
		}
		return last;
	}

	/**
	 * Get the last consumer in the chain. If this is the last one, returns *this*.
	 */
	const EventConsumer<VALUE, TIME>* lastConsumer() const
	{
		auto last = this;
		while (last->nextConsumer() != nullptr) {
			last = last->nextConsumer();
		}
		return last;
	}

	/**
	 * Attach next event consumer right after this one.
	 */
	void attachNextConsumer(EventConsumer<VALUE, TIME> &consumer)
	{
		if (mNextConsumer != nullptr) {
			throw std::runtime_error("Next consumer is already attached.");
		}
		mNextConsumer = &consumer;
	}

	/**
	 * Detach the next event consumer.
	 */
	void detachNextConsumer()
	{
		if (mNextConsumer == nullptr) {
			throw std::runtime_error("No next consumer is attached.");
		}
		mNextConsumer = nullptr;
	}

	// Events

	/**
	 * Consume another event (e.g., insert it into time series). The event must respect causality.
	 * @param time when the event was recorded
	 * @param value of the event
	 */
	void addEvent(TIME time, VALUE value)
	{
		if (time < mLastTime) {
			throw std::runtime_error("Unable to add event that violates causality.");
		}
		doAddEvent(time, value);
		mLastTime = time;
	}

	/**
	 * Notifies the event consumer that the time has advanced. This might be useful in case one of the consumers
	 * in chain is actually delaying or emitting events so the whole pipeline will not get stuck.
	 */
	void advanceTime(TIME time)
	{
		if (time < mLastTime) {
			throw std::runtime_error("Unable to advance time to past, since it violates causality.");
		}
		doAdvanceTime(time);
		mLastTime = time;
	}

	/**
	 * Clear all recorded events (start all over again).
	 * Logical time is not reset.
	 */
	void clear()
	{
		doClear();
	}
};


/**
 * Extended event consumer which also may produce events of different type.
 * It has a sprout that may have another event consumer (of differnt type) attached.
 * This is base class for all event transformers.
 * @param VALUE type of values being consumed
 * @param PROD_VALUE type of values being produced
 */
template<typename VALUE, typename PROD_VALUE, typename TIME = logtime_t>
class ForkedEventConsumer : public EventConsumer<VALUE, TIME>
{
private:
	/**
	 * Consumer of newly emitted/transformed events (beginning of another chain).
	 */
	EventConsumer<PROD_VALUE, TIME>* mSproutConsumer;

protected:
	void doAddEvent(TIME time, VALUE value) override
	{
		EventConsumer<VALUE, TIME>::doAddEvent(time, value);

		// Emitting events on the sprout must be defined in derived classes...
	}

	void doAdvanceTime(TIME time) override
	{
		EventConsumer<VALUE, TIME>::doAdvanceTime(time);
		if (mSproutConsumer != nullptr) {
			mSproutConsumer->advanceTime(time);
		}
	}

	void doClear() override
	{
		EventConsumer<VALUE, TIME>::doClear();
		if (mSproutConsumer != nullptr) {
			mSproutConsumer->clear();
		}
	}

public:
	ForkedEventConsumer() : mSproutConsumer(nullptr) {}

	/**
	 * Get prout consumer which receives newly emitted events.
	 */
	EventConsumer<PROD_VALUE, TIME>* sproutConsumer()
	{
		return mSproutConsumer;
	}

	/**
	 * Get prout consumer which receives newly emitted events.
	 */
	const EventConsumer<PROD_VALUE, TIME>* sproutConsumer() const
	{
		return mSproutConsumer;
	}

	/**
	 * Attach sprout consumer which receives newly emitted events.
	 */
	void attachSproutConsumer(EventConsumer<PROD_VALUE, TIME>& consumer)
	{
		if (mSproutConsumer != nullptr) {
			throw std::runtime_error("Sprout consumer is already attached.");
		}
		mSproutConsumer = &consumer;
	}

	/**
	 * Detach the sprout event consumer.
	 */
	void detachSproutConsumer()
	{
		if (mSproutConsumer == nullptr) {
			throw std::runtime_error("No sprout consumer is attached.");
		}
		mSproutConsumer = nullptr;
	}
};


/**
 * A quick wrapper that allows functions (and lambdas) to get access to passing events.
 */
template<typename VALUE, typename TIME = logtime_t>
class EventAnalyzer : public EventConsumer<VALUE, TIME>
{
private:
	VALUE mLastValue;
	std::function<void(TIME, VALUE)> mEventCallback;
	std::function<void()> mClearCallback;


protected:
	void doAddEvent(TIME time, VALUE value) override
	{
		EventConsumer<VALUE, TIME>::doAddEvent(time, value);
		mLastValue = value;
		mEventCallback(time, value);
	}

	void doAdvanceTime(TIME time) override
	{
		EventConsumer<VALUE, TIME>::doAdvanceTime(time);
		mEventCallback(time, mLastValue);

	}

	void doClear() override
	{
		EventConsumer<VALUE, TIME>::doClear();
		mClearCallback();
	}

public:
	/**
	 * Constructor gets one or two functions - event processor (mandatory) and clear callback (optional).
	 * The event processor is called with every new event and with advance time (using last event value).
	 */
	EventAnalyzer(std::function<void(TIME, VALUE)> eventCallback, std::function<void()> clearCallback = []() {})
		: mLastValue(), mEventCallback(eventCallback), mClearCallback(clearCallback) {}
};

template<typename TIME = logtime_t>
class TimeSeriesBase
{
public:
	/**
	 * Range of indices of time series events. Basically an interval [start, end).
	 */
	class Range {
	private:
		std::size_t mStart;	///< starting index (inclusive)
		std::size_t mEnd;	///< terminal index (exclusive)

	public:
		Range(std::size_t start = 0, std::size_t end = ~(std::size_t)0)
			: mStart(std::min(start, end)), mEnd(std::max(start, end))
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

	virtual std::size_t size() const = 0;
	virtual bool empty() const = 0;
	virtual TIME getEventTime(std::size_t idx) const = 0;
	virtual std::string getEventAsString(std::size_t idx) const = 0;

protected:
	/**
	 * Wrap a string in double quotes and properly encode all double quotes inside.
	 */
	static std::string encode_doublequotes(const std::string& str)
	{
		std::stringstream result;
		result << '"';
		std::for_each(str.begin(), str.end(), [&](const char c) {
			if (c == '"') result << '"'; // prefix double quote with another (RFC 4180)
			result << c;
		});
		result << '"';
		return result.str();
	}

	template<typename T>
	static std::string convert(const T& value) {
		if constexpr (std::is_same_v<T, std::string>) {
			return encode_doublequotes(value);
		}
		if constexpr (std::is_same_v<T, bool>) {
			return value ? "1" : "0";
		}
		if constexpr (std::is_arithmetic_v<T>) {
			return std::to_string(value);
		}
		else {
			return std::string(value);
		}
	}
};

/**
 * A container of time-marked events. It provides a similar interface like vector
 * (which is also used as internal storage) and additionaly some analytical functions that
 * might help with behavioral assertions.
 * @tparam VALUE the inner value of each event (e.g., a state of a pin)
 * @tparam TIME type used for logical time stamps
 */
template<typename VALUE, typename TIME = logtime_t> 
class TimeSeries : public TimeSeriesBase<TIME>, public EventConsumer<VALUE, TIME>
{
public:
	using Range = typename TimeSeriesBase<TIME>::Range;

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

protected:
	/**
	 * Internal data structure that actually holds the events.
	 * The events are sorted by their time in ascending order.
	 * Events with the same time may be in any order.
	 */
	std::vector<Event> mEvents;

	void doAddEvent(TIME time, VALUE value) override
	{
		if (!this->mEvents.empty() && this->mEvents.back().time > time) {
			throw std::runtime_error("Unable to add event that violates causality.");
		}

		mEvents.emplace_back(time, value);
		EventConsumer<VALUE, TIME>::doAddEvent(time, value);
	}

	void doClear() override
	{
		mEvents.clear();
		EventConsumer<VALUE, TIME>::doClear();
	}

public:
	// interface that simulates deque

	std::size_t size() const override
	{
		return mEvents.size();
	}

	bool empty() const override
	{
		return mEvents.empty();
	}

	TIME getEventTime(std::size_t idx) const override
	{
		return mEvents[idx].time;
	}

	std::string getEventAsString(std::size_t idx) const override
	{
		return TimeSeriesBase<TIME>::convert(mEvents[idx].value);
	}

	const Event& operator[](std::size_t idx) const
	{
		return mEvents[idx];
	}

	const Event& at(std::size_t idx) const
	{
		return mEvents[idx];
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
	 * Examine event time stamps of the entire series and return the mean delay between subsequent events.
	 */
	double getDeltasMean() const
	{
		return getDeltasMean(Range(0, mEvents.size()));
	}

	/**
	 * Examine event time stamps in given range and return the std. deviation
	 * of delays between subsequent events.
	 */
	double getDeltasDeviation(const Range& range) const
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
		return std::sqrt(((double)squareDeltas / count) - (mean * mean));	// E(X^2) - (EX)^2;
	}

	/**
	 * Examine event time stamps of the entire series and return the std. deviation
	 * of delays between subsequent events.
	 */
	double getDeltasDeviation() const
	{
		return getDeltasDeviation(Range(0, mEvents.size()));
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


	/**
	 * Tries to find the first occurence of selected subsequence usign eager algorithm
	 * (i.e., some of the items in this sequence may be skipped).
	 * TODO: this method should accept also vector or VALUE (let's fix it in the future)
	 * @param sequence of event values to search for (a time series, but the times are ignored)
	 * @param mapping output vector, for each event in sequence, it will hold an index refering to *this
	 *                partial mapping is stored here even if the whole sequence could not have been matched
	 * @return true if the whole sequence was matched
	 */
	bool findSelectedSubsequence(const TimeSeries<VALUE, TIME>& sequence, std::vector<std::size_t> &mapping) const
	{
		if (sequence.empty()) {
			throw std::runtime_error("Empty sequence given as needle for search.");
		}

		mapping.clear();
		if (empty()) {
			return false;
		}

		std::size_t idx = 0;
		for (std::size_t si = 0; si < sequence.size(); ++si) {
			while (idx < size() && sequence[si].value != mEvents[idx].value) {
				++idx;
			}

			if (idx < size()) {
				mapping.push_back(idx);
			}
			else break;
		}
	
		return mapping.size() == sequence.size();
	}


	/**
	 * Compare this time series with another one of the same type. Return total amount of discrete time
	 * (from given range) the values from events differ.
	 * @param timeSeries another time series object with which this is compared
	 * @param range time range of interest
	 * @param initialValue the value expected (for both series) before the first event
	 */
	TIME compare(const TimeSeries<VALUE, TIME> &timeSeries, const Range &range, const VALUE& initialValue) const
	{
		TIME res = 0;
		
		const TimeSeries<VALUE, TIME>* ts[]{ this, &timeSeries };
		VALUE lastValue[]{ initialValue, initialValue };
		std::size_t idx[]{ 0, 0 };

		// skip parts of time series which are before the range (update starting values)
		for (std::size_t t = 0; t < 2; ++t) {
			while (idx[t] < ts[t]->size() && ts[t]->at(idx[t]).time <= range.start()) {
				lastValue[t] = ts[t]->at(idx[t]).value;
				++idx[t];
			}
		}

		TIME lastTime = range.start();
		while (lastTime < range.end() && (idx[0] < ts[0]->size() || idx[1] < ts[1]->size())) {
			// index of the series which has next event sooner
			logtime_t nextTs[2];
			for (std::size_t t = 0; t < 2; ++t) {
				nextTs[t] = idx[t] < ts[t]->size() ? ts[t]->at(idx[t]).time : std::numeric_limits<logtime_t>::max();
			}
			std::size_t next = nextTs[0] <= nextTs[1] ? 0 : 1;

			if (lastValue[0] != lastValue[1]) {
				// during the time period from last time to now the values were different -> add this period to accumulator
				res += std::min(nextTs[next], range.end()) - lastTime;
			}

			// update the local states
			lastTime = nextTs[next];
			lastValue[next] = ts[next]->at(idx[next]).value;
			++idx[next];
		}

		// if the values differ at the end, tak it also into account
		if (lastTime < range.end() && lastValue[0] != lastValue[1]) {
			res += range.end() - lastTime;
		}

		return res;
	}
};


/**
 * Extension of time series so it can hold "future" events. Future events are registered but not emitted
 * to the next item in the chain until such action is triggered by time advancing or consumig a regular event.
 */
template<typename VALUE, typename TIME = logtime_t>
class FutureTimeSeries : public TimeSeries<VALUE, TIME>
{
private:
	/**
	 * Index refering just after the last item that was already consumed (emitted to the next consumer in chain).
	 */
	std::size_t mLastConsumed;

	/**
	 * Emit events which has not yet been emited up to given timestamp (inclusive).
	 */
	void consumeEventsUntil(TIME time)
	{
		while (mLastConsumed < this->mEvents.size() && this->mEvents[mLastConsumed].time <= time) {
			this->nextAddEvent(this->mEvents[mLastConsumed].time, this->mEvents[mLastConsumed].value);
			++mLastConsumed;
		}
	}

protected:
	void doAddEvent(TIME time, VALUE value) override
	{
		consumeEventsUntil(time);
		addFutureEvent(time, value); // this will actually take care of placing the event at the right position
		EventConsumer<VALUE, TIME>::doAddEvent(time, value);	// yes, we skip the TimeSeries implementation
	}


	void doAdvanceTime(TIME time) override
	{
		consumeEventsUntil(time);
		TimeSeries<VALUE, TIME>::doAdvanceTime(time);
	}

	void doClear() override
	{
		mLastConsumed = 0;
		TimeSeries<VALUE, TIME>::doClear();
	}

public:
	FutureTimeSeries() : mLastConsumed(0) {}

	/**
	 * Add event that is considered to be in the future. It is not passed along through the event consumer chain,
	 * until the time is advanced enough using advanceTime method. Future events may actually be inserted in random order.
	 * The only condition is that future event is not older than last real event.
	 */
	void addFutureEvent(TIME time, VALUE value)
	{
		if (this->mLastTime > time) {
			throw std::runtime_error("Unable to add event that violates causality.");
		}

		// shuffle the event to the right place from the back
		std::size_t idx = this->mEvents.size();
		this->mEvents.emplace_back(time, value);
		while (idx > 0 && this->mEvents[idx-1].time > this->mEvents[idx].time) {
			std::swap(this->mEvents[idx-1], this->mEvents[idx]);
			--idx;
		}

		if (idx < mLastConsumed) {
			throw std::runtime_error("Invariant breached! Index of last consumed event and last timestamp are not in sync.");
		}
	}

	/**
	 * Add constant timing skew to all event times.
	 */
	void addTimingSkew(TIME skew)
	{
		for (auto&& e : this->mEvents) {
			e.time += skew;
		}
	}
};


#endif
