#include "time_series.hpp"

#include "../test.hpp"

#include <functional>
#include <cstdint>

class TimeSeriesCompareTest : public MoccarduinoTest
{
private:
	void fillTs(const std::vector<logtime_t> times, FutureTimeSeries<int>& ts) const
	{
		int counter = 0;
		for (auto time : times) {
			++counter;
			ts.addFutureEvent(time, counter);
		}
	}

	logtime_t testCmp(const std::vector<logtime_t> times1, const std::vector<logtime_t> times2, logtime_t end, logtime_t start = 0) const
	{
		FutureTimeSeries<int> ts1, ts2;
		fillTs(times1, ts1);
		fillTs(times2, ts2);
		logtime_t res = ts1.compare(ts2, FutureTimeSeries<int>::Range(start, end), 0);
		logtime_t res2 = ts2.compare(ts1, FutureTimeSeries<int>::Range(start, end), 0);
		ASSERT_EQ(res, res2, "compare() should be symmetric");
		return res;
	}

public:
	TimeSeriesCompareTest() : MoccarduinoTest("time-series/compare") {}

	virtual void run() const
	{
		ASSERT_EQ(testCmp({ 100, 300, 500, 800 }, { 100, 300, 500, 800 }, 1000), 0, "identical series");
		ASSERT_EQ(testCmp({ 100, 300, 501, 800 }, { 100, 300, 500, 800 }, 1000), 1, "one ts off by 1");
		ASSERT_EQ(testCmp({ 100, 300, 501, 800 }, { 100, 300, 500, 800 }, 1000), 1, "one ts off by 1");
		ASSERT_EQ(testCmp({ 100, 300, 500, 800 }, { 150, 350, 550, 850 }, 1000), 200, "steady delayed 4x50");
		ASSERT_EQ(testCmp({ 100, 300, 500, 800 }, { 50, 250, 450, 750 }, 1000), 200, "steady early 4x50");
		ASSERT_EQ(testCmp({ 100, 150, 200, 850, 900 }, { 300, 400, 500, 800, 850 }, 1000), 500, "both early and delaying");
		ASSERT_EQ(testCmp({ 100, 200, 300, 400, 500, 600 }, { 110, 210, 310, 410, 510, 610 }, 605, 205), 40, "subrange");
		ASSERT_EQ(testCmp({ 0, 30, 50, 80, 90 }, { 100, 300, 500, 800 }, 1000), 1000, "completely off series");
	}
};


TimeSeriesCompareTest  _timeSeriesCompareTest;
