#include "simulation_funshield.hpp"
#include "simulation.hpp"
#include "emulator.hpp"

#include <iostream>

// a declaration of a method from interface that will give us the emulator instance
ArduinoEmulator& get_arduino_emulator_instance();

using leds_state_t = FunshieldSimulationController::leds_display_t::state_t;

int main(int argc, char *argv[])
{
    // initialize simulation
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    FunshieldSimulationController funshield(arduino);

    std::vector<int> buttonEvents = { 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
    std::vector<leds_state_t> expectedStates;

    try {
        // prepare testing data
        logtime_t time = 0;
        int activeLed = 0;
        for (auto button : buttonEvents) {
            time += 1000000;
            funshield.buttonClick(button, 100000, time);
            activeLed = (activeLed + (button == 0 ? 1 : 3)) % 4;
        }
        time += 1000000;

        LedsEventsDemultiplexer<4> demuxer(10000);
        LedsEventsAggregator<4> aggregator(50000);
        TimeSeries<leds_state_t> events;
        funshield.getLeds().attachSproutConsumer(demuxer);
        demuxer.attachNextConsumer(aggregator);
        aggregator.attachNextConsumer(events);

        arduino.runSetup();

        // simulate 30s of run
        arduino.runLoopsForPeriod(time);

        // analyze output pin history
        if (events.empty()) {
            std::cout << "No LED changes recorded whatsoever." << std::endl;
            return 1;
        }

        for (std::size_t i = 0; i < events.size(); ++i) {
            std::cout << events[i].time / 1000 << " " << events[i].value << std::endl;
        }

        auto mean = events.getDeltasMean();
        auto deviation = events.getDeltasDeviation();
        if (mean < 990000 || mean > 1010000) {
            std::cerr << "Average period is off by more than 10% of expected value." << std::endl;
            return 1;
        }

        std::cout << mean << " " << deviation << std::endl;
        if (deviation > 10000) {
            std::cerr << "Deviation is too high, the LEDs do not change fast enough when the buttons are pressed." << std::endl;
            return 1;
        }


        //auto lastState = events.front().value.value;
        //auto lastTime = events.front().time;
        //for (std::size_t i = 1; i < events.size(); ++i) {
        //    if (events[i].value.value == lastState) {
        //        continue; // this is not an actual change of state
        //    }

        //    auto dt = events[i].time - lastTime;
        //    if (dt < 990000 || dt > 1100000) {
        //        std::cout << "Subsequent changes of LED state are not within acceptable tolerance. Expected 1000ms delay, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
        //        return 1;
        //    }

        //    lastState = events[i].value.value;
        //    lastTime = events[i].time;
        //}

        //auto dt = arduino.getCurrentTime() - lastTime;
        //if (dt > 1100000) {
        //    std::cout << "Last LED state change was too long ago. Expected 1000ms delay at most, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
        //    return 1;
        //}
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}
