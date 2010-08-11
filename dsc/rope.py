import os, re, sys
def foo(m):
    return '#0x%x' % (int(m.group(1), 16) - int(m.group(2), 16))

limit = int(sys.argv[2])
lines = [None] * limit
for line in open(sys.argv[1]):
    line = re.sub('\s*;.*$', '', line)
    line = map(str.strip, line.split('\t', 2))
    if len(line) < 3: continue
    b, x, line = line
    line = line.strip().replace('\t', ' ').replace('.w ', ' ')
    if not re.match('[a-z]', line): continue
    m = re.match('b.*x.*r', line)
    lines.append(line)
    n = re.match('blx?', line)
    if n:
        lines = []
    if m:
        stuff = ''.join(a[:20].ljust(25) for a in lines[:-1])
        print b.ljust(10), stuff, lines[-1]
        #lines = []
    while len(lines) > limit: lines.pop(0)
