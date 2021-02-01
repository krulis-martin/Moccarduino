![Windows MSVC Build](https://github.com/krulis-martin/Moccarduino/workflows/Windows%20MSVC%20Build/badge.svg)
![Linux GCC Build](https://github.com/krulis-martin/Moccarduino/workflows/Linux%20GCC%20Build/badge.svg)

# Moccarduino

Mocking environment for testing Arduino code. The sole objective of this project is to provide testing platform that could be used in
[ReCodEx](https://github.com/ReCodEx) for Arduino assignments.

The emulator is operating on the well defined API. We are not simulating any low-level aspects of the actual processors used on Arduino boards.
On the other hand, we also implement support for Funshield which contains 3 buttons, 4 independent LEDs, and 4 digit 7-segment LED display controlled by shift register filled from Arduino over serial link.


## Code Overview

The most important part of the code is in `shared` directory. The surrounding projects are mainly designed to demonstrate and test the code.

- `constants.hpp` holds all constants and macros as defined in Arduino IDE
- `interface.hpp`, `interface.cpp` implements the low-level API for the tested Arduino code
- `funshield.h` holds additional constants needed for the Funshield (this header is given to students for development)
- `helpers.hpp` gathers all helper classes (`BitArray`, `ShiftRegister`)
- `time_series.hpp` is generalized implementation of sequence of events (used for various purposes, including analytical functions useful for behavioral assertions)
- `emulator.hpp` implements the actual state of the arduino board and provides object oriented interface (wich is recalled from C `interface`)
- `simulation.hpp` uses the `emulator.hpp` and implements controller for the simulation
- `led_display.hpp` is an implementation of 7-seg LED display accompanied by shift register (sequentially fed matrix control) and its demultiplexing and content decoding
- `simulation_funshield.hpp` uses `simulation.hpp` and implements higher-level of simulation routines targeting specifically Funshield applications


## Credits and Disclaimer

This code is currently being developed under Department of software engineering, Faculty of Mathematics and Physics, Charles University (Prague, Czech Republic). It is being tailored for our needs and we provide no guarantees whatsoever.
 