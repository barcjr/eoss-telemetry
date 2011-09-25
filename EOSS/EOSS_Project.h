#ifndef _EOSS_PROJECT_H_
#define _EOSS_PROJECT_H_

unsigned char getBitFromSchedule(unsigned char bitPos);
void stepMorse();
unsigned char getLengthOfMorse(unsigned char *morse);
void scheduleMorse(unsigned char *morse);
void txCallSign();
void openTxUsart();
unsigned char *formatAltitude(signed short alt);
void txUsart(const rom char *dataString);
void scheduleDump();

#endif
