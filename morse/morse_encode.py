# Written by Nick O'Dell
# See readme.md for an explanation of why this is needed

import sys
import binascii
hx = binascii.hexlify

morse_table = open("morse_table.txt").read()

morse_table = dict([(morse[0:1], morse[2:len(morse)]) for morse in morse_table.split("\n")])

DATA_BYTES_PER_LINE = 4

CALLSIGN = raw_input("What's your callsign? ")
OUTPUT_METHOD = None
while OUTPUT_METHOD != "y" and OUTPUT_METHOD != "n":
    OUTPUT_METHOD = raw_input("Do you want the output to be written to a file? (y/n) ").lower()
out_file = None
if OUTPUT_METHOD == "y":
    out_file = open("morse_array.out.txt", "w")

def pack_result(result):
    byte_array = []
    i = 0
    byte = 0
    for bit in result:
        byte += bit << (i % 8)
        i += 1
        if i % 8 == 0:
            byte_array.append(byte)
            byte = 0
        
    if byte != 0:
        byte_array.append(byte)
    elements = i
    
    # Pad it out
    while len(byte_array) < DATA_BYTES_PER_LINE:
        byte_array.append(0)
    
    # Add in the length byte
    byte_array.append(elements)
    return byte_array

def chop(morse, size):
    for i in xrange(0, len(morse), size):
        yield morse[i:i+size]


def character_to_morse(char):
    char = char.upper()
    morse_list = []
    morse = morse_table[char]
    for element in morse:
        if element == "-":
            morse_list.extend([1, 1, 1])
        elif element == ".":
            morse_list.append(1)
        elif element == "_":
            morse_list.append(0) # The space between words. Would be 7 zeros long, but 6 are added in other places.
        else:
            raise Exception("Unexpected char: " + repr(element) + ", -._ allowed")
            sys.exit()
        morse_list.append(0) # The inter-element space
    morse_list.extend([0, 0])
    return morse_list

def format_bytes(char, morse_list):
    byte_string = ", ".join(["0x" + hx(chr(byte)) for byte in pack_result(morse_list)])
    s = "{"+byte_string+"},  // " + repr(char)
    print s
    if out_file:
        out_file.write(s + "\n")

for char in list("0123456789ABCDEF") + ["ALT ", "H M ", CALLSIGN]:
    result = []
    if len(char) > 1:
        for c in char:
            result.extend(character_to_morse(c))
    else:
        result.extend(character_to_morse(char))
        
    if char == CALLSIGN:
        for result_subset in chop(result, DATA_BYTES_PER_LINE * 8):
            format_bytes(char, result_subset)
    else:
        if len(result) > DATA_BYTES_PER_LINE * 8:
            raise Exception(str(len(result)) + " elements of morse too long for " + \
                            str(DATA_BYTES_PER_LINE) + " data bytes")
        format_bytes(char, result)


out_file.close()
