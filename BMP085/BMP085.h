void delay_ms(unsigned short ms);
void BMP085_Calibration(void);
short bmp085ReadShort(unsigned char address);
short bmp085ReadTemp(void);
short bmp085ReadTemp(void);
short bmp085ReadPressure(void);
short bmp085ReadShort(unsigned char address);
void bmp085Convert(long *temperature, long *pressure);
