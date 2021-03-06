/************************************************************************
*
* Module:			EOSS_BMP085.c
* Description:		Code to test BMP085 functionality.
* Line length:		120 characters [only if the length is longer than 80 chars]
*
* Date:				Authors:					Comments:
* 16 Jul 2011		Austin Schaller				Created
*					Nick ODell
*
************************************************************************/

#include <p18f4520.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <math.h>		// Required for altitude measurement
#include "EOSS_Project.h"
#include "../BMP085/BMP085.c"		// IMPORTANT - Must include functions for BMP085 sensor!

#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON

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


void main()
{
	long temperature = 0;
	long pressure = 0;
	long altitude = 0;
	double temp = 0;

	OSCCON = 0x70;		// 8 MHz OSC
	
	// I2C
	TRISCbits.TRISC3 = 1;		// SCL
	TRISCbits.TRISC4 = 1;		// SDA
	
	// UART
	// Initialize UART
	openTxUsart();
	
	// Initialize I2C
	OpenI2C(MASTER, SLEW_OFF);
	
	
	
	delay_ms(100);
	
	
	BMP085_Calibration();
	BMP_dump_calibration();
	
	printf((const far rom char*) "=========================\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========RESTART=========\r\n");
	printf((const far rom char*) "=========================\r\n");
	
	while(1)
	{
		bmp085Convert(&temperature, &pressure);
		
		printf((const far rom char*)"Temperature: %ld (in 0.1 deg C)\r\n", temperature);
		printf((const far rom char*)"Pressure: %ld Pa\n\r\n", pressure);
		
		// For fun, lets convert to altitude
		temp = (double) pressure/101325;
		temp = 1-pow(temp, 0.19029);
		altitude = floor(44330*temp);
		printf((const far rom char*)"Altitude: %ldm\r\n", altitude);
		printf((const far rom char*) "=========================\r\n");
		
		delay_ms(10000);
	}
}