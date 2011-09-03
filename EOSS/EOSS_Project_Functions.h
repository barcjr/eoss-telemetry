#ifndef _EOSS_PROJECT_1_H_
#define _EOSS_PROJECT_1_H_

unsigned char getBitFromSchedule(unsigned char bitPos);
void stepMorse();
unsigned char getLengthOfMorse(unsigned char *morse);
void scheduleMorse(unsigned char *morse);
void txCallSign();
void openTxUsart();
unsigned char *formatAltitude(signed short alt);
void txUsart(const rom char *dataString);
void BMP085_Calibration(void);
short bmp085ReadShort(unsigned char address);
short bmp085ReadTemp(void);
short bmp085ReadPressure(void);
void bmp085Convert(long *temperature, long *pressure);

#endif
