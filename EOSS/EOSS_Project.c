/************************************************************************
*
* Module:			EOSS_Project.C
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

#define		MS_PER_TICK						120
#define		INTERRUPT_CLOCK_SETTING			5536			// See doc/timer_math.markdown
#define		MS_PER_CALLSIGN					10 * 60 * 1000	//10 minutes between callsigns, 60 seconds in a minute, 1000 milliseconds in a second.
#define		TICKS_PER_CALLSIGN				5000			//MS_PER_CALLSIGN/MS_PER_TICK
#define		PREFIX							10
#define		SUFFIX							11
#define		TERMINATOR						0xFF
#define		DATA_BYTES_PER_LINE				4

#define		CALLSIGN_SLOW_FACTOR			3
#define		ALTITUDE_SLOW_FACTOR			3

#define		FOSC		8000000
#define		BAUD 		9600

#define		TRUE		1
#define		FALSE		0

//** Include Files ******************************************************

#include <p18f4520.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <timers.h>
#include <math.h>		// Required for altitude conversion

// Custom libraries
#include "EOSS_Project.h"
#include "../BMP085/BMP085.c"		// IMPORTANT - Must include functions for BMP085 sensor!
#include "morse.c"


#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON


//** Globals Variables **************************************************
unsigned char blockMorse[8];

// Timing stuff, all measured in ticks
unsigned short timeSinceCallsign = TICKS_PER_CALLSIGN + 1;	// This is so that the PIC transmits the callsign as soon as
															// it boots.
unsigned char nextReadingTime = 0;


/************************************************************************
*
* Purpose:		Transmits characters
* Passed:		TXIF, TXREG
* Returned:			None
*
* Date:			Author:				Comments:
* 20 Sep 2011	Austin Schaller		Created
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
	OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_4);
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
	if(INTCONbits.TMR0IF) {
		WriteTimer0(INTERRUPT_CLOCK_SETTING);
		timeSinceCallsign++;
		INTCONbits.TMR0IF = 0; // Clear the interrupt
		stepMorse();
		
		//printf((const far rom char*) "INTERRUPT\r\n");
		//MORSE_PIN = ~MORSE_PIN;
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
	if(result <= maxDistTemp)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
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
	unsigned char length;
	unsigned char firstRun = TRUE;
	
	OSCCONbits.IRCF0=1;
	OSCCONbits.IRCF1=1;
	OSCCONbits.IRCF2=1;
	while(!OSCCONbits.IOFS);
	TRISA = 0x00;
	
	// Initialize I2C
	OpenI2C(MASTER, SLEW_OFF);
	
	// Initialize BMP085
	BMP085_Calibration();
	
	// Intialize SPI
	openTxUsart();
	
	
	// Erase Schedule
	for(i = 0; i < 32; i++)
	{
		schedule[i] = 0;
	}
	
	printf((const far rom char*) "\r\n=========================\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========================\r\n");
	
	// Initialize Timer Interrupt
	activateInterrupt();					// Set up the timer
	WriteTimer0(INTERRUPT_CLOCK_SETTING);	// Set the timer
	timeSinceCallsign = 0; //TICKS_PER_CALLSIGN + 1;
	
	while(1)
	{
		if((timeSinceCallsign & 0x0F) == 0)
		{
			//printf((const far rom char*) "timeSinceCallsign: %d\r\n", timeSinceCallsign);
		}
		//printf((const far rom char*) "Loop\r\n");
		//printf((const far rom char*) "txPos: %d\r\n", txPos);
		//printf((const far rom char*) "writePos: %d\r\n", writePos);
		
		if(checkNear(txPos, writePos, \
			slowTimeLeft ? 1 : 4)) // Make sure you always have three seconds of morse left.
		{
			
			printf((const far rom char*) "Getting more morse\r\n");
			// Temperature & Pressure Measurements
			bmp085Convert(&temperature, &pressure);
			
			// Altitude Measurement
			temporary = (double) pressure / 101325;
			temporary = 1 - pow(temporary, 0.19029);
			
			// Will only work if temporary is positive.
			altitude = floor((44330 * temporary) + 0.5);
			printf((const far rom char*) "altitude: %d\r\n", altitude);
			
			formatAltitude(altitude, &blockMorse[0]);
			length = getLengthOfMorse(&blockMorse[0]);
			
			if((timeSinceCallsign + length + 25) > TICKS_PER_CALLSIGN)
			{
				// Make sure that we set the timing variables correctly
				nextReadingTime = nextReadingTime - (timeSinceCallsign & 0xFF);
				timeSinceCallsign = 0;
				
				if(firstRun)
				{
					nextReadingTime = 0;
					firstRun = FALSE;
				}
				
				//Bug: this starts the slow part ahead of time, meaning some of the morse is caught by it.
				
				// Transmit call sign
				txCallsign();
				printf((const far rom char*) "slowTimeLeft: %d\r\n", slowTimeLeft);
			}
			else
			{
				scheduleMorse(&blockMorse[0]);
			}
		}
		
		if((timeSinceCallsign & 0xFF) == nextReadingTime)
		{
			//printf((const far rom char*) "Getting a reading\r\n");
			//bmp085Convert(&temperature, &pressure);
			// Store temperature, pressure in I2C Memory
		}
		
		while(MORSE_PIN)
		{
			SPEAKER_PIN = ~SPEAKER_PIN;
			Delay100TCYx(20);
		}	
	}
}
