"""Pure-python implementation of the
BMP085 calibration routine. Constants
and input values are hard-coded. I may
clean this up at some point."""


ac1 = 8271
ac2 = -958
ac3 = -14352
ac4 = 33487
ac5 = 25164
ac6 = 22590
b1 = 5498
b2 = 48
mb = -32768
mc = -11075
md = 2432

oss = 3

def calibrate(ut, up):
    up = up >> (8-oss)

    #Force it to handle as float
    up, ut = float(up), float(ut)
    
    x1 = (ut - ac6) * ac5 / 2**15
    x2 = mc * 2**11 / (x1 + md)
    b5 = x1 + x2
    t = (b5 + 8) / 2**4

    b6 = b5 - 4000
    x1 = (b2 * (b6 * b6 / 2**12))/2**11
    x2 = ac2 * b6 / 2**11
    x3 = x1 + x2


    b3 = ((ac1 * 4 + x3)*(2**oss) + 2) / 4
    x1 = ac3 * b6 / 2**13
    x2 = (b1 * (b6 * b6 / 2**12))/2**16
    x3 = ((x1 + x2) + 2) / 4


    b4 = ac4 * (x3 + 32768) / 2**15
    b7 = (up - b3) * (50000/2**oss)
    p = b7*2/b4
    x1 = (p / 2**8)**2
    x1 = (x1 * 3038)/2**16
    x2 = (-7357 * p)/2**16
    p = p + (x1 + x2 + 3791) / 2**4

    return t, p


