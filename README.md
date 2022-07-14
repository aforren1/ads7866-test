Potential steps for parallel read:

Normal SPI, except all ADCs hooked up to same chip select
Connect data to GPIO6

1. Start transaction
2. Read state from GPIO6_PSR (see `transfer` in SPI.h for teensy 4)
3. End transaction

Tips for GPIO6:
https://forum.pjrc.com/threads/58377-Reading-multiple-GPIO-pins-on-the-Teensy-4-0-quot-atomically-quot/page2 (IMXRT_GPIO6_DIRECT)
https://forum.pjrc.com/threads/69274-Reading-Pins-in-Parallel-Teensy-4-1?p=298049 (DIRECT, PSR, DR)
https://forum.pjrc.com/threads/61615-Teensy-4-1-Storing-the-value-of-18-pins-input-quickly
https://forum.pjrc.com/threads/60320-teensy-4-pinout-names post #3 for pin assign on teensy 4.0
i.e. we can pretty much use 14-22 for an easy 8 pins on teensy 4
for more, we could use a teensy 4.1 (see https://forum.pjrc.com/threads/69274-Reading-Pins-in-Parallel-Teensy-4-1?p=298049 post #17)
or move to micromod?

https://github.com/PaulStoffregen/SPI/blob/574ab8c7a8a45ea21cc56dcc6b7361da90868e86/SPI.h#L1069

or IMXRT_GPIO6_DIRECT?
