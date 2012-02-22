import binascii
import re
from sensor_calibration_test import calibrate

TIME_BETWEEN_READINGS = 3 #seconds

dump = open(raw_input("Filename of dump file: ")).read()

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
readings_list = []
while True:
    #copy out 16 bytes at a time
    block = data[addr:addr+16]

    
    # Is this block entirely 0xFF?
    # If so, end the program
    end = True
    for b in block:
        if b != 255:
            end = False
            break
    if end:
        print "Found unwritten block, stopping."
        break

    
    #print ut, up
    while len(block) > 5:
        #chomp off the data, callibrate it, add it to the list of readings
        block, up = press_parse(block)
        block, ut = temp_parse(block)
        
        readings.append(calibrate(ut, up))
    
    #If last byte has any error bits set, add it to the list.
    if block[0] != 0:
        if block[0] == 1:
            #That's a reboot!
            #The last three readings were in the wrong list, move them.
            readings_list.append(readings[:-3])
            readings = readings[-3:]
        else:
            errors.append(block[0])

    #Now do it again!
    addr += 16

# Issue: The first period had no readings, and the last wasn't recorded at all!
# So that's what this hack is for.
readings_list = readings_list[1:] + readings

print "Errors:",
print errors

print "List of uptime (minutes):",
print map(lambda x: len(x)*TIME_BETWEEN_READINGS/float(60), readings_list)


