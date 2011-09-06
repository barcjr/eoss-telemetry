/************************************************************************
*
* Module:       	EOSS_Project.C
* Description:  	Edge Of Space Science (EOSS) Project contributed by
*					BARC Jr.
* Line length:  	120 characters [only if the length is longer than 80 chars]
* Functions:    	See Below
*
* Date:			Authors:			Comments:
* 2 Jul 2011   		Austin Schaller    		Created
*			Nick ODell
*
************************************************************************/



/** Defines ************************************************************/

#define		MORSE_PIN                     	0
#define     MS_PER_TICK						120
#define     INTERRUPT_CLOCK_SETTING			64598		/*	(65536 - 64598) * 128E-6 = 120mS 
															(Prescaler = 1:256)
															Change the 64598 if you change the clock speed.	*/
#define     TICKS_PER_CALLSIGN				5000		//(10 * 60 * 1000)/MS_PER_TICK
#define		PREFIX							16
#define		SUFFIX							17
#define		TERMINATOR						0xFF

#define		FOSC		8000000
#define		BAUD 		9600

#define 	sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define 	cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

//** Include Files ******************************************************

#include <p18f4520.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <timers.h>
#include <math.h>		// Required for altitude measurement
#include "EOSS_Project.h"
#include "../BMP085/BMP085.c"		// IMPORTANT - Must include functions for BMP085 sensor!


#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON


//** Globals Variables **************************************************

// Last byte is the length
// These are automatically generated by a python script.
rom const unsigned char MorseCodeLib[22][4] = 
{
	{0x77, 0x77, 0x07, 0x15},  // 0
	{0xDD, 0xDD, 0x01, 0x13},  // 1
	{0x75, 0x77, 0x00, 0x11},  // 2
	{0xD5, 0x1D, 0x00, 0x0F},  // 3
	{0x55, 0x07, 0x00, 0x0D},  // 4
	{0x55, 0x01, 0x00, 0x0B},  // 5
	{0x57, 0x05, 0x00, 0x0D},  // 6
	{0x77, 0x15, 0x00, 0x0F},  // 7
	{0x77, 0x57, 0x00, 0x11},  // 8
	{0x77, 0x77, 0x01, 0x13},  // 9
	{0x1D, 0x00, 0x00, 0x07},  // A
	{0x57, 0x01, 0x00, 0x0B},  // B
	{0xD7, 0x05, 0x00, 0x0D},  // C
	{0x57, 0x00, 0x00, 0x09},  // D
	{0x01, 0x00, 0x00, 0x03},  // E
	{0x75, 0x01, 0x00, 0x0B},  // F
	{0x9D, 0xAE, 0x1C, 0x18},  // Prefix
	{0x55, 0xEE, 0x00, 0x13},  // Suffix
	{0xDD, 0xB9, 0xBB, 0x18},  // Callsign 1
	{0x3B, 0x57, 0xAE, 0x18},  // Callsign 2
	{0x73, 0x75, 0xB9, 0x18},  // Callsign 3
	{0x0A, 0x00, 0x00, 0x07}   // Callsign 4
};

// Timing stuff, all measured in ticks
long timeSinceCallsign = TICKS_PER_CALLSIGN + 1;	// This is so that the PIC transmits the callsign as soon as 
													// it boots.
unsigned char nextMorseTime = 0;
unsigned char nextReadingTime = 0;

// Schedule for morse code procedure
unsigned char schedule[32];
unsigned char txPos;
unsigned char writePos;
unsigned char slowTimeLeft;		// Transmit 25 times slower (i.e. 3 second element length) if > 0. Measured in ticks

/************************************************************************
*
* Purpose:      To look at the schedule and find the scheduled bit.
* Passed:       0-255 depending on which bit you want to look at from the
*				MorseCodeLib.
* Returned:     0-1 depending on the state of the bit.
* Note:			
* Date:		Author:			Comments:
* 16 Apr 2011	Nick ODell		Created
*
************************************************************************/

/*
The top 5 bits specify the byte and the lower 3 specify the bit. This
results in a more convinient representation of the data in the schedule
array.
*/
unsigned char getBitFromSchedule(unsigned char bitPos)
{
	// Return the state of the bit retreived
	return (schedule[bitPos >> 3] >> (bitPos & 0x07)) & 0x01;
}
	
/************************************************************************
*
* Purpose:      Called when the main loop deems it time to tranmit the next segment.
* Passed:       None
* Returned:     None
* Note:			
* Date:			Author:				Comments:
* 7  Mar 2011	Austin Schaller     Created
* 16 Apr 2011	Nick ODell          Gutted; renamed; replaced;
*
************************************************************************/
void stepMorse()
{
	unsigned char oneBit;
	
	nextMorseTime = nextMorseTime + (slowTimeLeft == 0 ? 1 : 0x19);		// Move the trigger for the timer.
	
	oneBit = getBitFromSchedule(txPos);
	
	// TODO: Transmit on MORSE_PIN
	
	// If the slower transmit time is still enabled, decrement the timer.
	if(slowTimeLeft > 0)
	{
		slowTimeLeft--;
	}
	
	if(txPos & 0x07 == 0)
	{
		schedule[(txPos - 1) >> 3] = 0;		/*	Clear out the schedule behind you, so that bits from 256 ticks ago 
												don't come back to haunt you.	*/
	}
	txPos++;
}

/************************************************************************
*
* Purpose:      Pushes the items given onto the schedule queue.
* Passed:       Morse code encoded as what's shown in MorseCodeLib + 1. Terminate
* 				sequences with 0xFF.
*
* Returned:     None
*
* Date:		Author:			Comments:
* 16 Apr 2011	Nick O'Dell         	Created
*
************************************************************************/
void scheduleMorse(unsigned char *morse)
{
	unsigned char i;
	unsigned char index, txBit;
	
	
    while((index = *morse++) != TERMINATOR)
	{
		unsigned char length = MorseCodeLib[index][3];
		for(i = 0; i < length; i++)
		{
			/*
			We get the bit from the morse code library, then OR it with
			the byte already in the schedule. Result: We write one element
			to the schedule.
			*/
			   
			txBit = (MorseCodeLib[index][i >> 3] >> (i & 0x07)) & 0x01;
			schedule[writePos >> 3] |= txBit << (writePos & 0x07);
			writePos++;
		}
	}
}

/************************************************************************
*
* Purpose:      Pushes the callsign onto the schedule queue. This function
* expects that txPos == writePos
* Passed:       None
* Returned:     None
*
* Date:		Author:			Comments:
* 16 Apr 2011	Nick O'Dell		Created
*
************************************************************************/
void txCallSign()
{
	unsigned char morse[] = {18, 19, 20, 21, TERMINATOR};
	unsigned char length = getLengthOfMorse(&morse[0]);
	
	slowTimeLeft = length;
	scheduleMorse(&morse[0]);
}

/************************************************************************
*
* Purpose:      Configures USART module for TX operation
* Passed:       SPBRG, TXSTA, RCSTA
* Returned:     None
* Note:			Asynchronous Mode
*
* Date:		Author:			Comments:
* 20 Sep 2011	Austin Schaller     	Created
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
* Purpose:      Changes altitude into the morse library sequence 
* Passed:       Altitude
* Returned:     Pointer to array
*
* Date:		Author:			Comments:
* 16 Apr 2011	Nick O'Dell         	Created
*
************************************************************************/
unsigned char *formatAltitude(signed short alt)
{
	signed char i;
	unsigned char morse[7];
	unsigned char leading_zero;
	unsigned char number_of_array_items = 0;
	unsigned char *pointer;
	unsigned char number;
	
	morse[0] = PREFIX;
	number_of_array_items++;
	
	leading_zero = 1;  	// This should only be 1/0
	
	// This counts down from 3 to 0 because the most significant digit comes first.
	for(i = 3; i >= 0; i--)
	{
		/*
		Multiply i by 4, then right-shift alt that amount
		and OR it. This selects 4 bits at a time, allowing
		us to send hexadecimal.
		*/
		
		number = (alt >> (i << 2)) & 0x0F;
		
		/*
		If the other digits so far have been zeros, and this
		is a zero, and this isn't the last digit, don't output
		this digit.
		*/
		
		if(leading_zero == 1 && number == 0 && i != 0)
		{
			// Do nothing
		}
		else
		{
			morse[number_of_array_items] = number;		// morse[number_of_digits] = number; - DBUG
			number_of_array_items++;
			leading_zero = 0;    
		}
	}
	
	morse[number_of_array_items] = SUFFIX;			// The "H M" at the end of the sequence
	number_of_array_items++;
	
	morse[number_of_array_items] = TERMINATOR;		// Terminator
	number_of_array_items++;
	
	pointer = &morse[0];
	return pointer;
}

/************************************************************************
*
* Purpose:      Transmits characters
* Passed:       TXIF, TXREG
* Returned:     None
*
* Date:		Author:			Comments:
* 20 Sep 2011	Austin Schaller     	Created
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

/** Main Loop **********************************************************/
void main()
{
	int i;
	long temperature = 0;
	long pressure = 0;
	signed short altitude = 0;
	double temporary = 0;
	unsigned char *morse;
	unsigned char length;
	
	OSCCONbits.IRCF0=1; 
	OSCCONbits.IRCF1=1;
	OSCCONbits.IRCF2=1;
	while(!OSCCONbits.IOFS);
	TRISA = 0x00;
	
	// Initialize I�C
	// Intialize SPI
	
	while(1)
	{
		if(txPos == writePos)
		{
			// Temperature & Pressure Measurements
			//bmp085Convert(&temperature, &pressure);
			
			// Altitude Measurement
			temporary = (double) pressure / 101325;
			temporary = 1 - pow(temporary, 0.19029);
			
			// Will only work if temporary is positive.
			altitude = floor((44330 * temporary) - 0.5);
			
			morse = formatAltitude(altitude);
			length = getLengthOfMorse(morse);
			
			if(timeSinceCallsign + length > TICKS_PER_CALLSIGN)
			{
				// Make sure that we set the timing variables correctly
				nextReadingTime = nextReadingTime - (timeSinceCallsign & 0xFF);
				nextMorseTime = nextMorseTime - (timeSinceCallsign & 0xFF);
				timeSinceCallsign = 0;
				
				// Transmit call sign
				txCallSign();
			}
			else
			{
				scheduleMorse(morse);
			}
		}
		
		if(timeSinceCallsign & 0xFF == nextMorseTime)
		{
			stepMorse();
		}
		
		if(timeSinceCallsign & 0xFF == nextReadingTime)
		{
			//bmp085Convert(&temperature, &pressure);
			// Store temperature, pressure in I�C Memory
		}
	}
}
