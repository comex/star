#!/usr/bin/env python
import os, sys, shutil, time
os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])) + '/..')
os.system('rm -rf staged')
os.system('make clean')
os.mkdir('staged')
os.mkdir('staged/_')
os.mkdir('staged/json')
for plat in eval('{%s}' % open('config/configdata.py').read()).keys():
    if plat.startswith('.') or plat.endswith('_self'): continue
    outfn = 'staged/_/' + plat + '.pdf'
    jsonfn = 'staged/json/' + plat + '.json'
    if os.system('config/config.py "%s" 2>/dev/null' % plat) != 0:
        print 'failed to config for', plat
        open(outfn + '_FAILED', 'w')
        time.sleep(1)
        continue
    shutil.copy('config/config.json', jsonfn)
    if os.system('make') != 0:
        print 'failed to make for', plat
        open(outfn + '_FAILED', 'w')
        time.sleep(1)
        continue
    shutil.copy('cff/out.pdf', outfn)
    time.sleep(1)
