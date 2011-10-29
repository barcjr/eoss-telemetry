#include				<p18f4520.h>
#include				<stdio.h>
#include				<i2c.h>
#include				<delays.h>
#include				"EEPROM.c"

#pragma config OSC=INTIO67, WDT=OFF, LVP=OFF, DEBUG=ON


//** Defines ************************************************************
#define		CONTROL_BASE		0xa0
#define		CHIP_NUM			0				//Should be 0-3
#define		BLOCK_SELECT		0				//More of a placeholder than anything.

#define		LED					LATAbits.LATA0

/************************************************************************
*
* Purpose:		Configures USART module for TX operation
* Passed:		None
* Returned:	 	SPBRG, TXSTA, RCSTA
* Note:			Asynchronous Mode
*
************************************************************************/
void openTxUsart(void)
{
		// TX UART Configuration
		
		SPBRG = 12;					// Set baud = 9600
		TXSTAbits.SYNC = 0;			// Asynchronous mode
		RCSTAbits.SPEN = 1;			// Serial port enabled
		TXSTAbits.BRGH = 0;			// Low Speed
		TXSTAbits.TXEN = 1;			// Enable transmission 
}

/************************************************************************
*
* Purpose:		Goes through device memory, checking to see if there's a
*				byte NOT EQUAL to needle. If so, print a warning.
* Returned:	 	0 if good, -1 if read error, -2 if byte is not what we expected.
*
************************************************************************/
#define BLOCK_SIZE 32
	
signed char EEPROM_memcheck(unsigned char device, unsigned char needle)
{
	signed char err;
	unsigned long address;
	unsigned char temp[BLOCK_SIZE];
	unsigned char i;
	
	
	printf((const far rom char*) "Full memory check start of device %02x.\r\n", device);
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		// Junk to make sure it fails if RandomRead doesn't actually
		// set this for some reason.
		temp[i] = 0x48 ^ i;
	}
	
	for(address = 0x0000; address < 0x20000; address += BLOCK_SIZE)
	{
		err = EERandomRead_mod(device, address, &temp[0], BLOCK_SIZE);
		//printf((const far rom char*) "blah\r\n");
		//printf((const far rom char*) "ERROR in EERandomRead_mod: %d\r\n", err);
		if(err)
		{
			printf((const far rom char*) "ERROR in EERandomRead_mod: %d\r\n", err);
			return -1;
		}
		for(i = 0; i < BLOCK_SIZE; i++)
		{
			if(temp[i] != needle)
			{
				printf((const far rom char*) "ERROR: Byte %lx is %02x rather than expected %02x\r\n", address, temp[i], needle);
				return -2;
			}
			//printf(".");
		}
	}
	
	printf((const far rom char*) "Full memory check successful.\r\n");
	return 0;
}

/************************************************************************
*
* Purpose:		Goes through device memory, writing needle.
* Returned:	 	0 if good, -1 if read error.
*
************************************************************************/
signed char EEPROM_memwrite(unsigned char device, unsigned char needle)
{
	signed char err;
	unsigned long address;
	unsigned char temp_write[BLOCK_SIZE];
	unsigned char i;
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		temp_write[i] = needle;
	}
	
	printf((const far rom char*) "Full memory write start of device %02x.\r\n", device);
	
	for(address = 0x0000; address < 0x20000; address += BLOCK_SIZE)
	{
		err = EEByteWrite_mod(device, address, &temp_write[0], BLOCK_SIZE);
		//printf((const far rom char*) "ERROR in EEByteWrite_mod: %d\r\n", err);
		if(err)
		{
			printf((const far rom char*) "ERROR in EEByteWrite_mod: %d\r\n", err);
			return -1;
		}
		//printf((const far rom char*) ".");
	}
	printf((const far rom char*) "Full memory write successful.\r\n");
	return 0;
}

/************************************************************************
*
* Purpose:		Goes through device memory, dumping it to UART
* Returned:	 	0 if good, -1 if read error.
*
************************************************************************/
signed char EEPROM_memdump(unsigned char device)
{
	signed char err;
	unsigned long address;
	unsigned char temp[BLOCK_SIZE];
	unsigned char i;
	
	
	printf((const far rom char*) "Full memory dump start of device %02x.\r\n", device);
	
	for(address = 0x0000; address < 0x20000; address += BLOCK_SIZE)
	{
		err = EERandomRead_mod(device, address, &temp[0], BLOCK_SIZE);
		//printf((const far rom char*) "blah\r\n");
		//printf((const far rom char*) "ERROR in EERandomRead_mod: %d\r\n", err);
		if(err)
		{
			printf((const far rom char*) "ERROR in EERandomRead_mod: %d\r\n", err);
			return -1;
		}
		for(i = 0; i < BLOCK_SIZE; i++)
		{
			printf((const far rom char*) "%02x", temp[i]);
			//printf(".");
		}
	}
	
	printf((const far rom char*) "\r\nFull memory dump successful.\r\n");
	return 0;
}

/*** Notes *************************************************************
*
* Memory is sub-divided into 512K blocks. The first block is 0x0000 to 
* 0xFFFF and the second block is 0x10000 to 0x1FFFF.
*
***********************************************************************/
void main()
{
		// Variables
		int i, counter, ack;
		unsigned char control;
		unsigned char read_dest[32];
		unsigned char write_src[32];
		
		OSCCON = 0x70;			// 8 MHz OSC
		while(!OSCCONbits.IOFS);
		
		TRISAbits.TRISA0 = 0;			// LED
		
		// I2C
		TRISCbits.TRISC3 = 1;			// SCL
		TRISCbits.TRISC4 = 1;			// SDA
		
		// UART
		TRISCbits.TRISC6 = 1;			// TX
		TRISCbits.TRISC7 = 1;			// RX
		
		OpenI2C(MASTER, SLEW_ON);
		SSPADD = 4;          // rate = Fosc/4*(SSPADD + 1)       rate = 8 MHz/20 = 400 kHz
		//SSPADD = 1;        // SSPADD = Fosc/(4*rate) - 1       if rate = 1 MHz, 8/4 - 1 = 1
		//SSPADD = 19;                     // 100 kHz SSPADD = 8 MHz/(4*100kHz) - 1 = 19
		//SSPADD = 199;        // 10 kHz
		
		openTxUsart();
		
		printf((const far rom char*) "=========================\r\n");
		printf((const far rom char*) "=========RESTART=========\r\n");
		printf((const far rom char*) "=========RESTART=========\r\n");
		printf((const far rom char*) "=========RESTART=========\r\n");
		printf((const far rom char*) "=========================\r\n");
			
		Delay10KTCYx(100);
		

		
 		
		printf((const far rom char*) "MASTER: %02x, SSPADD: %d\r\n", MASTER, SSPADD);
		
		control = CONTROL_BASE;
		//EEPROM_memwrite(control, 0xFF);
		//EEPROM_memcheck(control, 0xFF);
		//EEPROM_memdump(control);
		
		while(1);
}