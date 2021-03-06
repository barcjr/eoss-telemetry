
/************************************************************************
*
* Purpose:      	Delays ms milliseconds
* Passed:       	Void
* Returned:			Void
* Note:
*
* Date:				Author:				Comments:
* 4 Aug 2011		Nick ODell     		Created
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
* Purpose:      	Calibrates the BMP085 pressure sensor.
* Passed:       	Void
* Returned:     	Void
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller     Created
*
************************************************************************/
void BMP085_Calibration(void)
{
	ac1 = bmp085ReadShort(0xAA);
	ac2 = bmp085ReadShort(0xAC);
	ac3 = bmp085ReadShort(0xAE);
	ac4 = bmp085ReadShort(0xB0);
	ac5 = bmp085ReadShort(0xB2);
	ac6 = bmp085ReadShort(0xB4);
	b1 = bmp085ReadShort(0xB6);
	b2 = bmp085ReadShort(0xB8);
	mb = bmp085ReadShort(0xBA);
	mc = bmp085ReadShort(0xBC);
	md = bmp085ReadShort(0xBE);

	/*printf("\tAC1 = %d\r\n", ac1);
	printf("\tAC2 = %d\r\n", ac2);
	printf("\tAC3 = %d\r\n", ac3);
	printf("\tAC4 = %d\r\n", ac4);
	printf("\tAC5 = %d\r\n", ac5);
	printf("\tAC6 = %d\r\n", ac6);
	printf("\tB1 = %d\r\n", b1);
	printf("\tB2 = %d\r\n", b2);
	printf("\tMB = %d\r\n", mb);
	printf("\tMC = %d\r\n", mc);
	printf("\tMD = %d\r\n", md);
	printf("------------------------\r\n");*/
}

/************************************************************************
*
* Purpose:      	Will read two sequential 8-bit registers, and return
					a 16-bit value.
* Passed:       	Unsigned char
* Returned:     	Short
* Note:
*
* Date:				Author:					Comments:
* 16 Mar 2011		Austin Schaller     	Created
*
************************************************************************/
long bmp085ReadShort(unsigned char address)
{
	unsigned short msb, lsb;
	unsigned short data;
	
	StartI2C();
	
	WriteI2C(BMP085_W);		// Write 0xEE
	WriteI2C(address);		// Write register address
	
	RestartI2C();
	
	WriteI2C(BMP085_R);		// Write 0xEF
	
	msb = ReadI2C();		// Get MSB result
	//printf("msb: %d\r\n", msb);
	AckI2C();
	lsb = ReadI2C();		// Get LSB result
	//printf("lsb: %d\r\n", lsb);

	NotAckI2C();
	StopI2C();
	
	/*data = msb * 256;
	data |= lsb;*/
	data = msb;
	data *= 256;
	data |= lsb;

	//printf("data: %d\r\n", data);
	return data;
}

/************************************************************************
*
* Purpose:      	Will read the 16-bit temperature value of BMP085 sensor.
* Passed:       	Void
* Returned:     	Long
* Note:
*
* Date:				Author:					Comments:
* 16 Mar 2011		Austin Schaller     	Created
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
	
	return bmp085ReadShort(0xF6);
}

/************************************************************************
*
* Purpose:      	Will read the 16-bit pressure value from BMP085 sensor.
* Passed:       	Void
* Returned:     	Long
* Note:
*
* Date:				Author:				Comments:
* 16 Mar 2011		Austin Schaller     Created
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
	
	return bmp085ReadShort(0xF6);
}

/************************************************************************
*
* Purpose:      	Will read the 16-bit pressure value from BMP085 sensor.
* Passed:       	Long *temperature, long *pressure
* Returned:     	Void
* Note:
*
* Date:				Author:					Comments:
* 16 Mar 2011		Austin Schaller     	Created
*
************************************************************************/
void bmp085Convert(long *temperature, long *pressure)
{
	long ut;
	long up;
	long x1, x2, b5, b6, x3, b3, p;
	unsigned long b4, b7;
	
	// Temperature before pressure
	ut = bmp085ReadTemp();
	up = bmp085ReadPressure();
	
	x1 = ((long) ut - ac6) * ac5 >> 15;
	x2 = ((long) mc << 11) / (x1 + md);
	b5 = x1 + x2;
	
	*temperature = (b5 + 8) >> 4;
	
	b6 = b5 - 4000;
	x1 = (b2 * (b6 * b6 >> 12)) >> 11;
	x2 = ac2 * b6 >> 11;
	x3 = x1 + x2;
	b3 = (((long) ac1 * 4 + x3) + 2) << 2;
	x1 = ac3 * b6 >> 13;
	x2 = (b1 * (b6 * b6 >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (ac4 * (unsigned long) (x3 + 32768)) >> 15;
	b7 = ((unsigned long) up - b3) * (50000 >> OSS);
	p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	
	*pressure = p + ((x1 + x2 + 3791) >> 4);
	
	delay_ms(10);
}
