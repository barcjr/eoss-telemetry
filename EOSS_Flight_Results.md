Edge of Space Sciences (EOSS) Launch
====================================
On February 25th, 2012, the Boulder Amateur Radio Club for Juniors (BARC Jr.) participated in its very first EOSS
launch.

Payload Design
-----------
Using a PIC microcontroller, a BMP085 pressure and temperature sensor, an EEPROM memory chip, and a 10-meter
transmitter - all designed from scratch - the payload successfully performs the following actions:

1. Upon startup, transmits our amateur radio callsign for FCC identification. Our scheduling code ensures that we
	transmit our callsign at 10-minute intervals, depending on the perceived data measurements. If the data is
	expected to extend past the 10-minute period, the PIC will merely store the data and transmit the callsign.		
	
2. Using a BMP085 temperature and pressure sensor, the PIC gathers data, stores that data in memory, and then
	derives the current altitude based on those readings. Averaging of twenty samples per measurement was imposed
	for less fluctuation.		

3. Once the altitude has been calculated, the PIC converts it to Morse Code and appropriately triggers an I/O pin
	that controls the 10-meter (~28.322 MHz) transmittter. Since altitude can be easily derived from the temperature
	and pressure measurements (stored in the EEPROM memory chip), the altitude measurements are not stored in memory.		
		
EOSS Flight Results
-------------------
During the entire flight, the BARC Jr. EOSS team was able to successfully receive altitude measurements from the 10-meter
beacon transmitter. Unfortunately, we received only thirty minutes of data storage compared to three hours. The causes
of this failure is still being investigated, but one possible reason is due to extreme temperatures.		

Please see the [EOSS 173-Recap](http://www.eoss.org/ansrecap/ar_200/recap173.htm) link for detailed information.

![Flight KML](http://www.eoss.org/eoss173/eoss173_k0scc-11_recap_GE.gif)