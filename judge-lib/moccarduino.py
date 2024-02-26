import sys
import csv
import os

ON = 0
OFF = 1


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

    def get_raw(self):
        return self.value

    def get_position_raw(self, pos):
        return (self.value >> (pos * 8)) & 0xff

    def is_position_empty(self, pos):
        return self.get_position_raw(pos) == 0xff

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
        digits = [0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90]
        mask = (0xffffff00 << (pos * 8)) | (0xffffff >> ((3-pos) * 8))
        glyph = digits[value] << (pos * 8)
        self.value = (self.value & mask) | glyph

    def has_decimal_dot(self, pos=None):
        '''
        Return true if decimal dot is set.
        If `pos` is none, only specified position is tested.
        '''
        if pos is None:
            return (self.value & 0x80808080) > 0  # at least one dot is set
        else:
            return ((self.value >> (pos * 8)) & 0x80) > 0

    def get_decimal_positions(self):
        '''
        Return a list of all positions where a decimal dot is set.
        '''
        res = []
        for i in range(0, 4):
            if self.has_decimal_dot(i):
                res.append(i)

    def get_number(self, decode_decimal_dot=False):
        '''
        Parse a number on the display. Returns None if state is not valid.
        If decimal dot is set, a float number may be retuned.
        '''
        res = 0
        for i in range(0, 4):
            if self.is_position_empty(i):
                break
            digit = self.get_digit(i)
            if digit is None:
                return None
            res = (res * 10) + digit

        if decode_decimal_dot:
            dots = self.get_decimal_positions()
            if len(dots) > 0:
                if len(dots) > 1:
                    return None
                return res / (10 ** dots[0])
            else:
                return res

    def get_char(self, pos):
        # TODO
        return None

    def get_text(self, space=' ', invalid_char=None):
        '''
        Return plain text representation of the display.
        If invalid_char is set, unrecognizable positions are filled with it,
        otherwise None is returned if any position is not valid.
        '''
        res = []
        for i in range(3, -1, -1):
            if self.is_position_empty(i):
                res.append(space)
                continue

            digit = self.get_digit(i)
            if digit is not None:
                res.append(str(digit))
                continue

            char = self.get_char(i)
            if char is not None:
                res.append(char)
                continue

            if invalid_char is None:
                return None
            res.append(invalid_char)

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

    def load(self, csv_file):
        '''
        Load the log from a CSV produced by Moccarduino generic tester.
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

            for line in reader:
                line["timestamp"] = int(line["timestamp"])
                if self.buttons:
                    for b in ['b1', 'b2', 'b3']:
                        line[b] = self._read_bool(line.get(b))
                if self.leds:
                    line["leds"] = self._read_leds(line["leds"])
                if self.display:
                    line["7seg"] = self._read_display(line["7seg"])
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
