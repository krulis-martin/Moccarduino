#ifndef MOCCARDUINO_SHARED_TIME_SERIES_HPP
#define MOCCARDUINO_SHARED_TIME_SERIES_HPP

#include <deque>
#include <algorithm>
#include <initializer_list>
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
		inline bool operator <(const Event& e)
		{
			return time < e.time || (time == e.time && value < e.value);
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
	void merge(std::initializer_list<TimeSeries<VALUE, TIME>> series)
	{
		clear();
		for (auto&& s : series) {
			for (auto&& e : s.mEvents) {
				mEvents.push_back(e);
			}
		}
		std::sort(mEvents.begin(), mEvents.end());
	}
};

#endif
