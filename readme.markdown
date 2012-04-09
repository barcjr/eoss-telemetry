BARC Jr. EOSS Telemetry Project
===============================

This project reads pressure and temperature data, records both of them in EEPROM, calculates the current altitude, and then transmits the altitude over the air via morse code.

Peripherals
-----------

The PIC has 4 peripherals: The EEPROM (for storing readings), the BMP085 (for capturing readings), the UART (for sending debug messages and the collected data) and last, but not least, the radio on/off pin. 

Morse Code
----------

See [doc/morse.markdown](https://github.com/nickodell/eoss-telemetry/blob/master/doc/morse.markdown)

EOSS Flight Results
-------------------
See [EOSS_Flight_Results.md](https://github.com/barcjr/eoss-telemetry/blob/master/EOSS_Flight_Results.md)