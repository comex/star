# Turn a memory dump into something like the original Mach-O file (containing segments at the right location)
# Usage: python unload.py input-dump starting-address-of-input-dump output
# You can get input-dump by dd-ing from /dev/kmem starting at either c0000001 or 80000001 (or something around there)
import sys, struct
f = open(sys.argv[1], 'rb')
addy = int(sys.argv[2], 16)
g = open(sys.argv[3], 'wb')
f.seek(0x1000 - (addy & 0xfff))
assert f.read(4) == '\xce\xfa\xed\xfe'
cputype, cpusubtype, \
filetype, ncmds, sizeofcmds, flags = struct.unpack('IIIIII', f.read(24))
for ci in xrange(ncmds):
    z = f.tell() 
    cmd, cmdsize = struct.unpack('II', f.read(8))
    if cmd == 1: # LC_SEGMENT
        segname = f.read(16)
        vmaddr, vmsize, fileoff, filesize = struct.unpack('IIII', f.read(16))
        print segname, hex(vmaddr), hex(fileoff), hex(filesize)
        if vmaddr >= addy:
            f.seek(vmaddr - addy)
            g.seek(fileoff)
            g.write(f.read(filesize))
        else:
            print 'fail'
    f.seek(z + cmdsize)
f.close()
g.close()
