import os, sys
for plat in eval('{%s}' % open('../config/configdata.py').read()).keys():
    if plat.startswith('.') or plat.endswith('_self'): continue
    if os.system('../config/config.py "%s" 2>/dev/null' % plat) != 0:
        print 'failed to config for', plat
        sys.exit(1)
    thing = os.popen('''echo "else if(0 == strcmp(platform, CONFIG_PLATFORM)) { kernel_pmap = CONFIG_KERNEL_PMAP; mem_size = CONFIG_MEM_SIZE; }" | cpp `cat ../config/config.cflags` | tail -n 1''').read()
    print thing.strip()

