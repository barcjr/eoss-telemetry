#include "../BMP085/BMP085.h"

// * Defines ************************************************************
#define		BMP085_R	0xEF
#define		BMP085_W	0xEE
#define		OSS			0		// Oversampling Setting (note: code is not set up to use other OSS values)

// * Globals Variables **************************************************

// Voodoo calibration varibles
long ac1;
long ac2;
long ac3;
long b1;
long b2;
long mb;
long mc;
long md;
unsigned long ac4;
unsigned long ac5;
unsigned long ac6;

/************************************************************************
*
* Purpose:		Delays ms milliseconds
* Passed:		Void
* Returned:		Void
* Note:
*
<<<<<<< HEAD
* Date:				Author:				Comments:
* 4 Aug 2011		Nick ODell			Created
=======
* Date:			Authors:			Comments:
* 16 Jul 2011   	Austin Schaller    		Created
*			Nick ODell
>>>>>>> 7c9b0ab34e41d6924c7cd821a3e06f62c444c1f3
*
************************************************************************/
void delay_ms(unsigned short ms)
{
	for(; ms > 0; ms--)
	{
		Delay100TCYx(20);
	}
}

/************************************************************************
*
* Purpose:		Calibrates the BMP085 pressure sensor.
* Passed:		Void
* Returned:		Void
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
*
************************************************************************/
void BMP085_Calibration(void)
{
	ac1 = (signed short) bmp085ReadShort(0xAA);
	ac2 = (signed short) bmp085ReadShort(0xAC);
	ac3 = (signed short) bmp085ReadShort(0xAE);
	ac4 = bmp085ReadShort(0xB0);
	ac5 = bmp085ReadShort(0xB2);
	ac6 = bmp085ReadShort(0xB4);
	b1 = (signed short) bmp085ReadShort(0xB6);
	b2 = (signed short) bmp085ReadShort(0xB8);
	mb = (signed short) bmp085ReadShort(0xBA);
	mc = (signed short) bmp085ReadShort(0xBC);
	md = (signed short) bmp085ReadShort(0xBE);
}

/************************************************************************
*
* Purpose:		Calibrates the BMP085 pressure sensor with known stuff from
*				the datasheet. For debugging only.
* Passed:		Void
* Returned:		Void
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
*
************************************************************************/
void BMP085_Known_Calibration(void)
{
	ac1 = 408;
	ac2 = -72;
	ac3 = -14383;
	ac4 = 32741;
	ac5 = 32757;
	ac6 = 23153;
	b1 = 6190;
	b2 = 4;
	mb = -32768;
	mc = -8711;
	md = 2868;
}

/************************************************************************
*
* Purpose:		Will read two sequential 8-bit registers, and return
				a 16-bit value.
* Passed:		Unsigned char
* Returned:		Short
* Note:			Return value must be typecast to an signed short if reading a signed value!
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
*
************************************************************************/
unsigned short bmp085ReadShort(unsigned char address)
{
	unsigned short msb, lsb;
	unsigned short data;
	
	StartI2C();
	
	WriteI2C(BMP085_W);		// Write 0xEE
	WriteI2C(address);		// Write register address
	
	RestartI2C();
	
	WriteI2C(BMP085_R);		// Write 0xEF
	
	msb = ReadI2C();		// Get MSB result
	AckI2C();
	lsb = ReadI2C();		// Get LSB result
	
	NotAckI2C();
	StopI2C();
	
	delay_ms(10);
	
	data = msb << 8;
	data |= lsb;
	
	return data;
}

/************************************************************************
*
* Purpose:		Will read the 16-bit temperature value of BMP085 sensor.
* Passed:		Void
* Returned:		Long
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
*
************************************************************************/
long bmp085ReadTemp(void)
{
	StartI2C();
	
	WriteI2C(BMP085_W);		// Write 0xEE
	WriteI2C(0xF4);		// Write register address
	WriteI2C(0x2E);		// Write register data for temp
	
	StopI2C();
	
	delay_ms(10);		// Max time is 4.5ms
	
	return (signed short) bmp085ReadShort(0xF6);
}

/************************************************************************
*
<<<<<<< HEAD
* Purpose:		Will read the 16-bit pressure value from BMP085 sensor.
* Passed:		Void
* Returned:		Long
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
=======
* Purpose:      	Configures USART module for TX operation
* Passed:       	SPBRG, TXSTA, RCSTA
* Returned:     	None
* Note:			Asynchronous Mode
*
* Date:			Author:				Comments:
* 20 Sep 2011		Austin Schaller     		Created
>>>>>>> 7c9b0ab34e41d6924c7cd821a3e06f62c444c1f3
*
************************************************************************/
long bmp085ReadPressure(void)
{
	StartI2C();
	
	WriteI2C(BMP085_W);		// Write 0xEE
	WriteI2C(0xF4);		// Write register address
	WriteI2C(0x34);		// Write register data for temp
	
	StopI2C();
	
	delay_ms(10);		// Max time is 4.5ms
	
	return (unsigned long) bmp085ReadShort(0xF6);
}

/************************************************************************
*
* Purpose:		Will read the 16-bit pressure value from BMP085 sensor.
* Passed:		Long *temperature, long *pressure
* Returned:		Void
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller		Created
*
************************************************************************/

// The bit-shift for possibly negative numbers.
// Note that divide has a higher precedence than
// bit shift, so this is not precisely the same.
#define SHIFT(shift) (((long) 1) << (shift))

void bmp085Convert(long *temperature, long *pressure)
{
	long ut;
	long up;
	long x1, x2, b5, b6, x3, b3, p;
	unsigned long b4, b7;
	
	ut = bmp085ReadTemp();
	up = bmp085ReadPressure();
	
	// For running the conversion algorithm against known datasheet values.
	//ut = 27898;
	//up = 23843;
	
	
	// Very sorry about this pile of code
	x1 = (ut - ac6) * ac5 / SHIFT(15);
	x2 = (mc * SHIFT(11)) / (x1 + md);
	b5 = x1 + x2;
	*temperature = (b5 + 8) / SHIFT(4);
	
	b6 = b5 - 4000;
	x1 = (b2 * (b6 * b6 / SHIFT(12))) / SHIFT(11);
	x2 = ac2 * b6 / SHIFT(11);
	x3 = x1 + x2;
	
	b3 = ((((ac1 * 4) + x3) << OSS) + 2) / SHIFT(2);
	x1 = ac3 * b6 / SHIFT(13);
	x2 = (b1 * (b6 * b6 / SHIFT(12))) / SHIFT(16);
	x3 = ((x1 + x2) + 2) / SHIFT(2);
	b4 = (ac4 * (unsigned long) (x3 + 32768)) / SHIFT(15);
	b7 = ((unsigned long) up - b3) * (50000 >> OSS);
	p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
	
	x1 = p / SHIFT(8);
	x1 *= x1;
	x1 = (x1 * 3038) / SHIFT(16);
	x2 = (-7357 * p) / SHIFT(16);
	x2 = x2 / SHIFT(16);
	
	*pressure = p + (x1 + x2 + 3791) / SHIFT(4);
}
