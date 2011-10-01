#ifndef _EOSS_PROJECT_H_
#define _EOSS_PROJECT_H_

unsigned char getBitFromSchedule(unsigned char bitPos);
void stepMorse();
unsigned char getLengthOfMorse(unsigned char *morse);
void scheduleMorse(unsigned char *morse);
void txCallsign();
void openTxUsart();
void formatAltitude(unsigned short alt, unsigned char *morsePointer);
void txUsart(const rom char *dataString);
void scheduleDump();

#endif
