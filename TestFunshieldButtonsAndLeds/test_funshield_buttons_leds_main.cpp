#include "simulation_funshield.hpp"
#include "simulation.hpp"
#include "emulator.hpp"

#include <iostream>

// a declaration of a method from interface that will give us the emulator instance
ArduinoEmulator& get_arduino_emulator_instance();

using leds_state_t = FunshieldSimulationController::leds_display_t::state_t;

int main(int argc, char* argv[])
{
    // initialize simulation
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    FunshieldSimulationController funshield(arduino);

    // sequence of buttons (how they are pressed), one click per second
    std::vector<int> buttonEvents = { 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
    std::vector<leds_state_t> expectedStates;

    try {
        // prepare testing data
        logtime_t time = 0;
        int activeLed = 0;
        leds_state_t ledsState(OFF);
        ledsState.set(ON, activeLed, 1);
        expectedStates.push_back(ledsState);

        for (auto button : buttonEvents) {
            time += 1000000;
            funshield.buttonClick(button, 100000, time);
            activeLed = (activeLed + (button == 0 ? 1 : 3)) % 4;
            ledsState.fill(OFF);
            ledsState.set(ON, activeLed, 1);
            expectedStates.push_back(ledsState);
        }
        time += 1000000;

        // assemble the components
        LedsEventsDemultiplexer<4> demuxer(10000);
        LedsEventsAggregator<4> aggregator(50000);
        TimeSeries<leds_state_t> events;
        funshield.getLeds().attachSproutConsumer(demuxer);
        demuxer.attachNextConsumer(aggregator);
        aggregator.attachNextConsumer(events);

        arduino.runSetup();

        // simulate the run until all buttons are pressed
        std::cout << "Running the simulation (" << time / 1000000 << "s) ..." << std::endl;
        arduino.runLoopsForPeriod(time);

        // analyze output pin history
        if (events.empty()) {
            std::cerr << "No LED changes recorded whatsoever." << std::endl;
            return 1;
        }

        if (events.size() != expectedStates.size()) {
            std::cerr << "Total " << expectedStates.size() << " state changes expected, but " << events.size() << " events reported." << std::endl;
            return 1;
        }

        for (std::size_t i = 0; i < events.size(); ++i) {
            // std::cout << events[i].time / 1000 << " " << events[i].value << std::endl;
        }

        auto mean = events.getDeltasMean();
        auto deviation = events.getDeltasDeviation();
        bool ok = true;

        if (mean < 990000 || mean > 1010000) {
            std::cerr << "Average period is off by more than 10% of expected value." << std::endl;
            ok = false;
        }

        if (deviation > 10000) {
            std::cerr << "Deviation is too high, the LEDs do not change fast enough when the buttons are pressed." << std::endl;
            ok = false;
        }

        for (std::size_t i = 0; i < events.size(); ++i) {
            if (events[i].value != expectedStates[i]) {
                std::cerr << "Event #" << i << ": state " << events[i].value << " reported, but " << expectedStates[i] << " expected" << std::endl;
                ok = false;
            }
        }

        if (!ok) {
            std::cerr << "Test failed!" << std::endl;
            return 1;
        }

        std::cout << "Simulation ended successfully." << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}
