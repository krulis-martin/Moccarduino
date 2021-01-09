/**
 * Simple test applied on the standard LED blink example from Arduino IDE codebase.
 */
#include <simulation.hpp>
#include <emulator.hpp>

#include <iostream>

// a declaration of a method from interface that will give us the emulator instance
ArduinoEmulator& get_arduino_emulator_instance();


int main(int argc, char* argv[])
{
    // initialize simulation
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    arduino.registerPin(LED_BUILTIN, OUTPUT);

    try {
        arduino.runSetup();

        // simulate 30s of run
        std::cout << "Simulate 30s of code run..." << std::endl;
        logtime_t endTime = arduino.getCurrentTime() + 30000000; // 30s
        while (arduino.getCurrentTime() < endTime) {
            arduino.runSingleLoop();
        }

        // analyze output pin history
        auto& events = arduino.getPinEventsRaw(LED_BUILTIN);

        if (events.size() < 29) {
            std::cerr << "Too few LED changes. At least 29 expected, but only " << events.size() << " recorded." << std::endl;
            return 1;
        }

        auto lastState = events.front().value;
        auto lastTime = events.front().time;
        for (std::size_t i = 1; i < events.size(); ++i) {
            if (events[i].value == lastState) {
                continue; // this is not an actual change of state
            }

            auto dt = events[i].time - lastTime;
            if (dt < 990000 || dt > 1100000) {
                std::cerr << "Subsequent changes of LED state are not within acceptable tolerance. Expected 1000ms delay, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
                return 1;
            }

            lastState = events[i].value;
            lastTime = events[i].time;
        }

        auto dt = arduino.getCurrentTime() - lastTime;
        if (dt > 1100000) {
            std::cerr << "Last LED state change was too long ago. Expected 1000ms delay at most, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
            return 1;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }

    std::cout << "Simulation completed successfully." << std::endl;
    return 0;
}
