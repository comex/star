# Usage: python ipsw.py [the ipsw to import]
import zipfile, plistlib, shutil, sys, os, tempfile, atexit

def system(x):
    print x
    if os.system(x):
        raise Exception('Command failed')

def go_away():
    try:
        os.rmdir(output)
    except:
        pass
atexit.register(go_away)

input_path = os.path.realpath(sys.argv[1])
os.chdir(os.path.dirname(os.path.realpath(sys.argv[0])))
fbs = os.path.realpath('FirmwareBundles')
configdata = os.path.realpath('configdata.py')
iDeviceKeys = plistlib.readPlist('iDeviceKeys.plist')
out_root = os.path.realpath('../bs')
tmpdir = tempfile.mkdtemp()
print 'tmpdir:', tmpdir
os.chdir(tmpdir)

z = zipfile.ZipFile(input_path, 'r', zipfile.ZIP_DEFLATED)
nl = z.namelist()
#print nl
pl = plistlib.readPlistFromString(z.read('Restore.plist'))
identifier = '%s_%s_%s' % (pl['ProductType'], pl['ProductVersion'], pl['ProductBuildVersion'])
short_identifier = '%s_%s' % (pl['ProductType'], pl['ProductVersion'])
output = os.path.join(out_root, short_identifier)
os.mkdir(output)
pwnage_pl_fn = '%s/%s.bundle/Info.plist' % (fbs, identifier)

stuff = iDeviceKeys.get(pl['ProductType'], {}).get(pl['ProductVersion'])

if stuff is not None and stuff.has_key('KernelCache'):
    kc = stuff['KernelCache']
    kc_key = kc['Key']
    kc_iv = kc['IV']
    fs_key = stuff['RootFilesystem']['RootFilesystemKey']
elif os.path.exists(pwnage_pl_fn):
    system('plutil -convert xml1 %s' % pwnage_pl_fn)
    pwnage_pl = plistlib.readPlist(pwnage_pl_fn)
    kc = pwnage_pl['FirmwarePatches']['KernelCache']
    kc_key = kc['Key']
    print kc
    kc_iv = kc['IV']
    fs_key = pwnage_pl['RootFilesystemKey']
else:
    print 'No keys...'
    sys.exit(1)

print 'kernelcache...'
kc_name = pl.get('KernelCachesByTarget', pl.get('KernelCachesByPlatform')).values()[0]['Release']
z.extract(kc_name)
system('xpwntool %s tempkc.e -k %s -iv %s -decrypt' % (kc_name, kc_key, kc_iv)) #!
os.unlink(kc_name)
system('xpwntool tempkc.e %s/kern' % output) #!
os.unlink('tempkc.e')

print 'root filesystem...'
fs_name = pl['SystemRestoreImages']['User']
system('unzip -q -o -j "%s" %s' % (input_path, fs_name)) # 'unzip' used for speed
system('vfdecrypt -i %s -k %s -o temproot.dmg' % (fs_name, fs_key))
os.mkdir('mnt')
system('hdiutil attach -noverify -mountpoint mnt temproot.dmg')
if os.path.exists('mnt/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv7'):
    arch = 'armv7'
    shutil.copy('mnt/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv7', '%s/cache' % output)
elif os.path.exists('mnt/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv6'):
    arch = 'armv6'
    shutil.copy('mnt/System/Library/Caches/com.apple.dyld/dyld_shared_cache_armv6', '%s/cache' % output)

shutil.copy('mnt/sbin/launchd', '%s/launchd' % output)
os.chmod('%s/launchd' % output, 0755)
system('hdiutil detach mnt')
os.unlink('temproot.dmg')
os.unlink(fs_name)
p = os.popen('lipo -info %s/launchd' % output)
stuff = p.read().strip()

if '3.1.' in short_identifier:
    arch += '_3.1.x'
#else:
#    arch += '_3.2+'

# allow for customization.
if not eval('{%s}' % open(configdata).read()).has_key(short_identifier):
    new = '''
'*X*': {
    '<': '.*A*',
    '#kern': {
    
    },
},
    '''.strip().replace('*A*', arch).replace('*X*', short_identifier)
    open(configdata, 'a').write(new + '\n')

# clean up
os.rmdir('mnt')
os.chdir('/')
os.rmdir(tmpdir)
