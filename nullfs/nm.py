#!/usr/bin/python
import sys, os, struct, anydbm
syms = {}
for line in os.popen('/opt/xbinutils/bin/gnm -a ~/share/ipadkern'):
    line = line.strip().split()
    if len(line) != 3: continue
    try:
        addr = int(line[0], 16)
    except:
        continue
    sym = line[2]
    if line[1] == 'T' and sym not in ('_copyin', '_memset'):
        addr |= 1
    syms[sym] = addr

# todo: config.
syms['_vfs_getattr'] = 0xc01d6ac5
syms['_namei'] = 0xc007f0a9
syms['_nameidone'] = 0xc007e7a1
syms['_ubc_info_init'] = 0xc01616a5
syms['_VFS_VPTOFH'] = 0xc0090fe5
syms['_VFS_FHTOVP'] = 0xc0091071
syms['_VFS_VGET'] = 0xc0091101

outf = open('loader.txt', 'w')

usyms = []

for line in os.popen('otool -Ivv nullfs.dylib'):
    line = line.strip()
    if not line.startswith('0x') or '_' not in line: continue
    iaddr = int(line[2:10], 16)
    if iaddr > 0xe0000000:
        sym = line[line.find('_'):]
        usyms.append((iaddr, sym))

for line in os.popen('otool -rvv nullfs.dylib'):
    line = line.strip()
    if '_' in line and '(__' not in line:
        iaddr = int(line[:8], 16) + 0xc06ed000
        sym = line[line.find('_'):]
        usyms.append((iaddr, sym))

for iaddr, sym in usyms:
    #print hex(iaddr), sym, syms.get(sym)
    print >> outf, '%x %x' % (iaddr, syms[sym])
    # todo: write it to the file at that point
    # borrow the routine from config.py


