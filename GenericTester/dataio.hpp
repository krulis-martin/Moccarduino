#ifndef MOCCARDUINO_GENERIC_TESTER_OUTPUT_HPP
#define MOCCARDUINO_GENERIC_TESTER_OUTPUT_HPP

#include "time_series.hpp"
#include "simulation_funshield.hpp"

#include <iostream>
#include <map>
#include <string>
#include <memory>

/**
 * Load input text file (stream) with button events. Fill them into funshield emulator and record them in output time series.
 * @param sin input stream (the opened text file)
 * @param funshield the emulator being pre-loaded with button events
 * @param outputEvents a vector of time series (one for each button), if the vector is empty, no events are recorded
 * @return duration of the emulation as loaded from the input stream
 */
logtime_t loadInputData(std::istream& sin, FunshieldSimulationController& funshield, std::vector<std::shared_ptr<TimeSeries<bool>>>& outputEvents);

/**
 * Print out formatted CSV composed of multiple time series (collecting events).
 * First col of the CSV is always the `timestamp`
 * @param sout where the CSV is outputted
 * @param events map of time series, key in the map denotes name of the column
 * @param delimiter used for CSV separation
 */
void printEvents(std::ostream& sout, const std::map<std::string, std::shared_ptr<TimeSeriesBase<>>>& events, char delimiter = ',');


#endif
