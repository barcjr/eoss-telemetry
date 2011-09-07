BARC EOSS Telemetry Project
===========================

This project, in its finished state, will read pressure and temperature data, record both of them in E²PROM, calculate the present altitude in a temporary variable, and then transmit the altitude over the air via morse code.  Note that some of this documentation describes things that don't exist yet.

Peripherals
-----------

The PIC has 4 peripherals: The EEPROM (for storing readings), the BMP085 (for capturing readings), the USART (for sending debug messages and the collected data) and last, but not least, the radio on/off pin. 

Morse Code
----------

There are two formats that we use to move morse code around. In the first, one bit equals one element. We'll call this bit-morse. In the other, one octet equals one 'building block.' We'll call this block-morse. These building blocks are the things the PIC needs to find individually. For example, a message might read "ALT 12F0M H " (except in morse code, obviously). It puts together a prefix ("ALT "), 1, 2, F, 0, and a suffix ("M H ") to form that message. Then, it goes through each building block one at a time and copies the bit-morse in that block into the schedule, which is also represented by bit-morse.

The schedule is 256 bits packed into 32 bytes. Two functions, getBitFromSchedule, and scheduleMorse, facilitate reading and writing, respectively. The function stepMorse is called every 120ms to take the next element on the schedule and set the radio pin to transmit it. It also wipes out the morse code it reads on a byte level so that you don't see morse from 256 ticks ago popping up. 

Note: These building blocks are all pre-calculated by a script. Use `morse/morse_encode.py` to generate the morse code array. 

Power
-----

The PIC, EEPROM, and sensor (the BMP085) consume less than 5 mA in total, so it can be powered by a coin cell. It probably shouldn't be powered off the same supply as the radio, though.