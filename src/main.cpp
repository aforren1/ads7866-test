/*
Normal SPI, except all ADCs are hooked up to the same chip select
and data pins are connected to GPIO6 (pins 14 & 15, respectively)

For expanding, can probably find enough pins between GPIO6 and 7
(need to be more careful about pin selection on 7, b/c it might run into SPI pins?)
*/

#include <Arduino.h>
#include <SPI.h>
#include <ResponsiveAnalogRead.h>

// selects both/all ADCs
constexpr int cs_pin = 10;
constexpr int clk_pin = 7;

constexpr int d0_pin = 14;
constexpr int d1_pin = 15;

uint8_t counter = 0;
constexpr uint16_t n_skip = 200; // read once every 20
uint16_t junk = 0;
uint16_t val0 = 0;
uint16_t val1 = 0;

const SPISettings sets(3400000, MSBFIRST, SPI_MODE3);
EventResponder callbackHandler;
volatile int dma_dun = 0;

void callback(EventResponderRef er) {
    dma_dun = 1;
}

ResponsiveAnalogRead an1;

void setup() {
  while (!Serial) {}
  // TODO: weird, this completely crashes teensy??
  //an1.begin(1, true);
  callbackHandler.attachImmediate(&callback);
  pinMode(cs_pin, OUTPUT);
  pinMode(clk_pin, INPUT);
  pinMode(6, OUTPUT);
  // https://forum.pjrc.com/threads/69274-Reading-Pins-in-Parallel-Teensy-4-1?p=298049
  GPIO6_GDIR &= 0xFFFF; // set all gpio6 pins to input
  digitalWriteFast(cs_pin, HIGH);
  SPI.begin();
  SPI.beginTransaction(sets);
  Serial.println("Here");
}

// taken from answer and comment from leoly
// https://stackoverflow.com/questions/47981/how-do-i-set-clear-and-toggle-a-single-bit#comment46654671_47990
inline uint16_t set_bit_at_pos(uint16_t val, uint8_t gpio_val, uint8_t pos) {
    return (val & ~(1 << pos)) | (gpio_val << pos);
}

elapsedMicros tm;
uint8_t data[] = {1, 1};
uint32_t t = 0;
uint32_t t0 = 0;
uint32_t dt = 0;

void loop() {
    //Serial.println(counter);
    //tm = 0;

    // start reading
    if (counter == 0) {
        digitalWriteFast(cs_pin, LOW);
        // watch the clock. We're going to manually
        // read the data out of GPIO6 after every clock cycle
        // NB we could do real work with this (i.e. add another sensor)
        // but for now, just demonstrate how this might work
        // with GPIO6 pins
        SPI.transfer(data, nullptr, 2, callbackHandler);
        for (uint16_t j = 0; j < 2; j++) {
            //while (!digitalReadFast(clk_pin)) {}
            for (uint16_t i = 0; i < 8; i++) {
                // spin until low
               // Serial.println(i);
                while (digitalReadFast(clk_pin)) {}
                digitalWriteFast(6, LOW);
                if (i == 0 && j == 0) {
                    t = tm;
                    dt = t - t0;
                    t0 = t;
                }
                // data valid? now read all of GPIO6, and shift out the bits we need
                register uint32_t data = GPIO6_PSR;
                //register uint32_t d2 = GPIO7_PSR; 
                uint16_t idx = 15-(i + j*8);
                // TODO: in "real" version, we should have tx_data allocated
                // and just directly toggle bits in that? Save some memcpys later
                val0 = set_bit_at_pos(val0, (data >> 18) & 1U, idx); // pin 14
                val1 = set_bit_at_pos(val1, (data >> 19) & 1U, idx); // pin 15
                digitalWriteFast(6, HIGH);
                // spin until high (unless last iteration)
                if (!(i == 7 && j == 1)) {
                    while (!digitalReadFast(clk_pin)) {}
                }
            }

        }
        digitalWriteFast(cs_pin, HIGH);
        // wait for DMA?
        // while (!dma_dun) {}
        // dma_dun = 0;

    } else if ((counter % 2) == 0) {
        // read every other time, but we're going to ignore the value
        // idea is to oversample, then decimate
        // we only really want ~1kHz, and we're getting up to 200kHz
        // but sending data takes a few us, so if we read every other time
        // we can account for the extra slop. i.e.
        // - clock to run at 200kHz
        // - sample at 100kHz
        // - report at 1kHz
        digitalWriteFast(cs_pin, LOW);
        junk = SPI.transfer16(0);
        // SPI.transfer(data, nullptr, 2, callbackHandler);
        // an1.update(val0);
        digitalWriteFast(cs_pin, HIGH);
        // TODO: use ResponsiveAnalogRead to filter out noise during this time?
    } else {
        // TODO: why does this work??
        // half-guesses I haven't thought through:
        // our "raw" sampling period is 200kHz, so must sleep for
        // up to 5us == 5000ns to account for the missing interval
        // however, sending data on the non-skipped times
        // takes some amount of time, so we need to account
        // for slop in that process-- otherwise, we'll keep drifting
        // by however long that takes
        // this is a magic number for our 1kHz sampling rate
        delayNanoseconds(4670);
    }

    // stop reading
    //Serial.println(tm);
    // send data
    if (counter == 0) {
        // Serial.print(dt);
        // Serial.print(" ");
        Serial.print(val0);
        Serial.print(" ");
        Serial.println(val1);
        Serial.send_now();
    }
    counter += 1;
    counter %= n_skip;
    //Serial.println(tm);
    //delayMicroseconds(200);
}
