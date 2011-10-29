/************************************************************************
*
* Module:			EOSS_Project.c
* Description:		Edge Of Space Science (EOSS) Project contributed by
*					BARC Jr.
* Line length:		120 characters [only if the length is longer than 80 chars]
* Functions:		See Below
*
* Date:				Authors:			Comments:
* 2 Jul 2011		Austin Schaller		Created
*			Nick ODell
*
************************************************************************/



/** Defines ************************************************************/

#define		MORSE_PIN						LATAbits.LATA0
#define		SPEAKER_PIN						LATAbits.LATA1

#define		MS_PER_TICK						60
#define		INTERRUPT_CLOCK_SETTING			5536			// See doc/timer_math.markdown
#define		MS_PER_CALLSIGN					10 * 60 * 1000	//10 minutes between callsigns, 60 seconds in a minute, 1000 milliseconds in a second.
#define		TICKS_PER_CALLSIGN				10000			//MS_PER_CALLSIGN/MS_PER_TICK

#define		FOSC							8000000
#define		BAUD 							9600

#define		TRUE							1
#define		FALSE							0

#define		ERROR_HANDLING

#define		EEPROM_CONTROL					0xA0

// Stuff that flag gets set to.
#define STARTED			0b00000001
#define OUT_OF_SPACE	0b00000010
#define EEPROM_ERROR	0b00000100
#define BMP_ERROR		0b00001000		

//** Include Files ******************************************************

#include <p18f4520.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <timers.h>
#include <math.h>		// Required for altitude conversion

#define MAX_EEPROM_SIZE 0x20000

#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON


//** Globals Variables **************************************************
unsigned char blockMorse[8];
unsigned char eepromBuffer[16];
unsigned char eepromBufferIndex = 0;

unsigned long eepromAddr = 0x00000;
unsigned char flag;


// Timing stuff, all measured in ticks
unsigned short timeSinceCallsign;
unsigned char nextReadingTime = 0;

// Include our stuff after the variables in case it needs them.
#include "EOSS_Project.h"
#include "../BMP085/BMP085.c"
#include "../EEPROM/EEPROM.c"
#include "morse.c"

/************************************************************************
*
* Purpose:		Configures USART module for TX operation
* Passed:		SPBRG, TXSTA, RCSTA
* Returned:		None
* Note:			Asynchronous Mode
*
************************************************************************/
void openTxUsart(void)
{
	// TX UART Configuration
	
	SPBRG = 12;				// Set baud = 9600
	TXSTAbits.SYNC = 0;		// Asynchronous mode
	RCSTAbits.SPEN = 1;		// Serial port enabled
	TXSTAbits.BRGH = 0;		// Low Speed
	TXSTAbits.TXEN = 1;		// Enable transmission
}

/************************************************************************
*
* Purpose:		Transmits characters
* Passed:		TXIF, TXREG
* Returned:		None
*
************************************************************************/
void txUsart(const rom char *data)
{
	char t;
	while(t = *data++)
	{
		while(!PIR1bits.TXIF);		// Ready for new character?
		TXREG = t;		// Send character
	}
}

/************************************************************************
*
* Purpose:		Set up timer0
* Passed:		None
* Returned:		None
*
************************************************************************/
void activateInterrupt(void)
{
	OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_2);
	INTCONbits.GIEH = 1; //enable interrupts
}

/************************************************************************
*
* Purpose:		Called during interrupt; resets timer and increments
* 				timeSinceCallsign.
* Passed:		None
* Returned:		None
*
************************************************************************/
#pragma code onInterrupt = 0x08
#pragma interrupt onInterrupt
void onInterrupt(void)
{

	if(INTCONbits.TMR0IF)
	{
		WriteTimer0(INTERRUPT_CLOCK_SETTING);
		timeSinceCallsign++;
		stepMorse();
		SPEAKER_PIN = MORSE_PIN;
		//printf("%c", MORSE_PIN ? '.' : ' ');
		INTCONbits.TMR0IF = 0; // Clear the interrupt
	}
}

/************************************************************************
*
* Purpose:		Checks whether one and two are within maxDist of each other.
* Passed:		None
* Returned:		None
*
************************************************************************/
unsigned char checkNear(unsigned char one, unsigned char two, unsigned char maxDist)
{
	signed short result;
	signed short maxDistTemp = maxDist;
	
	result = one;
	result -= two;
	//printf((const far rom char*) "result: %d\r\n", result);
	if(result < 0)
	{
		result = -result;
	}
	if(result > 256 - maxDist)
	{
		return TRUE;
	}	
	if(result <= maxDistTemp)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#define INSERT_IN_EEPROM_BUFFER(x) \
eepromBuffer[eepromBufferIndex++] = (x);
void takeReading()
{
	unsigned long tempRaw, pressRaw;
	unsigned char i, err;
	
	
	if(eepromAddr >= MAX_EEPROM_SIZE)
	{
		printf((const far rom char*) "Out of space\r\n");
		return;
	}	
	printf((const far rom char*) "Getting a reading\r\n");
	
	tempRaw = bmp085ReadTemp();
	pressRaw = bmp085ReadPressure();
	
	printf((const far rom char*) "pressRaw %lx tempRaw %lx\r\n", pressRaw, tempRaw);
	
	
	// Insert pressure, low byte first.
	INSERT_IN_EEPROM_BUFFER((pressRaw >> 16) & 0xFF);
	INSERT_IN_EEPROM_BUFFER((pressRaw >>  8) & 0xFF);
	INSERT_IN_EEPROM_BUFFER(pressRaw & 0xFF);
	
	// Insert temperature, low byte first.
	INSERT_IN_EEPROM_BUFFER((tempRaw >> 8)  & 0xFF);
	INSERT_IN_EEPROM_BUFFER(tempRaw & 0xFF);
	
	
	if(eepromBufferIndex < 15)
	{
		// Do nothing
	}
	else
	{
		// Buffer is almost full. Add the flag byte and send it out
		eepromBuffer[15] = flag;
		flag = 0;
		printf((const far rom char*) "Writing to eeprom address 0x%lx\r\n", eepromAddr);
		for(i = 0; i < 16; i++)
		{
			printf((const far rom char*) "%02x ", eepromBuffer[i]);
		
		}
		printf((const far rom char*) "\r\n");
			
		err = EEByteWrite_mod(EEPROM_CONTROL, eepromAddr, &eepromBuffer[0], 16);
		if(err)
		{
			flag |= EEPROM_ERROR;
		}
		eepromAddr += 16;
		eepromBufferIndex = 0;
	}
	nextReadingTime += 50;
}
// Tramples on eepromBuffer
void findUnusedAddress() {
	unsigned char i, empty;
	while(TRUE)
	{
		EERandomRead_mod(EEPROM_CONTROL, eepromAddr, &eepromBuffer[0], 16);
		if(eepromAddr >= MAX_EEPROM_SIZE)
		{
			// Give up
			printf((const far rom char*) "Out of space\r\n");
			return;
		}
		for(i = 0; i < 16; i++)
		{
			empty = TRUE;
			if(eepromBuffer[i] != 0xFF)
			{
				// This part isn't empty!
				empty = FALSE;
				break;
			}
			
		}
		if(empty)
		{
			
			printf((const far rom char*) "Found empty at: %lx\r\n", eepromAddr);
			// Found an empty part, we're done here.
			return;
		} else {
			// Try the next part
			eepromAddr += 16;
		}
	}
}

/** Main Loop **********************************************************/
void main()
{
	int i, j;
	long temperature = 0;
	long pressure = 0;
	signed short altitude = 0;
	double temporary = 0;
	unsigned short length;
	unsigned char firstRun = TRUE;
	
	OSCCONbits.IRCF0=1;
	OSCCONbits.IRCF1=1;
	OSCCONbits.IRCF2=1;
	while(!OSCCONbits.IOFS);
	TRISA = 0x00;
	
	// Initialize I2C
	OpenI2C(MASTER, SLEW_OFF);
	SSPADD = 4;
	
	// Initialize BMP085
	BMP085_Calibration();
	
	// Intialize UART
	openTxUsart();
	
	// Erase Schedule
	for(i = 0; i < 32; i++)
	{
		schedule[i] = 0;
	}
	
	flag = 0;
	flag |= STARTED;
	
	printf((const far rom char*) "\r\n=========================\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========================\r\n");
	
	// Find empty patch of EEPROM
	findUnusedAddress();
	
	// Initialize Timer Interrupt
	activateInterrupt();					// Set up the timer
	WriteTimer0(INTERRUPT_CLOCK_SETTING);	// Set the timer
	timeSinceCallsign = TICKS_PER_CALLSIGN + 1;
	
	while(1)
	{
		bmp085Convert(&temperature, &pressure);
		if((timeSinceCallsign & 0x0F) == 0)
		{
			//printf((const far rom char*) "timeSinceCallsign: %d\r\n", timeSinceCallsign);
		}
		//printf((const far rom char*) "Loop\r\n");
		//printf((const far rom char*) "txPos: %d\r\n", txPos);
		//printf((const far rom char*) "writePos: %d\r\n", writePos);
		
		if(checkNear(txPos, writePos, 5)) // Make sure you always have morse left.
		{
			
			//printf((const far rom char*) "Getting more morse\r\n");
			// Temperature & Pressure Measurements
			bmp085Convert(&temperature, &pressure);
			
			// Altitude Measurement
			temporary = (double) pressure / 101325;
			temporary = 1 - pow(temporary, 0.19029);
			
			// Will only work if temporary is positive.
			altitude = floor((44330 * temporary) + 0.5);
			//printf((const far rom char*) "altitude: %d\r\n", altitude);
			
			formatAltitude(altitude, &blockMorse[0]);
			length = getLengthOfMorse(&blockMorse[0]);
			
			if((timeSinceCallsign + length * ALTITUDE_SLOW_FACTOR) > TICKS_PER_CALLSIGN)
			{
				// Make sure that we set the timing variables correctly
				nextReadingTime = nextReadingTime - (timeSinceCallsign & 0xFF);
				timeSinceCallsign = 0;
				
				if(firstRun)
				{
					// If we just booted, we want to take a reading first thing.
					nextReadingTime = 0;
					firstRun = FALSE;
				}
				
				// Transmit call sign
				txCallsign();
				printf((const far rom char*) "callsignTimeLeft: %d\r\n", callsignTimeLeft);
			}
			else
			{
				scheduleMorse(&blockMorse[0]);
			}
		}
		
		if(checkNear((unsigned char)(timeSinceCallsign & 0xFF), nextReadingTime, 5))
		{
			// Store temperature, pressure in I2C Memory
			takeReading();
		}
	}
}
