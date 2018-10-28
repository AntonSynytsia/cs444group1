import os
import mmap
import struct
import random

mem = os.open("./big_file", os.O_RDWR)
addr = mmap.mmap(mem, 0x40000000, offset=random.randint(1, 256)*mmap.ALLOCATIONGRANULARITY)

base = 100
i = struct.unpack('I', addr[base:base+4])[0]
print(i)
