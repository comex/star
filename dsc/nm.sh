#!/bin/bash
# Usage: ./nm.sh iPad1,1_3.2.cache syms/iPad1,1_3.2
set -e
test "x$2" != "x"
if [ -e f/System ]; then umount f; fi
./dsc "$1" f
sleep 1
find f -type f | (while IFS='*' read x
do
echo "$x"
(nm -a -p -m "$x" | egrep '^[^ ]') || true
done) | /opt/local/bin/python2.6 -c "import sys, os, struct, anydbm
if os.path.exists('$2'): os.remove('$2')
db = anydbm.open('$2', 'c')
for line in sys.stdin:
    line = line.strip()
    z = line.find('external')
    if z == -1: continue
    addy = int(line[:8], 16)
    if '[Thumb]' in line:
        addy |= 1
        sym = line[z+17:]
    else:
        sym = line[z+9:]
    db[sym] = struct.pack('I', addy)"

