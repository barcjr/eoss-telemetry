#define		PREFIX							10
#define		SUFFIX							11
#define		TERMINATOR						0xFF
#define		DATA_BYTES_PER_LINE				4

#define		CALLSIGN_SLOW_FACTOR			2
#define		ALTITUDE_SLOW_FACTOR			2

// Last byte is the length
// These are automatically generated by a python script.
rom const unsigned char MorseCodeLib[15][DATA_BYTES_PER_LINE + 1] =
{
	{0x77, 0x77, 0x07, 0x00, 0x16},  // '0'
	{0xdd, 0xdd, 0x01, 0x00, 0x14},  // '1'
	{0x75, 0x77, 0x00, 0x00, 0x12},  // '2'
	{0xd5, 0x1d, 0x00, 0x00, 0x10},  // '3'
	{0x55, 0x07, 0x00, 0x00, 0x0e},  // '4'
	{0x55, 0x01, 0x00, 0x00, 0x0c},  // '5'
	{0x57, 0x05, 0x00, 0x00, 0x0e},  // '6'
	{0x77, 0x15, 0x00, 0x00, 0x10},  // '7'
	{0x77, 0x57, 0x00, 0x00, 0x12},  // '8'
	{0x77, 0x77, 0x01, 0x00, 0x14},  // '9'
	{0x1d, 0x5d, 0x71, 0x00, 0x1e},  // 'ALT '
	{0x70, 0x07, 0x00, 0x00, 0x12},  // ' M '
	{0xdd, 0x71, 0x77, 0x77, 0x20},  // 'W0DK/B'
	{0x5c, 0x71, 0x1d, 0x57, 0x20},  // 'W0DK/B'
	{0x17, 0x57, 0x01, 0x00, 0x14},  // 'W0DK/B'
};

unsigned char schedule[32];
unsigned char txPos = 0;
unsigned char writePos = 0;
unsigned char skippy = 0;		// Used to slow down morse transmission.
unsigned char callsignTimeLeft = 0;	// Transmit with the callsign slow factor if > 0. Measured in long elements
unsigned char timeUntilCallsignTime = 0;

/************************************************************************
*
* Purpose:		To look at the schedule and find the scheduled bit.
* Passed:		0-255 depending on which bit you want to look at from the
*				schedule.
* Returned:		0-1 depending on the state of the bit.
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
* Purpose:		Called when the main loop deems it time to tranmit the next segment.
* Passed:		None
* Returned:		None
* Note:			
************************************************************************/
void stepMorse()
{
	skippy++;
	if(callsignTimeLeft > 0 && timeUntilCallsignTime == 0)
	{
		if(skippy == CALLSIGN_SLOW_FACTOR)
		{
			// Don't skip this time.
			skippy = 0;
			callsignTimeLeft--;
		} else {
			// Skip!
			return;
		}
	}
	else
	{
		if(skippy == ALTITUDE_SLOW_FACTOR)
		{
			// Don't skip this time.
			skippy = 0;
			if(timeUntilCallsignTime > 0)
			{
				timeUntilCallsignTime--;
			}
		} else {
			// Skip!
			return;
		}
	}
	
	
	MORSE_PIN = getBitFromSchedule(txPos);

	//printf((const far rom char*) "%d", MORSE_PIN);
	//printf((const far rom char*) "txPos: %d\r\n", txPos);
	//printf((const far rom char*) "writePos: %d\r\n", writePos);
	//printf((const far rom char*) "Byte: %d\r\n", schedule[txPos >> 3]);
	
	// If the bottom three bits are zero, we just finished the previous byte.
	// Clear it out.
	if((txPos & 0x07) == 0)
	{
		schedule[(txPos - 1) >> 3] = 0;		/*	Clear out the schedule behind you, so that bits from 256 ticks ago
												don't come back to haunt you.	*/
	}
	txPos++;
}

/************************************************************************
*
* Purpose:		Pushes the items given onto the schedule queue.
* Passed:		Morse code encoded as what's shown in MorseCodeLib. Terminate
* 				sequences with 0xFF.
* Returned:		None
*
************************************************************************/
void scheduleMorse(unsigned char *morse)
{
	unsigned char i;
	unsigned char index, txBit;
	
	
	while((index = *morse++) != TERMINATOR)
	{
		unsigned char length = MorseCodeLib[index][DATA_BYTES_PER_LINE];
		//printf((const far rom char*) "MorseCodeLib: %d\r\n", index);
		//printf((const far rom char*) "\r\n");
		for(i = 0; i < length; i++)
		{
			/*
			We get the bit from the morse code library, then OR it with
			the byte already in the schedule. Result: We write one element
			to the schedule.
			*/
			
			txBit = (MorseCodeLib[index][i >> 3] >> (i & 0x07)) & 0x01;
			//printf((const far rom char*) "%d", txBit);
			
			schedule[writePos >> 3] |= txBit << (writePos & 0x07);
			//printf((const far rom char*) "SByte: %d\r\n", schedule[writePos >> 3]);
			writePos++;
		}
		//printf((const far rom char*) "\r\n");
	}
}

/************************************************************************
*
* Purpose:		Determines the length of the block-morse passed to it
* Passed:		Morse code encoded as what's shown in MorseCodeLib. Terminate
* 				sequences with 0xFF.
*
* Returned:		The length of the morse sequence in elements.
*
************************************************************************/

unsigned char getLengthOfMorse(unsigned char *morse)
{
	unsigned char length = 0;
	unsigned char index = 0;
	while((index = *morse++) != TERMINATOR)
	{
		length += MorseCodeLib[index][DATA_BYTES_PER_LINE];
	}
	return length;
}

/************************************************************************
*
* Purpose:		Pushes the callsign onto the schedule queue. This function
* expects that txPos == writePos
* Passed:		None
* Returned:		None
*
************************************************************************/
void txCallsign()
{
	unsigned char morse[] = {12, 13, 14, TERMINATOR};
	unsigned char length = getLengthOfMorse(&morse[0]);
	
	timeUntilCallsignTime = writePos - txPos;
	
	//printf((const far rom char*) "txCallsign\r\n");

	callsignTimeLeft = length;
	scheduleMorse(&morse[0]);
	//scheduleDump();
}

/************************************************************************
*
* Purpose:		Dumps the schedule to printf
* Passed:		None
* Returned:		None
*
************************************************************************/
void scheduleDump()
{
	int i;
	for(i = 0; i < 32; i++)
	{
		printf((const far rom char*) "%d: %d\r\n", i, schedule[i]);
	}
}





/************************************************************************
*
* Purpose:		Changes altitude into the morse library sequence
* Passed:		Altitude, Pointer to array
* Returned:		None
*
************************************************************************/

//Sorta hacky, but oh well.
#define INSERT_IN_MORSE(item) *morsePointer++ = (item);

void formatAltitude(unsigned short alt, unsigned char *morsePointer)
{
	signed char i;
	unsigned char leading_zero = TRUE;
	unsigned char array_index = 0;
	unsigned short number;
	
	//printf((const far rom char*) "formatAltitude start\r\n");
	
	INSERT_IN_MORSE(PREFIX)
	
	// This counts down from 4 to 0 because the most significant digit comes first.
	for(i = 4; i >= 0; i--)
	{
		number = (unsigned short) alt / pow(10, i);
		number = number % 10;
		
		/*
		If the other digits so far have been zeros, and this
		is a zero, and this isn't the last digit, don't output
		this digit.
		*/
		
		if(leading_zero == TRUE && number == 0 && i != 0)
		{
			// Do nothing
		}
		else
		{
			INSERT_IN_MORSE(number);
			leading_zero = FALSE;
		}
	}
	
	INSERT_IN_MORSE(SUFFIX);			// The "H M" at the end of the sequence
	
	INSERT_IN_MORSE(TERMINATOR);		// Terminator
	
	//printf((const far rom char*) "formatAltitude end\r\n");
}
