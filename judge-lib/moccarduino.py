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


class SimulationLog:
    '''
    '''
    @staticmethod
    def _read_bool(s):
        if s is None or s == '':
            return None
        return bool(int(s))

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
                    line["leds"] = LedState(line["leds"])
                self.events.append(line)

    def count(self):
        return len(self.events)

    def max_timestamp(self):
        if len(self.events) == 0:
            raise Exception("No events were loaded.")
        return self.events[len(self.events)-1]["timestamp"]

    def get(self, idx):
        return self.events[idx]

    def get_leds(self):
        return list(filter(None, map(lambda e: e["leds"], self.events)))
