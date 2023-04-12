![Windows MSVC Build](https://github.com/krulis-martin/Moccarduino/workflows/Windows%20MSVC%20Build/badge.svg)
![Linux GCC Build](https://github.com/krulis-martin/Moccarduino/workflows/Linux%20GCC%20Build/badge.svg)

# Moccarduino

Mocking environment for testing Arduino code. The sole objective of this project is to provide a testing platform that could be used in
[ReCodEx](https://github.com/ReCodEx) for Arduino assignments.

The emulator is operating on a well-defined API. We are not simulating any low-level aspects of the actual processors used on Arduino boards.
On the other hand, we also implement support for Funshield which contains 3 buttons, 4 independent LEDs, and 4 digit 7-segment LED display controlled by a shift register filled from Arduino over a serial link.

## Limitations and differences

There are several differences from the actual Arduino which may cause you problems. Please, check the following issues and use suggested workarounds so that your Arduino code works properly in Moccarduino.

<table class="table">
<thead>
	<tr>
		<th>Issue</th>
		<th>Example</th>
		<th>Workaround</th>
	</tr>
</thead>
<tbody>
	<tr>
		<td>Unsupported complex initialization of global variables</td>
		<td class="text-nowrap"><code>size_t t = millis();</code></td>
		<td>
			Initialize the variable with a default value and call the complex initialization in 
			the <code>setup()</code> function.
		</td>
	</tr>
	<tr>
		<td>Serial communication API</td>
		<td class="text-nowrap"><code>Serial.print();</code></td>
		<td>Only <code>Serial.begin()</code>, <code>Serial.print()</code>, and <code>Serial.println()</code> are currently implemented (which should be enough for debugging). The methods does not perform anything, they are placeholders, so you do not remove you debug-code when testing. The testing scenaion may opt-out (disable the serial interface).</td>
	</tr>
	<tr>
		<td><code>max</code> is a function (but a macro at Arduino IDE)</td>
		<td class="text-nowrap"><code>int x; long y; max(x,y);</code></td>
		<td>Always use min/max with identical types of arguments.</td>
	</tr>
	<tr>
		<td>int/long are 16/32 bits at Arduino but 32/64 in most other compilers</td>
		<td class="text-nowrap"><code>int32_t x; long y; max(x,y);</code></td>
		<td>Do not mix int/long with int16_t/int32_t; make sure to use sufficiently large type for both platforms</td>
	</tr>
	<tr>
		<td>Funshield include</td>
		<td class="text-nowrap"><code>#include &lt;funshield.h&gt;</code></td>
		<td>Replace by <code>#include "funshield.h"</code></td>
	</tr>
	<tr>
		<td>Conservative C++ (order of declarations)</td>
		<td class="text-nowrap"><code>int main() { foo(); ...}</code><br><code>void foo() {...}</code></td>
		<td>In C++, you need to declare functions (classes, ...) before you use them (Arduino IDE is more benevolent).</td>
	</tr>
	<tr>
		<td>Static methods</td>
		<td class="text-nowrap"><code>class Foo { static void foo() ... }</code></td>
		<td>Static methods do not work in Moccarduino, use plain old C functions instead.</td>
	</tr>
	<tr>
		<td>Initialization</td>
		<td class="text-nowrap"><code>class Button { </code><br>
			<code>  Button() { pinMode( b[0], INPUT); } </code><br>
			<code>};</code></td>
		<td>Emulated functions from Arduino IDE (e.g., pinMode) <b>MUST</b> be called in setup (not in constructors). Early emulator initialization (e.g., in a constructor of a global object) causes a signal and your program is terminated.</td>
	</tr>
	<tr>
		<td>Unsupported type <code>String</code></td>
		<td class="text-nowrap"><code>String stringOne = "Hello String";</code></td>
		<td>Use standard C-strings instead, i.e., <code>const char *stringOne = "Hello String";</code></td>
	</tr>
</tbody>
</table>

Furthermore, the Moccarduino implements only functions from documented Arduino API. Hardware functionality that required direct access to registers (e.g., timers) is not supported.

## Code Overview

The most important part of the code is in `shared` directory. The surrounding projects are mainly designed to demonstrate and test the code.

- `constants.hpp` holds all constants and macros as defined in Arduino IDE
- `interface.hpp`, `interface.cpp` implements the low-level API for the tested Arduino code
- `funshield.h` holds additional constants needed for the Funshield (this header is given to students for development)
- `helpers.hpp` gathers all helper classes (`BitArray`, `ShiftRegister`)
- `time_series.hpp` is a generalized implementation of a sequence of events (used for various purposes, including analytical functions useful for behavioral assertions)
- `emulator.hpp` implements the actual state of the Arduino board and provides an object-oriented interface (which is recalled from C `interface`)
- `simulation.hpp` uses the `emulator.hpp` and implements controller for the simulation
- `led_display.hpp` is an implementation of 7-seg LED display accompanied by shift register (sequentially fed matrix control) and its demultiplexing and content decoding
- `simulation_funshield.hpp` uses `simulation.hpp` and implements higher-level simulation routines targeting specifically Funshield applications


## Credits and Disclaimer

This code is currently being developed under the Department of software engineering, Faculty of Mathematics and Physics, Charles University (Prague, Czech Republic). It is being tailored to our needs and we provide no guarantees whatsoever.
 