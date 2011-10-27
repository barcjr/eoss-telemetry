#include <p18cxxx.h>
#include <i2c.h>

/********************************************************************
*	Function Name:	EEByteWrite										*
*	Return Value:	error condition status							*
*	Parameters:		EE memory device control, address and data		*
*					bytes.											*
*	Description:	Something of a misnomer, write as many bytes as you want
*																	*
********************************************************************/
signed char EEByteWrite_mod(unsigned char control, unsigned long address, unsigned char *data_src, unsigned char len)
{
	
	unsigned char bytes[] = {control, (address >> 8) & 0xFF, address & 0xFF};
	unsigned char i;
	unsigned char blockSel;
	signed char err;
	
	//printf("EEByteWrite_mod\r\n");
	IdleI2C();							// ensure module is idle
	StartI2C();							// initiate START condition
	while(SSPCON2bits.SEN);
		//printf("+");			// wait until start condition is over
	if(PIR2bits.BCLIF)				 // test for bus collision
	{
		return -1;					// return with Bus Collision error 
	}
	
	blockSel = (address >> 16) & 0x1;	//Bit 16 is which block
	
	bytes[0] |= (blockSel << 1);        //Set block select bit in control byte
	
	for(i = 0; i < 3; i++)
	{
		//printf("Byte to write to I2C: %02x\r\n", bytes[i]);
		err = WriteI2C(bytes[i]);		// write byte - R/W bit should be 0
		//printf("done\r\n");
		if(err)
		{
			//printf("err: %d\r\n", err);
			StopI2C();
			return -3;				// set error for write collision
		}
		//printf("wait ack\r\n");
		if(SSPCON2bits.ACKSTAT)
		{
			//printf("ackstat\n");
			StopI2C();
			return -2;				// return with Not Ack error condition
		}
	}
	for(i = 0; i < len; i++)
	{
		//printf("Byte[%d] to write to I2C: %02x\r\n", i, data_src[i]);
		err = WriteI2C(data_src[i]);
		//printf("done\r\n");
		if(err)		// write byte - R/W bit should be 0
		{
			//printf("err: %d\r\n", err);
			StopI2C();
			return -3;				// set error for write collision
		}
		//printf("wait ack\r\n");;
		if(SSPCON2bits.ACKSTAT)
		{
			//printf("ackstat\n");
			StopI2C();
			return -2;				// return with Not Ack error condition
		}
		//data_src++;
	}
	//printf("done\n");

	//IdleI2C();						// ensure module is idle	
	StopI2C();							// send STOP condition
	while(SSPCON2bits.PEN);			// wait until stop condition is over 
	if(PIR2bits.BCLIF)				// test for bus collision
	{
		return -1;					// return with Bus Collision error 
	}
	
	Delay10KTCYx(10);
	return 0;						// return with no error
	
}

/********************************************************************
*	Function Name:	EEByteWrite										*
*	Return Value:	error condition status							*
*	Parameters:		EE memory device control, address and data		*
*					bytes.											*
*	Description:	Write single data byte to I2C EE memory			*
*					device. This routine can be used for any I2C	*
*					EE memory device, which only uses 2 byte of		*
*					address data as in the 24FC1026					*
*																	*
********************************************************************/
signed char EERandomRead_mod(unsigned char control, unsigned long address, unsigned char *data, unsigned char len)
{
	
	unsigned char bytes[] = {control, (address >> 8) & 0xFF, address & 0xFF};
	unsigned char i;
	unsigned char blockSel;
	
	signed char err;
	StartI2C();							// initiate START condition
	while(SSPCON2bits.SEN);
		//printf("+");			// wait until start condition is over 
	if(PIR2bits.BCLIF)				 // test for bus collision
	{
		return -1;					// return with Bus Collision error 
	}
	
	blockSel = (address >> 16) & 0x1;	//Bit 16 is which block
	
	bytes[0] |= (blockSel << 1);        //Set block select bit in control byte
	
	//printf("Reading from eprom\r\n");
	for(i = 0; i < 3; i++)
	{
		//printf("EERandomRead Byte to write to I2C: %02x\r\n", bytes[i]);
		err = WriteI2C(bytes[i]);
		//printf("done\r\n");
		if(err)
		{
			//printf("err: %d\r\n", err);
			StopI2C();
			return -3;				// set error for write collision
		}
		//printf("wait ack\r\n");;
		if(SSPCON2bits.ACKSTAT)
		{
			//printf("ackstat\n");
			StopI2C();
			return -2;				// return with Not Ack error condition
		}
	}
	
	AckI2C();
	StartI2C();

	while(SSPCON2bits.PEN);			// wait until stop condition is over 
	if(PIR2bits.BCLIF)				// test for bus collision
	{
		return -1;					// return with Bus Collision error 
	}
	control |= 0x01;
	
	//printf("EERandomRead: Byte to write to I2C: %02x\r\n", control);

	err = WriteI2C(control);
	//printf("done\r\n");
	if(err)
	{
		//printf("err: %d\r\n", err);
		StopI2C();
		return -3;				// set error for write collision
	}
	//printf("wait ack\r\n");
	if(SSPCON2bits.ACKSTAT)
	{
		//printf("ackstat\n");
		StopI2C();
		return -2;				// return with Not Ack error condition
	}
	
//	while(!SSPSTATbits.BF) {
//		//printf("SSPBUF = %02x\r\n", SSPBUF);
//	}
	for(i = 0; i < len-1; i++) {
		*data++ = ReadI2C();
		AckI2C();
	}
	*data++ = ReadI2C();          	//Read final byte
	NotAckI2C();

	StopI2C();							// send STOP condition
	while(SSPCON2bits.PEN);			// wait until stop condition is over 
	if(PIR2bits.BCLIF)				// test for bus collision
	{
		return -1;					// return with Bus Collision error 
	}
	return 0;						// return with no error
}
