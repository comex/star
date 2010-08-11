#define _FILE_OFFSET_BITS 64
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#define IOSFC_BUILDING_IOSFC
#include <IOSurface/IOSurfaceAPI.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
int kmem_fd = 0;

unsigned int pmap, _gVirtBase, _gPhysBase, starpmap, pmap408;
static unsigned int vram_phys, vram_length, vram_virt;

static unsigned int read32(unsigned int addy) {
    unsigned int ret;
    assert(kmem_fd);

    errno = 0;
    if(pread(kmem_fd, &ret, 4, (unsigned long long) addy) != 4) {
        printf("could not read %x: %s\n", addy, strerror(errno));
        abort();
    }
    return ret;
}

static void do_test() {
    int c;
    while(c = getchar()) if(c == '\n') break;
    char *stuff = malloc(160000);
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, stuff, 160000);
    close(fd);

    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dict, CFSTR("IOSurfaceIsGlobal"), kCFBooleanTrue);
    int val = (int) 'ARGB'; // LE
    CFDictionarySetValue(dict, CFSTR("IOSurfacePixelFormat"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    val = 200;
    CFDictionarySetValue(dict, CFSTR("IOSurfaceHeight"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    CFDictionarySetValue(dict, CFSTR("IOSurfaceBufferTileMode"), kCFBooleanFalse);
    val = 800;
    CFDictionarySetValue(dict, CFSTR("IOSurfaceBytesPerRow"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    val = 4;
    CFDictionarySetValue(dict, CFSTR("IOSurfaceBytesPerElement"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    val = 1000000;
    CFDictionarySetValue(dict, CFSTR("IOSurfaceAllocSize"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    val = 200;
    CFDictionarySetValue(dict, CFSTR("IOSurfaceWidth"), CFNumberCreate(NULL, kCFNumberSInt32Type, &val));
    CFDictionarySetValue(dict, CFSTR("IOSurfaceMemoryRegion"), CFSTR("PurpleGfxMem"));

    IOSurfaceRef surface = IOSurfaceCreate(dict);
    IOSurfaceLock(surface, 0, NULL);
    memcpy(IOSurfaceGetBaseAddress(surface), stuff, 160000);
    IOSurfaceUnlock(surface, 0, NULL);

    char *vram = malloc(vram_length);
    assert(pread(kmem_fd, vram, vram_length, (unsigned long long) vram_virt) == vram_length);
    for(int i = 0; i < (vram_length - 160000); i++) {
        if(!memcmp(vram + i, stuff, 160000)) {
            printf("Offset %d\n", i);
        }
    }

    CFRelease(surface);
}


static unsigned int virt2phys(unsigned int virt) {
    unsigned int r0 = read32(starpmap + 4*((virt - pmap408) >> 20));
    if(!r0) return 0;

    // weirdo is 0, so it's like
    // XXXYY000

    switch(r0 & 3) {
    case 1:
        r0 = (r0 & 0xfffffc00) + (_gVirtBase - _gPhysBase) + 4*((virt >> 12) & 0xff);
        if(!r0) return 0;
        return (read32(r0) & 0xfffff000) | (virt & 0xfff); // lolwhat
    case 2:
        if((r0 & 0x40000) || (r0 & 0xf0000 == 0x80000)) {
            return (r0 & 0xfff00000) | (virt & 0xfffff);
        } else {
            return (r0 & 0xffff0000) | (virt & 0xffffff);
        }
    default:
        return 0;
    }
}

int main(int argc, char **argv) {
    assert(sizeof(off_t) == 8);
    assert(argc == 2);
    char *name = argv[1];
    kmem_fd = open("/dev/kmem", O_RDWR);
    if(kmem_fd <= 0) {
        printf("This needs to be run as root.\n");
        return 1;
    }

    char platform[128];
    FILE *platform_fp = popen("(uname -m; sw_vers -productVersion) | xargs echo | sed 's/ /_/'", "r");
    fgets(platform, sizeof(platform), platform_fp);
    platform[strlen(platform)-1] = 0;
    fclose(platform_fp);
    printf("Platform: %s\n", platform);
    /*
     echo "else if(0 == strcmp(platform, CONFIG_PLATFORM)) { kernel_pmap = CONFIG_KERNEL_PMAP; mem_size = CONFIG_MEM_SIZE; }" | cpp `cat ../config/config.cflags` | tail -n 1
     */
    unsigned int kernel_pmap, mem_size;
    if(0);
    else if(0 == strcmp(platform, "iPad1,1_3.2.1")) { kernel_pmap = 0xc02401f4; mem_size = 0xc02671a0; }
    else if(0 == strcmp(platform, "iPhone1,2_4.0")) { kernel_pmap = 0x80241218; mem_size = 0x80264140; }
    else if(0 == strcmp(platform, "iPod2,1_4.0")) { kernel_pmap = 0x80241218; mem_size = 0x80264140; }
    else if(0 == strcmp(platform, "iPhone3,1_4.0")) { kernel_pmap = 0x80245218; mem_size = 0x80268140; }
    else if(0 == strcmp(platform, "iPad1,1_3.2")) { kernel_pmap = 0xc02401f4; mem_size = 0xc02671a0; }
    else if(0 == strcmp(platform, "iPhone2,1_4.0.1")) { kernel_pmap = 0x80245218; mem_size = 0x80268140; }
    else if(0 == strcmp(platform, "iPod3,1_4.0")) { kernel_pmap = 0x80245218; mem_size = 0x80268140; }
    else if(0 == strcmp(platform, "iPhone1,1_3.1.3")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPhone1,1_3.1.2")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPod2,1_3.1.3")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPod2,1_3.1.2")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPhone3,1_4.0.1")) { kernel_pmap = 0x80245218; mem_size = 0x80268140; }
    else if(0 == strcmp(platform, "iPhone1,2_3.1.3")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPod3,1_3.1.3")) { kernel_pmap = 0xc02071e0; mem_size = 0xc0226240; }
    else if(0 == strcmp(platform, "iPhone2,1_3.1.2")) { kernel_pmap = 0xc02071e0; mem_size = 0xc0226240; }
    else if(0 == strcmp(platform, "iPhone2,1_3.1.3")) { kernel_pmap = 0xc02071e0; mem_size = 0xc0226240; }
    else if(0 == strcmp(platform, "iPhone2,1_4.0")) { kernel_pmap = 0x80245218; mem_size = 0x80268140; }
    else if(0 == strcmp(platform, "iPod1,1_3.1.2")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPod1,1_3.1.3")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPod3,1_3.1.2")) { kernel_pmap = 0xc02071e0; mem_size = 0xc0226240; }
    else if(0 == strcmp(platform, "iPhone1,2_3.1.2")) { kernel_pmap = 0xc020e1e0; mem_size = 0xc022d244; }
    else if(0 == strcmp(platform, "iPhone1,2_4.0.1")) { kernel_pmap = 0x80241218; mem_size = 0x80264140; }
    else {
        printf("Error: Platform not recognized.\n");
        return 1;
    }
    
    pmap = read32(kernel_pmap);
    _gVirtBase = read32(mem_size - 12);
    _gPhysBase = read32(mem_size - 8);
    starpmap = read32(pmap);
    pmap408 = read32(pmap + 0x408);

    io_iterator_t it;
    assert(!IORegistryCreateIterator(kIOMasterPortDefault, kIOServicePlane, kIORegistryIterateRecursively, &it));
    io_registry_entry_t reg;
    while(reg = IOIteratorNext(it)) {
        io_name_t name_;
        assert(!IORegistryEntryGetNameInPlane(reg, kIOServicePlane, name_));
        if(!strcmp(name, name_)) {
            CFTypeRef thing = IORegistryEntryCreateCFProperty(reg, CFSTR("IODeviceMemory"), NULL, 0);
            thing = CFArrayGetValueAtIndex(thing, 0);
            thing = CFArrayGetValueAtIndex(thing, 0);
            CFNumberRef number = CFDictionaryGetValue(thing, CFSTR("address"));
            CFNumberGetValue(number, kCFNumberSInt32Type, &vram_phys);
            number = CFDictionaryGetValue(thing, CFSTR("length"));
            CFNumberGetValue(number, kCFNumberSInt32Type, &vram_length);
            CFRelease(thing);
            break;
        }
    }

    if(!vram_phys) {
        printf("Error: Couldn't find vram.\n");
        return 1;
    }

    printf("%s phys: %08x\n", name, vram_phys);
    printf("%s size: %08x\n", name, vram_length);

    char buf[0x1000];
    unsigned int virt = 0x00000000;
    do {
        unsigned int phys = virt2phys(virt);
        if(phys == vram_phys) {
            printf("%s virt: %08x\n", name, virt);
            vram_virt = virt;
            // no break
        }
    } while(virt += 0x1000);


    return 0;
}
