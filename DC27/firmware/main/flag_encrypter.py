#!/usr/bin/python
import sys

if (len(sys.argv) < 2):
	print "usage: ./flag_encrypter.py \"{flag: doodlebug}\""
	exit()

flag = sys.argv[1]

key = "GOURRY"
out = ""
idx = 0
for c in flag:
	out = out + chr(ord(c) ^ ord(key[idx]))
	new = chr(ord(key[idx]) + 1)
	key = key[:idx] + new + key[idx+1:]
	idx += 1
	if (idx == 6):
		idx = 0

print flag
print repr(out)
