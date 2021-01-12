#include <simulation_funshield.hpp>
#include <simulation.hpp>
#include <emulator.hpp>

#include <iostream>

// a declaration of a method from interface that will give us the emulator instance
ArduinoEmulator& get_arduino_emulator_instance();

void foo(const std::vector<int>& data)
{
    for (auto&& x : data) {
        std::cout << x << std::endl;
    }
}


int main(int argc, char *argv[])
{
    foo({ 1, 2, 3 });

    std::vector<int> data({ 5, 6, 7 });
    foo(data);
    return 0;

    /*
    // initialize simulation
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    arduino.registerPin(LED_BUILTIN, OUTPUT);

    try {
        arduino.runSetup();

        // simulate 30s of run
        logtime_t endTime = arduino.getCurrentTime() + 30000000; // 30s
        while (arduino.getCurrentTime() < endTime) {
            arduino.runSingleLoop();
        }

        // analyze output pin history
        auto& events = arduino.getPinEvents(LED_BUILTIN);

        if (events.empty()) {
            std::cout << "No LED changes recorded whatsoever." << std::endl;
            return 1;
        }

        auto lastState = events.front().value.value;
        auto lastTime = events.front().time;
        for (std::size_t i = 1; i < events.size(); ++i) {
            if (events[i].value.value == lastState) {
                continue; // this is not an actual change of state
            }

            auto dt = events[i].time - lastTime;
            if (dt < 990000 || dt > 1100000) {
                std::cout << "Subsequent changes of LED state are not within acceptable tolerance. Expected 1000ms delay, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
                return 1;
            }

            lastState = events[i].value.value;
            lastTime = events[i].time;
        }

        auto dt = arduino.getCurrentTime() - lastTime;
        if (dt > 1100000) {
            std::cout << "Last LED state change was too long ago. Expected 1000ms delay at most, but " << (dt / 1000) << "ms delay was recorded." << std::endl;
            return 1;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }

    return 0;
    */
}
