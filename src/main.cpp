/*
Normal SPI, except all ADCs are hooked up to the same chip select
and data pins are connected to GPIO6 (pins 14 & 15, respectively)

1. start transaction

*/

/*
TODO: clk_pin not reading
*/

#include <Arduino.h>
#include <SPI.h>

// selects both/all ADCs
constexpr int cs_pin = 10;
constexpr int clk_pin = 7;

constexpr int d0_pin = 14;
constexpr int d1_pin = 15;

uint16_t counter = 0;
constexpr uint16_t n_skip = 20; // read once every 20
uint16_t junk = 0;
uint16_t val0 = 0;
uint16_t val1 = 0;

const SPISettings sets(1700000, MSBFIRST, SPI_MODE3);

void setup() {
  while (!Serial) {}
  pinMode(cs_pin, OUTPUT);
  pinMode(clk_pin, INPUT);
  // https://forum.pjrc.com/threads/69274-Reading-Pins-in-Parallel-Teensy-4-1?p=298049
  GPIO6_GDIR &= 0xFFFF; // set all gpio6 pins to input
  digitalWriteFast(cs_pin, HIGH);
  SPI.begin();
  SPI.beginTransaction(sets);
  Serial.println("Here");
}

// completely readable
// taken from answer and comment from leoly
// https://stackoverflow.com/questions/47981/how-do-i-set-clear-and-toggle-a-single-bit#comment46654671_47990
inline uint16_t set_bit_at_pos(uint16_t val, uint8_t gpio_val, uint8_t pos) {
    return val & ~(1 << pos) | (gpio_val << pos);
}

elapsedMicros tm;

void loop() {
    Serial.println(counter);
    tm = 0;
    // start reading
    digitalWriteFast(cs_pin, LOW);
    if (counter == 0) {
        // watch the clock. We're going to manually
        // read the data out of GPIO6 after every clock cycle
        for (uint16_t i = 0; i < 16; i++) {
            // spin until low
            Serial.println(i);
            while (digitalReadFast(clk_pin)) {}
            // data valid? now read all of GPIO6, and shift out the bits we need
            register uint32_t data = GPIO6_PSR;
            val0 = set_bit_at_pos(val0, (data >> 18) & 1U, i); // pin 14
            val1 = set_bit_at_pos(val1, (data >> 19) & 1U, i); // pin 15
            // spin until high
            while (!digitalReadFast(clk_pin)) {}
        }

    } else {
        // read, but we're going to ignore the value
        // idea is to force the ADC to do the work, which
        // should spread out noise
        // we only really want ~1kHz, and we're getting 200kHz
        junk = SPI.transfer16(0);
    }

    // stop reading
    digitalWriteFast(cs_pin, HIGH);
    Serial.println(tm);
    // send data
    if (counter == 0) {
        Serial.print(val0);
        Serial.print(" ");
        Serial.println(val1);
    }
    counter += 1;
    counter %= n_skip;
}
