Generic tester is configured by command line arguments, reads input (button) events from a file and produces a simulation event log.

### Command line arguments

- `--save` - Path to a file to which the simulation log (as CSV) is saved (stdout is used, if no file is given).
- `--simulation-length` - Length of the simulation in ms (overrides value from input file, required if no input file is provided).
- `--loop-delay` - Delay between two loop invocations [us] (default 100).
- `--log-buttons` - Add button events into output log.
- `--log-leds` - Add LED events into output log.
- `--log-7seg` - Add events of the 7-segment display into output log.
- `--raw-leds` - Deactivate LEDs event smoothing by demultiplexer and aggregator.
- `--leds-demuxer-window` - Size of the LEDs demultiplexing window [ms].
- `--leds-aggregator-window` - Size of the LEDs demultiplexing window [ms]."
- `--raw-7seg` - Deactivate 7-seg display event smoothing by demultiplexer and aggregator.
- `--7seg-demuxer-window` - Size of the LEDs demultiplexing window [ms].
- `--7seg-aggregator-window` - Size of the LEDs demultiplexing window [ms].
- `--enable-delay` - If set, builtin functions delay() and delayMicroseconds() are enabled.
- `--one-latch-loop` - Limit only one 7seg latch activation in each loop.

Optionally, the application takes one position argument -- a path to the input file, from which the button events are loaded. If `-` is given instead of a path, stdin is used to load input.

Example (4th or 5th assignment might be tested like this):
```
$> generic_tester --log-buttons --log-7seg --one-latch-loop -
```

### Input file format

Input is a simple text file (similar to CSV, but uses spaces instead of ',' or ';'), so it can be easily processed in C++ without any additional libs. Each input event is on a single row, evens must be ordered by simulation timestamp in ascending order.

Each line has a fixed format:
```
<timestamp> <button_number> <new_state>
```
- `--timestamp` is simulation time (microseconds from the beginning) in decimal format
- `--`button_number` is the index of the button (1-3 with currently supported funshield)
- `--new_state` is a single char `'u'` (up) or `'d'` (down) indicating the new state of the button

Optionally, the last line of the input file may hold only the timestamp (no button number or state) to denote the end time of the simulation. If the end time is missing, it is set shortly after the last button event (the delay is implementation-defined).


### Output file format

The output file is a CSV that uses commas as a separator (no quotes are used for value wrapping since the values are trivial). The first column is always a timestamp, remaining columns are defined on the first row based on the configuration arguments (whether particular events are logged). Possible columns are:

- `--timestamp` - event simulation time (microseconds from the beginning) in decimal format
- `--b1`, `b2`, `b3` - buttons, possible values are 0 = button is released, 1 = button is down
- `--leds` - four bit values encoded into single hex digit (0-f), least significant bit is LED #1 (according to `funshield.h`) uses inverted logic (1 = OFF, 0 = ON)
- `--7seg` - output of 7-seg display binary value (4B) encoded in hex (8 digits) holding a digital representation of the display state (first byte is the rightmost position), also uses inverted logic

All columns besides `timestamp` are filled only when the value is changed at that time (otherwise it is an empty string). Note that multiple changing events may take place at the same time, so multiple different columns may be non-empty on the same row.

The LEDs and 7seg display use smoothening unless the are switched to _raw_* collection (by a particular argument).

