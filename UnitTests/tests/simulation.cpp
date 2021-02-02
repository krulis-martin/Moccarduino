#include "simulation.hpp"

#include "../test.hpp"

#include <functional>
#include <cstdint>

class DisableFunctionsTest : public MoccarduinoTest
{
private:
	void testDisableFunction(ArduinoSimulationController &simulation, const std::string& fncName, std::function<void()> const& op) const
	{
		simulation.disableMethod(fncName);
		std::string comment("disable " + fncName + "()");
		ASSERT_EXCEPTION(ArduinoEmulatorException, op, comment);
		simulation.enableMethod(fncName);
		try {
			op();
		}
		catch (ArduinoEmulatorException&) {
			ASSERT_TRUE(false, "enabled method " + fncName + "() throws on invocation");
		}
	}

public:
	DisableFunctionsTest() : MoccarduinoTest("simulation/disable-functions") {}

	virtual void run() const
	{
		ArduinoEmulator emulator;
		ArduinoSimulationController simulation(emulator);
		simulation.registerPin(1, INPUT);
		emulator.pinMode(1, INPUT);
		simulation.registerPin(2, OUTPUT);
		emulator.pinMode(2, OUTPUT);

		testDisableFunction(simulation, "pinMode", [&]() { emulator.pinMode(1, INPUT); });
		testDisableFunction(simulation, "digitalWrite", [&]() { emulator.digitalWrite(2, 0); });
		testDisableFunction(simulation, "digitalRead", [&]() { emulator.digitalRead(1); });
		testDisableFunction(simulation, "analogRead", [&]() { emulator.analogRead(1); });
		//testDisableFunction(simulation, "analogReference", [&]() { emulator.analogReference(); }); NOT IMPLEMENTED YET
		//testDisableFunction(simulation, "analogWrite", [&]() { emulator.analogWrite(); }); NOT IMPLEMENTED YET
		testDisableFunction(simulation, "millis", [&]() { emulator.millis(); });
		testDisableFunction(simulation, "micros", [&]() { emulator.micros(); });
		testDisableFunction(simulation, "delay", [&]() { emulator.delay(1); });
		testDisableFunction(simulation, "delayMicroseconds", [&]() { emulator.delayMicroseconds(1); });
		//testDisableFunction(simulation, "pulseIn", [&]() { emulator.pulseIn(); }); NOT IMPLEMENTED YET
		//testDisableFunction(simulation, "pulseInLong", [&]() { emulator.delay(1); }); NOT IMPLEMENTED YET
		testDisableFunction(simulation, "shiftOut", [&]() { emulator.shiftOut(2, 2, LSBFIRST, 0); });
		testDisableFunction(simulation, "shiftIn", [&]() { emulator.shiftIn(1, 2, LSBFIRST); });
		//testDisableFunction(simulation, "tone", [&]() { emulator.tone(); }); NOT IMPLEMENTED YET
		//testDisableFunction(simulation, "noTone", [&]() { emulator.noTone(); }); NOT IMPLEMENTED YET

	}
};


DisableFunctionsTest _disableFunctionsTest;
