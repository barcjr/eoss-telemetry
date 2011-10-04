Morse Code
----------

There are two formats that we use to move morse code around. In the first, one bit equals one element. We'll call this bit-morse. In the other, one octet equals one 'building block.' We'll call this block-morse. These building blocks are the things the PIC needs to find individually. For example, a message might read "ALT 12F0M H " (except in morse code, obviously). It puts together a prefix ("ALT "), 1, 2, F, 0, and a suffix ("M H ") to form that message. Then, it goes through each building block one at a time and copies the bit-morse represented by that block into the schedule, which is also represented with bit-morse.

The schedule is 256 bits packed into 32 bytes. Two functions, getBitFromSchedule, and scheduleMorse, facilitate reading and writing, respectively. The function stepMorse is called every 120ms to take the next element on the schedule and set the radio pin to transmit it. It also wipes out the morse code it reads on a byte level so that you don't see morse from 256 ticks ago popping up. 

There are two tools for encoding and decoding the morse library. They are located in morse/.