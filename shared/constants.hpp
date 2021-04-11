#ifndef MOCCARDUINO_SHARED_CONSTANTS_HPP
#define MOCCARDUINO_SHARED_CONSTANTS_HPP

/*
 * Most of this code is taken directly from Arduino IDE code base.
 */

#include <cstdint>

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x0 // this is actually 0x2, but we do not want to distinguish INPUT and INPUT_PULLUP at the moment

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

#define CHANGE 1
#define FALLING 2
#define RISING 3

#define DEFAULT 0
#define EXTERNAL 1
#define INTERNAL1V1 2
#define INTERNAL INTERNAL1V1
#define INTERNAL2V56 3

#ifdef abs
#undef abs
#endif

/*
// These were defined by arduino codebase, but they mess up with math.h and stdlib.h...
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
*/
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#define lowByte(w) ((std::uint8_t) ((w) & 0xff))
#define highByte(w) ((std::uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

// avr-libc defines _NOP() since 1.6.2
#ifndef _NOP
#define _NOP() do { __asm__ volatile ("nop"); } while (0)
#endif

typedef unsigned int word;

#define bit(b) (1UL << (b))

typedef bool boolean;
typedef std::uint8_t byte;


// Pins

#define NUM_DIGITAL_PINS            20
#define NUM_ANALOG_INPUTS           6
#define digitalPinHasPWM(p)         ((p) == 3 || (p) == 5 || (p) == 6 || (p) == 9 || (p) == 10 || (p) == 11)

#define PIN_SPI_SS    (10)
#define PIN_SPI_MOSI  (11)
#define PIN_SPI_MISO  (12)
#define PIN_SPI_SCK   (13)

static const std::uint8_t SS = PIN_SPI_SS;
static const std::uint8_t MOSI = PIN_SPI_MOSI;
static const std::uint8_t MISO = PIN_SPI_MISO;
static const std::uint8_t SCK = PIN_SPI_SCK;

#define PIN_WIRE_SDA        (18)
#define PIN_WIRE_SCL        (19)

static const std::uint8_t SDA = PIN_WIRE_SDA;
static const std::uint8_t SCL = PIN_WIRE_SCL;

#define LED_BUILTIN 13

#define PIN_A0   (14)
#define PIN_A1   (15)
#define PIN_A2   (16)
#define PIN_A3   (17)
#define PIN_A4   (18)
#define PIN_A5   (19)
#define PIN_A6   (20)
#define PIN_A7   (21)

static const std::uint8_t A0 = PIN_A0;
static const std::uint8_t A1 = PIN_A1;
static const std::uint8_t A2 = PIN_A2;
static const std::uint8_t A3 = PIN_A3;
static const std::uint8_t A4 = PIN_A4;
static const std::uint8_t A5 = PIN_A5;
static const std::uint8_t A6 = PIN_A6;
static const std::uint8_t A7 = PIN_A7;


#define SERIAL_PORT_MONITOR   Serial
#define SERIAL_PORT_HARDWARE  Serial

#endif
