
/************************************************************************
*
* Module:       	BMP085_1.C
* Description:  	Code to determine BMP085 functionality.
* Line length:  	120 characters [only if the length is longer than 80 chars]
* Functions:    	See Below
*
* Date:				Authors:					Comments:
* 21 Aug 2011   	Austin Schaller    			Created
*					Nick ODell
*
************************************************************************/

#include <p18f4520.h>
#include <delays.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
#include <math.h>		// Required for altitude measurement
#include "EOSS_Project_Functions.h"

#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON

//** Defines ************************************************************
#define 	BMP085_R 	0xEF
#define 	BMP085_W 	0xEE
#define 	OSS 		0		// Oversampling Setting (note: code is not set up to use other OSS values)


//** Globals Variables **************************************************

// Voodoo calibration varibles
short ac1;
short ac2;
short ac3;
short b1;
short b2;
short mb;
short mc;
short md;
unsigned short ac4;
unsigned short ac5;
unsigned short ac6;

// BMP stuff can be included now.
#include "EOSS_BMP085.c"		// IMPORTANT - Must include functions for BMP085 sensor!

/************************************************************************
*
* Purpose:      Configures USART module for TX operation
* Passed:       SPBRG, TXSTA, RCSTA
* Returned:     None
* Note:			Asynchronous Mode
*
* Date:			Author:				Comments:
* 20 Sep 2011	Austin Schaller     Created
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
	//long altitude = 0;
	//double temp = 0;

	OSCCON = 0x70;		// 8 MHz OSC
	
	// I2C
	TRISCbits.TRISC3 = 1;		// SCL
	TRISCbits.TRISC4 = 1;		// SDA
	
	// UART
	TRISCbits.TRISC6 = 1;		// TX
	TRISCbits.TRISC7 = 1;		// RX
	
	// Initialize UART
	openTxUsart();
	
	// Initialize I2C
	OpenI2C(MASTER, SLEW_OFF);
	
	BMP085_Calibration();
	
	delay_ms(10);
	
	
	while(1)
	{
		bmp085Convert(&temperature, &pressure);
		
		printf((const far rom char*)"Temperature: %d (in 0.1 deg C)\r\n", temperature);
		printf((const far rom char*)"Pressure: %d Pa\n\r\n", pressure);

		printf((const far rom char*)"\r\n\r\n");
		
		// For fun, lets convert to altitude
		/*temp = (double) pressure/101325;
		temp = 1-pow(temp, 0.19029);
		altitude = round(44330*temp);
		printf("Altitude: %ldm\n\n", altitude);*/
		
		delay_ms(3000);
	}
}