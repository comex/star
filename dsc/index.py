import struct, anydbm, sys, glob

def build(path):
    db = anydbm.open('index', 'c')
    files = glob.glob('%s/*.txt' % path)
    db['_files'] = '\0'.join(files)
    for fil in files:
        lineno = 1
        fileoff = 0
        for line in open(fil, 'rb'):
            scratch = line[:6]
            try:
                key = int(line[:6], 16)
            except:
                if ':' in line:
                    key = '\0\0\0\0' + line[:line.find(':')]
                    db[key] = struct.pack('III', files.index(fil), lineno, fileoff)
            else:
                key = struct.pack('I', key)
                if not db.has_key(key):
                    db[key] = struct.pack('III', files.index(fil), lineno, fileoff)
            lineno += 1
            fileoff += len(line)

def ful(fp):
    ret = ''
    while True:
        ln = fp.readline()
        ret += ln
        if ln == '' or 'pop\t' in ln: break
    return ret

def lookup_sym(sym, full=False):
    db = anydbm.open('index')
    files = db['_files'].split('\0')
    key = '\0\0\0\0' + sym
    try:
        fili, line, filoff = struct.unpack('III', db[key])
    except KeyError:
        return None
    fp = open(files[fili], 'rb')
    fp.seek(filoff)
    if full: return ful(fp).rstrip()
    return '[%s:%d] %s %s' % (files[fili], line, fp.readline().strip(), fp.readline().strip())

def lookup_addr(addr, full=False):
    db = anydbm.open('index')
    files = db['_files'].split('\0')
    addr &= ~1
    key = struct.pack('I', int(addr) / 0x100)
    try:
        fili, line, filoff = struct.unpack('III', db[key])
    except KeyError:
        return None
    fp = open(files[fili], 'rb')
    fp.seek(filoff)
    q = '%08x' % addr
    lineno = line
    while True:
        line = fp.readline()
        if line[:8] == q:
            if full: return (line + ful(fp)).rstrip()
            return '[%s:%d] %s' % (files[fili], lineno, line.strip())
        elif line == '':
            return None
        lineno += 1

if sys.argv[1] == 'build':
    build(sys.argv[2])
elif sys.argv[1] == 'sym':
    print lookup_sym(sys.argv[2], len(sys.argv) > 3 and sys.argv[3] == 'full')
else:
    print lookup_addr(int(sys.argv[1], 16), len(sys.argv) > 2 and sys.argv[2] == 'full')
