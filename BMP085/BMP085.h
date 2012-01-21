void delay_ms(unsigned short ms);
void BMP085_Calibration(void);
long bmp085ReadTemp(void);
long bmp085ReadPressure(void);
unsigned short bmp085ReadShort(unsigned char address);
void bmp085Convert(long *temperature, long *pressure, unsigned char readings);
void BMP_process_error();
