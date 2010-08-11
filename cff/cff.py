import struct, sys
from cStringIO import StringIO
class vfile:
    def __init__(self, f):
        self.f = f
        self.off = f.tell()
    def seek(self, x, y=0):
        if y == 1:
            self.off += x 
        else:
            self.off = x
    def read(self, x):
        old = self.f.tell()
        self.f.seek(self.off)
        ret = self.f.read(x)
        self.off += x
        self.f.seek(old)
        return ret
    def tell(self):
        return self.off
def rb(f):
    try:
        return ord(f.read(1))
    except TypeError:   
        return None
def rh(f):
    return struct.unpack('>H', f.read(2))[0]
def rw(f):
    return struct.unpack('>I', f.read(4))[0]

def read_off(offsize, f):
    s = '\0' * (4 - offsize) + f.read(offsize)
    return struct.unpack('>I', s)[0]

def read_realop(f):
    def i():
        while True:
            byte = rb(f)
            yield (byte >> 4)
            yield (byte & 0xf)
    s = ''
    for nib in i():
        if 0 <= nib <= 9:
            s += str(nib)
        elif nib == 0xa:
            s += '.'
        elif nib == 0xb:
            s += 'E'
        elif nib == 0xc:
            s += 'E-'
        elif nib == 0xd:
            raise ValueError(nib)
        elif nib == 0xe:
            s += '-'
        elif nib == 0xf:
            return s

def read_intop(f):
    b0 = rb(f)
    if 32 <= b0 <= 246:
        return b0-139
    elif 247 <= b0 <= 250:
        b1 = rb(f)
        return (b0-247)*256 + b1 + 108
    elif 251 <= b0 <= 254:
        b1 = rb(f)
        return -(b0-251)*256 - b1 - 108
    elif b0 == 28:
        return rh(f)
    elif b0 == 29:
        return rw(f)
    elif b0 == 30:
        return read_realop(f)
    elif b0 == 255:
        return float(struct.unpack('>i', f.read(4))[0]) / 65536
    else:
        raise ValueError(b0)

def read_index(f):
    count = rh(f)
    if count == 0:
        return []
    offsize = rb(f)
    offbase = f.tell() + offsize*(count+1) - 1
    offs = vfile(f)
    offs = [read_off(offsize, offs) for i in xrange(count+1)]
    data = []
    for i in xrange(count):
        f.seek(offs[i] + offbase)
        data.append(f.read(offs[i+1] - offs[i]))
    f.seek(offs[count] + offbase)
    return data

def read_dict(f):
    opnds = []
    ret = []
    while True:
        b0 = rb(f)
        if b0 is None: break
        if b0 <= 21:
            if b0 == 12:
                op = (b0, rb(f))
            else:
                op = b0
            ret.append((op, opnds[0] if len(opnds) == 1 else tuple(opnds)))
            opnds = []
        else:
            f.seek(-1, 1)
            op = read_intop(f)
            opnds.append(op)
    return ret

def read_csop(f):
    b0 = rb(f)
    if b0 is None:
        return None
    elif b0 == 12:
        b1 = rb(f)
        return tbops[b1]
    elif 0 <= b0 <= 11 or 13 <= b0 <= 18 or 19 <= b0 <= 20 or 21 <= b0 <= 27 or 29 <= b0 <= 31:
        return ops[b0]
    else:
        f.seek(-1, 1)
        return read_intop(f)

ops = {
    0: '0??',
    1: 'hstem',
    2: '2??',
    3: 'vstem',
    4: 'vmoveto',
    5: 'rlineto',
    6: 'hlineto',
    7: 'vlineto',
    8: 'rrcurveto',
    9: '9??',
    10:'callsubr',
    11:'return',
    13:'13??',
    14:'endchar',
    15:'15??',
    16:'16??',
    17:'17??',
    18:'hstemhm',
    19:'hintmask',
    20:'cntrmask',
    21:'rmoveto',
    22:'hmoveto',
    23:'vstemhm',
    24:'rcurveline',
    25:'rlinecurve',
    26:'vvcurveto',
    27:'hhcurveto',
    29:'callgsubr',
    30:'vhcurveto',
    31:'hvcurveto',
}
tbops = {
    0: 'dotsection',
    3: 'and',
    4: 'or',
    5: 'not',
    9: 'abs',
    10:'add',
    11:'sub',
    12:'div',
    14:'neg',
    15:'eq',
    18:'drop',
    20:'put',
    21:'get',
    22:'ifelse',
    23:'random',
    24:'mul',
    26:'sqrt',
    27:'dup',
    28:'exch',
    29:'index',
    30:'roll',
    34:'hflex',
    35:'flex',
    36:'hflex1',
    37:'flex1',

}
    
def read_cs(f):
    while True:
        dec = read_csop(f)
        print dec,
        if dec == 'endchar' or dec is None:
            print
            break
        elif type(dec) == str:
            print




def read_cff(f):
    # Header
    major = rb(f)
    minor = rb(f)
    hdrsize = rb(f)
    absoffsize = rb(f)

    names = read_index(f)
    print 'Fonts:', names

    topdicts = [read_dict(StringIO(x)) for x in read_index(f)]
    
    strings = read_index(f)
    print 'Strings:', strings

    for font in topdicts:
        print '--font--'
        font = dict(font)
        print font
        for i in (0, 1, 2, 3, 4):
            print 'font[%d]:' % i,
            x = font.get(i)
            if x is None: 
                print 'notdef'
            elif x > 390:
                print strings[x - 391]
            else:
                print '<standard: %d>' % x

        #encoding_off = font[16]
        #charset_off = font[15]
        charstring_off = font[17]
        private_off = font[18]
        charstringtype = font.get((12, 6), 2)
        if charstringtype == 2:
            f.seek(charstring_off)
            for charstring in read_index(f):
                g = vfile(f)
                g.seek(0)
                print g.read(10485760).find(charstring)
                print '-'*80
                read_cs(StringIO(charstring))
            #print read_cs(g)
    
read_cff(open(sys.argv[1]))
