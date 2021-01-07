#include <simulation_funshield.hpp>
#include <simulation.hpp>
#include <emulator.hpp>

#include <iostream>


ArduinoEmulator& get_arduino_emulator_instance();


int main(int argc, char *argv[])
{
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    FunshieldSimulationController funshield(arduino);


    std::cout << "Hello World!\n";
    return 0;
}
