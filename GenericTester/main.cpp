#include "args.hpp"

#include "dataio.hpp"
#include "simulation_funshield.hpp"
#include "simulation.hpp"
#include "emulator.hpp"
#include "led_display.hpp"
#include "helpers.hpp"

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

// a declaration of a method from interface that will give us the emulator instance
ArduinoEmulator& get_arduino_emulator_instance();

using leds_state_t = FunshieldSimulationController::leds_display_t::state_t;
using display_state_t = FunshieldSimulationController::seg_display_t::state_t;
using output_events_t = std::map<std::string, std::shared_ptr<TimeSeriesBase<>>>;


/**
 * Load button events from input file (or stdin), feed them to funshield, and prepare output events series for logging.
 */
logtime_t processInput(bpp::ProgramArguments &args, FunshieldSimulationController &funshield, output_events_t &outputEvents)
{
    std::vector<std::shared_ptr<TimeSeries<bool>>> buttonEvents;
    if (args.getArgBool("log-buttons").getValue()) {
        buttonEvents.emplace_back(std::make_shared<TimeSeries<bool>>());
        buttonEvents.emplace_back(std::make_shared<TimeSeries<bool>>());
        buttonEvents.emplace_back(std::make_shared<TimeSeries<bool>>());
    }

    logtime_t simulationTime = 0;

    if (args.namelessCount() > 0) {
        if (args[0] != "-") {
            // load events from a file
            std::ifstream sin(args[0], std::ios::binary);
            if (!sin.is_open()) {
                throw std::runtime_error("Failed to open input file " + args[0]);
            }
            simulationTime = loadInputData(sin, funshield, buttonEvents);
        }
        else {
            // load events from stdin
            simulationTime = loadInputData(std::cin, funshield, buttonEvents);
        }
    }
    else {
        if (!args.getArgInt("simulation-length").isPresent()) {
            throw std::runtime_error("Argument '--simulation-length' is required when no input file is given.");
        }
    }

    if (args.getArgInt("simulation-length").isPresent()) {
        // possibly override simulation time
        simulationTime = (logtime_t)args.getArgInt("simulation-length").getValue() * 1000;
    }
    
    if (args.getArgBool("log-buttons").getValue()) {
        outputEvents["b1"] = buttonEvents[0];
        outputEvents["b2"] = buttonEvents[1];
        outputEvents["b3"] = buttonEvents[2];
    }

    return simulationTime;
}


/**
 * Merge all output series together and format them in CSV.
 */
void processOutput(bpp::ProgramArguments& args, output_events_t& outputEvents)
{
    if (args.getArgString("save").isPresent()) {
        std::ofstream sout(args.getArgString("save").getValue(), std::ios::binary);
        printEvents(sout, outputEvents);
    }
    else {
        printEvents(std::cout, outputEvents);
    }
}

int main(int argc, char* argv[])
{
    bpp::ProgramArguments args(0, 1);
    args.setNamelessCaption(0, "Input file with button events.");

    try {
        args.registerArg<bpp::ProgramArguments::ArgString>("save", "Path to a file to which the simulation log (as CSV) is saved (stdout is used, if no file is given).", false);

        args.registerArg<bpp::ProgramArguments::ArgInt>("simulation-length", "Length of the simulation in ms (overrides value from input file, required if no input file is provided).", false, 0, 0);
        args.registerArg<bpp::ProgramArguments::ArgInt>("loop-delay", "Delay between two loop invocations [us].", false, 100, 1);
        args.registerArg<bpp::ProgramArguments::ArgBool>("log-buttons", "Add button events into output log.");
        args.registerArg<bpp::ProgramArguments::ArgBool>("log-leds", "Add LED events into output log.");
        args.registerArg<bpp::ProgramArguments::ArgBool>("log-7seg", "Add events of the 7-segment display into output log.");

        args.registerArg<bpp::ProgramArguments::ArgBool>("raw-leds", "Deactivate LEDs event smoothing by demultiplexer and aggregator.");
        args.registerArg<bpp::ProgramArguments::ArgInt>("leds-demuxer-window", "Size of the LEDs demultiplexing window [ms].", false, 10, 0);
        args.registerArg<bpp::ProgramArguments::ArgInt>("leds-aggregator-window", "Size of the LEDs demultiplexing window [ms].", false, 50, 0);

        args.registerArg<bpp::ProgramArguments::ArgBool>("raw-7seg", "Deactivate 7-seg display event smoothing by demultiplexer and aggregator.");
        args.registerArg<bpp::ProgramArguments::ArgInt>("7seg-demuxer-window", "Size of the LEDs demultiplexing window [ms].", false, 15, 0);
        args.registerArg<bpp::ProgramArguments::ArgInt>("7seg-aggregator-window", "Size of the LEDs demultiplexing window [ms].", false, 30, 0);

        args.registerArg<bpp::ProgramArguments::ArgBool>("enable-delay", "If set, builtin functions delay() and delayMicroseconds() are enabled.");
        args.registerArg<bpp::ProgramArguments::ArgBool>("one-latch-loop", "Limit only one 7seg latch activation in each loop.");

        // Process the arguments ...
        args.process(argc, argv);
    }
    catch (bpp::ArgumentException& e) {
        std::cout << "Invalid arguments: " << e.what() << std::endl << std::endl;
        args.printUsage(std::cout);
        return 1;
    }

    output_events_t outputEvents;

    // initialize simulation
    ArduinoSimulationController arduino(get_arduino_emulator_instance());
    FunshieldSimulationController funshield(arduino);

    if (!args.getArgBool("enable-delay").getValue()) {
        arduino.disableMethod("delay");
        arduino.disableMethod("delayMicroseconds");
    }

    try {
        logtime_t simulationTime = processInput(args, funshield, outputEvents);

        // LEDs
        LedsEventsDemultiplexer<4> ledDemuxer(args.getArgInt("leds-demuxer-window").getValue() * 1000);
        LedsEventsAggregator<4> ledAggregator(args.getArgInt("leds-aggregator-window").getValue() * 1000);
        auto ledEvents = std::make_shared<TimeSeries<leds_state_t>>();
        if (args.getArgBool("log-leds").getValue()) {
            if (args.getArgBool("raw-leds").getValue()) {
                // collecting raw LED events
                funshield.getLeds().attachSproutConsumer(*ledEvents);
            }
            else {
                // LED events smoothing using demuxer and aggregator
                funshield.getLeds().attachSproutConsumer(ledDemuxer);
                ledDemuxer.attachNextConsumer(ledAggregator);
                ledAggregator.attachNextConsumer(*ledEvents);
            }
            outputEvents["leds"] = ledEvents;
        }

        // 7-seg display
        LedsEventsDemultiplexer<32> segDemuxer(args.getArgInt("7seg-demuxer-window").getValue() * 1000);
        LedsEventsAggregator<32> segAggregator(args.getArgInt("7seg-aggregator-window").getValue() * 1000);
        auto segEvents = std::make_shared<TimeSeries<display_state_t>>();
        if (args.getArgBool("log-7seg").getValue()) {
            if (args.getArgBool("raw-7seg").getValue()) {
                // collecting raw LED events
                funshield.getSegDisplay().attachSproutConsumer(*segEvents);
            }
            else {
                // LED events smoothing using demuxer and aggregator
                funshield.getSegDisplay().attachSproutConsumer(segDemuxer);
                segDemuxer.attachNextConsumer(segAggregator);
                segAggregator.attachNextConsumer(*segEvents);
            }
            outputEvents["7seg"] = segEvents;
        }

        // run simulation
        arduino.runSetup();

        // This analysis is performed to ensure that in one loop is only one display change (latch activation)
        std::size_t loopsCount = 0;
        std::size_t violatedLoopsCount = 0;
        std::size_t lastLoopLatchActivations = 0;
        bool lastLatchState = true;
        EventAnalyzer<ArduinoPinState> displayLatchAnalyzer([&](logtime_t time, ArduinoPinState state)
            {
                if (state.pin == latch_pin) {
                    bool pinValue = state.value == HIGH ? true : false;
                    if (!lastLatchState && pinValue) { // LOW -> HIGH edge
                        ++lastLoopLatchActivations;
                    }
                    lastLatchState = pinValue;
                }
            }
        );
        funshield.getSegDisplay().attachNextConsumer(displayLatchAnalyzer);

        logtime_t loopDelay = args.getArgInt("loop-delay").getValue();
        arduino.runLoopsForPeriod(simulationTime, loopDelay, [&](logtime_t)
            {
                if (lastLoopLatchActivations > 1) {
                    ++violatedLoopsCount;
                }
                lastLoopLatchActivations = 0; // reset for the next loop
                ++loopsCount;
                return true;
            }
        );

        if (args.getArgBool("one-latch-loop").getValue() && violatedLoopsCount > 0) {
            std::cout << "The single-latch-activation rule was violated in " << violatedLoopsCount << " loop() invocations." << std::endl;
            return 2;
        }

        // make sure 
        if (outputEvents.empty()) {
            std::cout << "Simulation ended successfully, but no event logging was selected." << std::endl;
        }
        else {
            processOutput(args, outputEvents);
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 100;
    }

    return 0;
}
