import binascii
import re
from sensor_calibration_test import calibrate

dump = open("dump.txt").read()

# Look for at least 500 hex digits
data = re.search("[0-9a-fA-F]{500,}", dump).group(0)
data = binascii.unhexlify(data)
data = map(ord, data)

def press_parse(block):
    up = reduce(lambda x, y: (x << 8) + y, block[0:3])
    return block[3:], up

def temp_parse(block):
    ut = reduce(lambda x, y: (x << 8) + y, block[0:2])
    return block[2:], ut

addr = 0
errors = []
readings = []
while True:
    block = data[addr:addr+16]
    end = True
    # Is this block entirely 0xFF?
    # If so, end the program
    for b in block:
        if b != 255:
            end = False
            break
    if end:
        break
    #print ut, up
    while len(block) > 5:
        block, up = press_parse(block)
        block, ut = temp_parse(block)
        
        readings.append(calibrate(ut, up))
    #If last byte has any error bits set, add it to the list.
    if block[0] != 0:
        errors.append(block[0])
        #print block[0]
    addr += 16
print errors
for reading in readings:
    print reading[0], ',', reading[1]
    

