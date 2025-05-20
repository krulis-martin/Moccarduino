import sys
import csv
import os
import math

ON = 0
OFF = 1

#
# Mission critical functions
#


def judge_ok():
    '''
    Report success and terminate.
    '''
    print("1.0")
    sys.exit(0)


def judge_wrong(msg):
    '''
    Report failure (including a message) and terminate.
    '''
    print("0.0")
    print(msg)
    sys.exit(1)


def judge_internal(msg):
    '''
    Report internal error failure
    '''
    print("0.0")
    print("Internal error.")
    print(msg, file=sys.stderr)
    sys.exit(2)


#
# Classes
#

class WrongAnswer(Exception):
    '''
    An exception that is thrown if something is wrong with the tested output.
    '''

    def __init__(self, message):
        super().__init__(message)


class LedState:
    '''
    Represents the state of the 4 onboard LEDs. (a 4-bit value)
    '''

    def __init__(self, value=0xf):
        if type(value) is str:
            self.value = int(value, 16)
        elif type(value) is int:
            self.value = value
        else:
            raise Exception(
                "Unexpected value {} given as LED state.".format(value))

        if self.value < 0 or self.value > 0xf:
            raise Exception(
                "LED state value out of range ({}).".format(self.value))

    def get_raw(self):
        return self.value

    def __eq__(self, other):
        return self.value == other.value

    def __str__(self):
        str = ''
        v = self.value
        for _ in range(0, 4):
            str += '*' if (v & 1) == ON else '.'
            v = v >> 1
        return str

    def is_led_on(self, idx):
        return (self.value >> idx) & 1 == ON

    def set_led(self, idx, on=True):
        if on:
            self.value = self.value & 0xf ^ ((1 << idx) & 0xf)
        else:
            self.value = self.value | ((1 << idx) & 0xf)

    def get_bit_val(self):
        val = 0
        for i in range(0, 4):
            val = val * 2 + int(self.is_led_on(i))
        return val


class DisplayState:
    '''
    Represents a state of the 7seg display.
    '''

    # Translation table for letters
    letters = {
        0b10001000: "A",
        0b10000011: "B",
        0b11000110: "C",
        0b10100001: "D",
        0b10000110: "E",
        0b10001110: "F",
        0b10000010: "G",
        0b10001001: "H",
        0b11111001: "I",
        0b11100001: "J",
        0b10000101: "K",
        0b11000111: "L",
        0b11001000: "M",
        0b10101011: "N",
        0b10100011: "O",
        0b10001100: "P",
        0b10011000: "Q",
        0b10101111: "R",
        0b10010010: "S",
        0b10000111: "T",
        0b11000001: "U",
        0b11100011: "V",
        0b10000001: "W",
        0b10110110: "X",
        0b10010001: "Y",
        0b10100100: "Z",
    }

    # list of letter glyphs for inverse translation
    letter_glyphs = list(letters.keys())

    minus_sign = 0xbf
    empty_space = 0xff

    def __init__(self, value=0xffffffff):
        if type(value) is str:
            self.value = int(value, 16)
        elif type(value) is int:
            self.value = value
        else:
            raise Exception(
                "Unexpected value {} given as LED state.".format(value))

        if self.value < 0 or self.value > 0xffffffff:
            raise Exception(
                "LED state value out of range ({}).".format(self.value))

    def __eq__(self, other):
        return self.value == other.value

    def __str__(self):
        '''
         . _ . _ . _ . _ .
        . |_| |_| |_| |_| .
        . |_|o|_|o|_|o|_|o.
         .   .   .   .   .
        '''
        row1 = []
        row2 = []
        row3 = []
        segs2 = {0b100000: '|', 0b1000000: '_', 0b10: '|', }
        segs3 = {0b10000: '|', 0b1000: '_', 0b100: '|', 0x80: 'o'}
        for i in range(3, -1, -1):
            glyph = self.get_position_raw(i)
            row1.append('_' if glyph & 1 == ON else ' ')
            row2.append(
                ''.join(map(lambda s: s[1] if glyph & s[0] == ON else ' ',
                            segs2.items())))
            row3.append(
                ''.join(map(lambda s: s[1] if glyph & s[0] == ON else ' ',
                            segs3.items())))

        return (" . " + " . ".join(row1) + " .\n. " + ' '.join(row2)
                + " .\n. " + ''.join(row3) + ".\n ." + ('   .' * 4))

    def clear(self):
        self.value = 0xffffffff

    def get_raw(self):
        return self.value

    def get_position_raw(self, pos):
        return (self.value >> (pos * 8)) & 0xff

    def set_position_raw(self, pos, value):
        mask = (0xffffff00 << (pos * 8)) | (0xffffff >> ((3-pos) * 8))
        glyph = value << (pos * 8)
        self.value = (self.value & mask) | glyph

    def is_position_empty(self, pos, ignore_dot=False):
        mask = 0x7f if ignore_dot else 0xff
        return self.get_position_raw(pos) & mask == mask

    def get_glyph(self, pos):
        '''
        Return raw representation of a glyph (inverted logic) at given `pos`.
        Position 0 is the rightmost position, decimal dot is masked out.
        '''
        return self.get_position_raw(pos) | 0x80

    def get_digit(self, pos, empty_is_zero=False):
        '''
        Return decoded decimal digit at given position.
        None if the digit cannot be decoded.
        '''
        digits = {
            0xff: 0 if empty_is_zero else None,
            0xc0: 0,
            0xf9: 1,
            0xa4: 2,
            0xb0: 3,
            0x99: 4,
            0x92: 5,
            0x82: 6,
            0xf8: 7,
            0x80: 8,
            0x90: 9,
        }
        return digits.get(self.get_glyph(pos))

    def set_digit(self, pos, value):
        '''
        Show a digit at a particular position.
        '''
        digits = [0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90]
        self.set_position_raw(pos, digits[value])

    def has_decimal_dot(self, pos=None):
        '''
        Return true if decimal dot is set.
        If `pos` is none, only specified position is tested.
        '''
        if pos is None:
            return (self.value & 0x80808080) > 0  # at least one dot is set
        else:
            return ((self.value >> (pos * 8)) & 0x80) == 0

    def set_decimal_dot(self, pos, value=True):
        '''
        Modify decimal dot state at given `pos`.
        Value true sets the LED ON.
        '''
        mask = 0x80 << (pos * 8)
        if value:
            # true = bit is set to 0 (ON)
            mask = 0xffffffff ^ mask
            self.value &= mask
        else:
            # false = bit is set to 1 (OFF)
            self.value |= mask

    def get_decimal_positions(self):
        '''
        Return a list of all positions where a decimal dot is set.
        '''
        res = []
        for i in range(0, 4):
            if self.has_decimal_dot(i):
                res.append(i)
        return res

    def get_highest_nonempty_pos(self):
        '''
        Return index of the first nonempty position from the left:
        3 = thousands, 0 = units, -1 = entire display is empty
        '''
        for i in range(3, -1, -1):
            if not self.is_position_empty(i):
                return i
        return -1

    def get_number(self, decode_decimal_dot=False):
        '''
        Parse a number on the display. Returns None if state is not valid.
        If decimal dot is set, a float number may be returned.
        '''

        highest = self.get_highest_nonempty_pos()
        if highest < 0:
            return None

        res = 0
        for i in range(0, highest+1):
            if self.is_position_empty(i):
                return None

            digit = self.get_digit(i)
            if digit is None:
                if self.get_glyph(i) == self.minus_sign and i == highest:
                    res = -res
                else:
                    return None
            else:
                res += digit * (10 ** i)

        if decode_decimal_dot:
            dots = self.get_decimal_positions()
            if len(dots) > 0:
                if len(dots) > 1 and dots[0] <= highest:
                    return None
                res = res / (10 ** dots[0])

        return res

    def set_number(self, num):
        self.clear()
        minus = num < 0
        num = abs(num)
        for i in range(0, 4):
            if num > 0 or i == 0:
                self.set_digit(i, num % 10)
            elif minus:
                self.set_position_raw(i, self.minus_sign)
                break
            num = num // 10

    def get_letter(self, pos):
        '''
        Return decoded decimal digit at given position.
        None if the digit cannot be decoded.
        '''
        return self.letters.get(self.get_glyph(pos))

    def set_letter(self, pos, letter):
        '''
        Set a specific letter at specific position.
        Unknown characters are displayed as space ' '.
        '''
        letter = letter.lower()[:1]
        if not letter:
            raise Exception("Invalid letter '{}'.".format(letter))

        letter_index = ord(letter) - ord('a')
        if letter_index >= 0 and letter_index < len(self.letter_glyphs):
            glyph = self.letter_glyphs[letter_index]
        else:
            glyph = 0xff

        self.set_position_raw(pos, glyph)

    def _get_char(self, i, space, invalid_char, numbers):
        if self.is_position_empty(i, ignore_dot=True):
            return space

        if numbers:
            digit = self.get_digit(i)
            if digit is not None:
                return str(digit)

        letter = self.get_letter(i)
        if letter is not None:
            return letter

        return invalid_char

    def get_text(self, space=' ', decimal_dot=None, invalid_char=None,
                 numbers=True):
        '''
        Return plain text representation of the display.
        If invalid_char is set, unrecognizable positions are filled with it,
        otherwise None is returned if any position is not valid.
        '''
        res = []
        for i in range(3, -1, -1):
            char = self._get_char(i, space, invalid_char, numbers)
            if char is None:
                return None

            res.append(char)
            if decimal_dot and self.has_decimal_dot(i):
                res.append(decimal_dot)

        return ''.join(res)


class SimulationLog:
    '''
    Loads and holds all simulation log events.
    '''

    @staticmethod
    def _read_bool(s):
        if s is None or s == '':
            return None
        return bool(int(s))

    @staticmethod
    def _read_leds(s):
        if s is None or s == '':
            return None
        return LedState(s)

    @staticmethod
    def _read_display(s):
        if s is None or s == '':
            return None
        return DisplayState(s)

    def __init__(self, csv_file=None):
        self.events = []
        self.buttons = False
        self.leds = False
        self.display = False
        if csv_file is not None:
            self.load(csv_file)

    def has_buttons(self):
        return self.buttons

    def has_leds(self):
        return self.leds

    def has_display(self):
        return self.display

    def has_serial(self):
        return self.serial

    def load(self, csv_file):
        '''
        Load the log from a CSV produced by Moccarduino generic tester.
        Warning: at present, we cannot distinguish between no serial event and
        serial input containing an empty string.
        '''
        if not os.path.isfile(csv_file):
            raise Exception("CSV file {} does not exist.".format(csv_file))

        self.events = []
        with open(csv_file, 'r', encoding='utf8') as fp:
            # handle errors (when the log is not a CSV)
            first_line = fp.readline()
            if first_line.startswith("ERROR"):
                judge_wrong(fp.read())
            if first_line.startswith("INTERNAL ERROR"):
                judge_internal(fp.read())

            fp.seek(0)

            reader = csv.DictReader(fp)
            self.buttons = "b1" in reader.fieldnames
            self.leds = "leds" in reader.fieldnames
            self.display = "7seg" in reader.fieldnames
            self.serial = "serial" in reader.fieldnames

            for line in reader:
                line["timestamp"] = int(line["timestamp"])
                if self.buttons:
                    for b in ['b1', 'b2', 'b3']:
                        line[b] = self._read_bool(line.get(b))
                if self.leds:
                    line["leds"] = self._read_leds(line["leds"])
                if self.display:
                    line["7seg"] = self._read_display(line["7seg"])
                if self.serial and not line["serial"]:
                    line["serial"] = None
                self.events.append(line)

    def count(self):
        return len(self.events)

    def max_timestamp(self):
        if len(self.events) == 0:
            raise Exception("No events were loaded.")
        return self.events[-1]["timestamp"]

    def get(self, idx):
        return self.events[idx]

    def get_selected_events(self, type):
        return dict(filter(lambda v: v[1] is not None,
                           map(lambda e: (e["timestamp"], e[type]),
                               self.events)))

    def get_leds_values(self):
        return list(self.get_selected_events("leds").values())


#
# Additional functions
#

def statistics(data):
    '''
    Compute mean and deviation of integers on the input.
    '''
    if len(data) == 0:
        return None

    mean = 0
    mean2 = 0
    for x in data:
        mean += x
        mean2 += x * x
    mean = mean / len(data)
    mean2 = mean2 / len(data)
    # deviation = sqrt(E(X^2) - (EX)^2)
    return (mean, math.sqrt(mean2 - (mean*mean)))


def _find_best_match(events, value, max_time):
    '''
    Helper function that finds matching event at the beginning of events
    within the given time frame.
    Returns offset to events, -1 if no match is found
    '''
    if not events or events[0][0] > max_time:
        return -1  # no candidates within the timeframe

    for i in range(0, len(events)):
        if events[i][0] > max_time:
            break
        elif events[i][1] == value:
            return i
    return 0


def pair_events(expected, actual, time_window):
    '''
    Create mapping between two lists of events. An event is a tuple/list,
    where first item is timestamp, second item is the state/value.
    Expected events are calculated, actual are loaded from the log.
    The paring assumes the expected timestamp is always before actual ts.
    Returns a list of tuples where first item is from expected, second from
    actual. None may be used on either side for unpaired events.
    Time window defines maximal timestamp difference between paired events.
    '''
    mapping = []
    actual = actual[:]  # make a copy, so we can pop events
    for e in expected:
        while actual and actual[0][0] < e[0]:
            mapping.append((None, actual.pop(0)))

        best = _find_best_match(actual, e[1], e[0] + time_window)
        while best > 0:
            mapping.append((None, actual.pop(0)))
            best -= 1

        mapping.append((e, actual.pop(0) if best == 0 else None))

    return mapping


def _generate_button_events(from_time, to_time, button, delay, period):
    '''
    Helper function for get_button_action_events.
    Generate a list of button events for the given time interval.
    '''
    res = [(from_time, button)]
    time = from_time + delay
    while time < to_time:
        res.append((time, button))
        time += period
    return res


def get_button_action_events(log, end_time, delay=1000000, period=300000):
    '''
    Load button events and return a list of tuples (time, button) when a button
    action is actually triggered (taking repetitions into account).
    The delay and the period define button timing (like in the 3rd assignment).
    Buttons are indexed from zero (b1=0,...b3=2).
    '''
    res = []
    btns = {"b1": 0, "b2": 1, "b3": 2}
    states = [False, False, False]
    press_times = [0, 0, 0]
    for i in range(0, log.count()):
        event = log.get(i)
        for key, idx in btns.items():
            if event[key] is not None or states[idx] == event[key]:
                if event[key]:
                    press_times[idx] = event["timestamp"]
                elif states[idx]:
                    res += _generate_button_events(
                        press_times[idx], event["timestamp"],
                        idx, delay, period)
                states[idx] = event[key]

    for idx in range(0, len(states)):
        if states[idx]:
            res += _generate_button_events(
                press_times[idx], end_time, idx, delay, period)

    res.sort(key=lambda x: x[0])  # sort by time
    return res
