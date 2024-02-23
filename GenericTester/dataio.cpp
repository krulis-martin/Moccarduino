#include "dataio.hpp"

#include <limits>


/**
 * Load input text file (stream) with button events. Fill them into funshield emulator and record them in output time series.
 * @param sin input stream (the opened text file)
 * @param funshield the emulator being pre-loaded with button events
 * @param outputEvents a vector of time series (one for each button), if the vector is empty, no events are recorded
 * @return duration of the emulation as loaded from the input stream
 */
logtime_t loadInputData(std::istream& sin, FunshieldSimulationController& funshield, std::vector<std::shared_ptr<TimeSeries<bool>>>& outputEvents)
{
    std::string line;
    std::size_t lineCount = 0;
    logtime_t lastTime = 0;
    bool buttonStates[] = { false, false, false };

    while (std::getline(sin, line)) {
        ++lineCount;
        if (line.empty()) continue;

        // parse the line
        logtime_t time = 0;
        int button = -1;
        char action = '\0';
        std::stringstream(line) >> time >> button >> action;
        if (time < lastTime) {
            throw std::runtime_error("Timestamps are not ordered on line " + std::to_string(lineCount)
                + ". Timestamp " + std::to_string(time) + " is lower than the previous " + std::to_string(lastTime) + ".");
        }
        lastTime = time;

        if (button == -1 && action == '\0') {
            return lastTime; // the timestamp had no additional arguments, it must have been the last marker
        }

        if (button < 1 || button > 3 || (action != 'u' && action != 'd')) {
            throw std::runtime_error("Invalid operation (button #" + std::to_string(button) + " action " + action
                + ") found at line " + std::to_string(lineCount));
        }
        --button; // normalize to zero-based index

        bool newButtonState = action == 'd'; // down = true
        if (buttonStates[button] == newButtonState) {
            continue; // no change in state
        }
        buttonStates[button] = newButtonState;

        // enqueue the event into funshield emulator
        if (newButtonState) {
            funshield.buttonDown(button, time);
        }
        else {
            funshield.buttonUp(button, time);
        }

        // record it for the output events
        if (button < (int)outputEvents.size()) {
            outputEvents[button]->addEvent(time, newButtonState);
        }
    }

    return lastTime + 100000; // add 100ms after last button event
}

/**
 * Helper structure that holds series of events with current index to the first unprocessed event and its timestamp.
 */
struct SeriesWrapper
{
	std::size_t index;
	logtime_t time;
	std::shared_ptr<TimeSeriesBase<>> events;

	SeriesWrapper(std::shared_ptr<TimeSeriesBase<>> _events)
		: index(0), time(_events->empty() ? std::numeric_limits<logtime_t>::max() : _events->getEventTime(0)), events(_events)
	{}

	void advanceIndex()
	{
		++index;
		if (index < events->size()) {
			time = events->getEventTime(index);
		}
		else {
			time = std::numeric_limits<logtime_t>::max();
		}
	}

	bool isDone() const
	{
        return index >= events->size();
	}
};


bool allDone(const std::vector<SeriesWrapper>& series)
{
    for (auto& s : series) {
        if (!s.isDone()) return false;
    }
    return true;
}


logtime_t getMinTimestamp(const std::vector<SeriesWrapper>& series)
{
    logtime_t t = std::numeric_limits<logtime_t>::max();
    for (auto& s : series) {
        t = std::min(t, s.time);
    }
    return t;
}


void printEvents(std::ostream& sout, const std::map<std::string, std::shared_ptr<TimeSeriesBase<>>>& events, char delimiter)
{
    // print header and prepare series wrappers
	std::vector<SeriesWrapper> series;
    sout << "timestamp";
    for (auto const& it : events) {
        sout << delimiter << it.first;
        series.emplace_back(it.second);
    }
    sout << "\n";

    // process the time series one timestamp at a time
    while (!allDone(series)) {
        logtime_t ts = getMinTimestamp(series);
        sout << ts;
        for (auto& s : series) {
            sout << delimiter;
            if (!s.isDone() && s.time == ts) {
                sout << s.events->getEventAsString(s.index);
                s.advanceIndex();
            }
        }
        sout << "\n";
    }
}
