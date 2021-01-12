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
        constexpr logtime_t simulationTime = 100; // seconds
        std::cout << "Simulate " << simulationTime << " seconds of code run..." << std::endl;
        logtime_t endTime = arduino.getCurrentTime() + (simulationTime * 1000000); // us -> s
        while (arduino.getCurrentTime() < endTime) {
            arduino.runSingleLoop();
        }

        // analyze output pin history
        auto& events = arduino.getPinEvents(LED_BUILTIN);
        auto range = events.findRepetitiveSubsequence({ ArduinoPin::PinState(LED_BUILTIN, LOW), ArduinoPin::PinState(LED_BUILTIN, HIGH) }); // LED goes on and off again
        auto blinkCount = range.length() / 2;
        auto mean = events.getDeltasMean(range);
        auto variance = events.getDeltasVariance(range);
        std::cout << "LED blinked " << blinkCount << " times with avg. period " << (mean / 500000.0) << "s and variance " << variance << std::endl;

        if (blinkCount < 49 || blinkCount > 100) {
            std::cerr << "Number of blinks expected was 49 or 50." << std::endl;
            return 1;
        }

        if (mean < 990000 || mean > 1100000) {
            std::cerr << "Average period is off by more than 10% of expected value." << std::endl;
            return 1;
        }

        if (variance > 1) {
            std::cerr << "Variance is too high, the blinking is not regular enough." << std::endl;
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
