#include "time_series.hpp"

#include "../test.hpp"

#include <functional>
#include <cstdint>

class TimeSeriesFindSelectedSubseqTest : public MoccarduinoTest
{
private:
	template<typename T>
	void fillTs(const std::vector<T> values, FutureTimeSeries<T>& ts, logtime_t period = 100) const
	{
		logtime_t time = 0;
		for (auto val: values) {
			time += period;
			ts.addFutureEvent(time, val);
		}
	}

	void test(const std::vector<int> haystack, const std::vector<int> needle, const std::vector<std::size_t> expectedIndices) const
	{
		FutureTimeSeries<int> ts1, ts2;
		fillTs(haystack, ts1);
		fillTs(needle, ts2);
		std::vector<std::size_t> mapping;
		bool res = ts1.findSelectedSubsequence(ts2, mapping);
		ASSERT_EQ(res, needle.size() == expectedIndices.size(), "returned value");
		ASSERT_EQ(mapping.size(), expectedIndices.size(), "mapping size");
		if (mapping.size() == expectedIndices.size()) {
			for (std::size_t i = 0; i < mapping.size(); ++i) {
				ASSERT_EQ(mapping[i], expectedIndices[i], "expected index mapping");
			}
		}
	}

public:
	TimeSeriesFindSelectedSubseqTest() : MoccarduinoTest("time-series/findSelectedSubsequence") {}

	virtual void run() const
	{
		test({ 10, 20, 30 }, { 10, 20, 30 }, { 0, 1, 2 });
		test({ 10, 20, 30, 40, 50, 60, 70 }, { 20, 50, 60 }, { 1, 4, 5 });
		test({ 10, 20, 30 }, { 30, 40, 50 }, { 2 });
		test({ 10, 20, 30 }, { 40, 50, 60 }, {});
		test({ 10, 0, 10, 20, 20, 30, 31, 30, 40, 70, 40 }, { 10, 20, 30, 40 }, { 0, 3, 5, 8 });
	}
};


TimeSeriesFindSelectedSubseqTest   _timeSeriesFindSelectedSubseqTest;


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
