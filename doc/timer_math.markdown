Interrupt timer settings
========================

In EOSS/EOSS_Project.c, there is the following define:

    #define INTERRUPT_CLOCK_SETTING 5536

How was this gotten?

Interrupt timers
----------------

**This section is an oversimplification.**

There is a register that the PIC increments (increases by one) every cycle, no matter what it is doing. Eventually, the number in that register is so large that the top digit is removed and the register becomes 0. When the register is zero, the PIC runs a special section of the code called an interrupt. That interrupt sets the register so that the interrupt runs again in a little bit, and so on.

Calculating the clock setting
-----------------------------

First, we figure out how often we want the interrupt to fire. We wanted 20WPM morse. 

    20 words = 1000 elements
    1000 elements per minute = 1000/60 elements per second

But we want seconds per element, not elements per second.

    60/1000 elements per second = 0.06 seconds = 60 milliseconds

So the interrupt should fire every 60 milliseconds.

Then, we take the clock speed. In our case, 8MHz. Divide it by 4. 2MHz. This means that register will be incremented 2 million times a second. But how many times will it be incremented in 60 ms?

    2,000,000 * 0.060 = 120,000

Next, if we use a 16-bit timer, then there can only be 2^16 increments before the interrupt runs. This means we have to add a prescaler of 1:2.

    2,000,000 * 0.60 / 2 = 60,000

Subtract from 2^16 to get our clock setting.

    2^16 - 60000 = 5536

