/************************************************************************
*
* Module:			BMP085.c
* Description:		BMP085-specific code.
* Line length:		120 characters [only if the length is longer than 80 chars]
*
* Date:				Authors:					Comments:
* 16 Jul 2011		Austin Schaller				Created
*					Nick ODell
*
************************************************************************/

#include "../BMP085/BMP085.h"

// * Defines ************************************************************
#define		BMP085_R	0xEF
#define		BMP085_W	0xEE
#define		OSS			3		// Oversampling Setting

// * Globals Variables **************************************************

const rom unsigned char OSS_conversion_time[] = {5, 8, 14, 26};

signed char BMP_err;

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
* Purpose:		Dumps calibration constants.
* Passed:		Void
* Returned:		Void
*
************************************************************************/
void BMP_dump_calibration(void)
{
	printf((const far rom char*) "%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n%ld\r\n,",\
				ac1, ac2, ac3, ac4, ac5, ac6, b1, b2, mb, mc, md);
}

/************************************************************************
*
* Purpose:		If this is being compiled as part of EOSS_Launch, 
* 				set the flag bit corresponding to a BMP error.
* 				Otherwise, do nothing.
* Passed:		Void
* Returned:		Void
*
************************************************************************/
void BMP_process_error()
{
#ifdef ERROR_HANDLING
	if(BMP_err != 0)
	{
		flag |= BMP_ERROR;
		printf((const far rom char*) "BMP_err: %d\r\n", BMP_err);
		BMP_err = 0;
	}
#endif
}

/************************************************************************
*
* Purpose:		Will read two sequential 8-bit registers, and return
				a 16-bit value.
* Passed:		Unsigned char
* Returned:		Short
* Note:			Return value must be typecast to an signed short if reading a signed value!
*
************************************************************************/
unsigned short bmp085ReadShort(unsigned char address)
{
	unsigned short msb, lsb;
	unsigned short data;
	StartI2C();
	
	BMP_err |= WriteI2C(BMP085_W);		// Write 0xEE
	BMP_err |= WriteI2C(address);		// Write register address

	RestartI2C();

	BMP_err |= WriteI2C(BMP085_R);		// Write 0xEF
	
	msb = ReadI2C();		// Get MSB result
	BMP_err |= AckI2C();
	lsb = ReadI2C();		// Get LSB result
	

	NotAckI2C();
	StopI2C();

	delay_ms(10);

	data = msb << 8;
	data |= lsb;
	
	BMP_process_error();

	return data;
}

/************************************************************************
*
* Purpose:		Will read three sequential 8-bit registers, and return
				a 24-bit value.
* Passed:		Unsigned char
* Returned:		Unsigned long
*
************************************************************************/
unsigned long bmp085ReadThreeBytes(unsigned char address)
{
	unsigned short msb, lsb, xlsb;
	unsigned long data;

	StartI2C();

	BMP_err |= WriteI2C(BMP085_W);		// Write 0xEE
	BMP_err |= WriteI2C(address);		// Write register address

	RestartI2C();

	BMP_err |= WriteI2C(BMP085_R);		// Write 0xEF

	msb = ReadI2C();		// Get MSB result
	AckI2C();
	lsb = ReadI2C();		// Get LSB result

	NotAckI2C();
	StopI2C();

	delay_ms(10);

	data = msb;
	data <<= 8;
	data |= lsb;
	data <<= 8;
	data |= xlsb;
	
	BMP_process_error();

	return data;
}

/************************************************************************
*
* Purpose:		Will read the 16-bit temperature value of BMP085 sensor.
* Passed:		Void
* Returned:		Long
* Note:
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
* Purpose:		Will read the 16-bit pressure value from BMP085 sensor.
* Passed:		Void
* Returned:		Long
* Note:
*
************************************************************************/
long bmp085ReadPressure(void)
{
	StartI2C();

	WriteI2C(BMP085_W);		// Write 0xEE
	WriteI2C(0xF4);		// Write register address
	WriteI2C(0x34 | (OSS << 6));		// Write register data for temp
	
	StopI2C();
	
	delay_ms(OSS_conversion_time[OSS]);		// Max time is 4.5ms
	if(OSS) {
		return bmp085ReadThreeBytes(0xF6);
	} else {
		return ((long) bmp085ReadShort(0xF6)) << 8;
	}
}

/************************************************************************
*
* Purpose:		Will find callibrated pressure + temperature
* Passed:		Long *temperature, long *pressure, unsigned char readings
* Returned:		Void
* Note:
*
************************************************************************/

// The bit-shift for possibly negative numbers.
// Note that divide has a higher precedence than
// bit shift, so this is not precisely the same.
#define SHIFT(shift) (((long) 1) << (shift))

void bmp085Convert(long *temperature, long *pressure, unsigned char readings)
{
	long ut;
	long up;
	long x1, x2, b5, b6, x3, b3, p;
	unsigned long b4, b7;
	unsigned char i;
	
	ut = 0;
	up = 0;
	
	//Take many readings, average
	for(i = 0; i < readings; i++)
	{
		ut += bmp085ReadTemp();
		up += bmp085ReadPressure();
	}
	ut = ut / readings;
	up = up / readings;

	// For running the conversion algorithm against known datasheet values.
	//ut = 27898;
	//up = 23843;
	
	// Throw away the data that is essentially noise
	up = up >> (8-OSS);

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
