# Written by Nick O'Dell
# Can decode a line from morse_encode.py, useful for debugging

import sys, binascii, re
ux = binascii.unhexlify

char_representation = {0: " ", 1: "#"}

def extract_bytes(s):
    regex = "0x[0-9A-F]{2}"
    return [ux(c[2:4]) for c in re.findall(regex, s, flags=re.IGNORECASE)]
string = raw_input("Enter a line generated from morse_encode.py: ")
byte_list = extract_bytes(string)
if not byte_list:
    raise Exception("Invalid Input!")

length = ord(byte_list[-1]) # The last one is length

j = 0
for s in byte_list[:-1]:
    s = ord(s)
    for i in xrange(8):
        j += 1
        if j > length:
            break
        sys.stdout.write(char_representation[s & 1]) #instead of printing, set the transmit pin with this.
        s = s >> 1

    # Kinda hacky, but oh well
    if j > length:
        break
sys.stdout.write("END")
