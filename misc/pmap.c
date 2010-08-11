#define _FILE_OFFSET_BITS 64
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
int kmem_fd = 0;

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

static unsigned int virt2phys(unsigned int virt, int verbose) {
    // Note: assuming addr_hi is 0
    unsigned int pmap = read32(CONFIG_KERNEL_PMAP);
    //printf("[pmap]=%x [pmap+408]=%x\n", read32(pmap), read32(pmap + 0x408));
    unsigned int gVirtBase = read32(CONFIG_MEM_SIZE - 12);
    unsigned int gPhysBase = read32(CONFIG_MEM_SIZE - 8);
    //printf("gVirtBase = %x     gPhysBase = %x\n", gVirtBase, gPhysBase);
    //printf("weirdo is %x\n", read32(pmap + 0x408));
    unsigned int r0 = read32(read32(pmap) + 4*((virt - read32(pmap + 0x408)) >> 20));
    //printf("%x %x\n", read32(pmap), 4*((virt - read32(pmap + 0x408)) >> 20));
    if(verbose) {
        printf("virt = %x\n", virt);
        printf("r0 = (%d) %x\n", r0 & 3,r0);
    }
    if(!r0) return 0;

    // weirdo is 0, so it's like
    // XXXYY000

    switch(r0 & 3) {
    case 1:
        r0 = (r0 & 0xfffffc00) + (gVirtBase - gPhysBase) + 4*((virt >> 12) & 0xff);
        if(!r0) return 0;
        if(verbose) {
            printf("second-level desc: %x\n", read32(r0));
        }
        return (read32(r0) & 0xfffff000) | (virt & 0xfff); // lolwhat
        // ^- is from the ARM ARM.  It is completely differen thtan v :/
        unsigned int x = (r0 & 0xfff) >> 2;
        unsigned int r3 = read32((r0 & 0xfffff000) + 12*x + 0x408);
        printf("[1] r0 = %x r3 = %x virt = %x\n", r0, r3, virt);
        if(r3 == 0) {
            return read32(r0);
        } else {
            return (virt & 0xfffff000) ^ r3;
        }
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

int main() {
    assert(sizeof(off_t) == 8);
    kmem_fd = open("/dev/kmem", O_RDWR);
    assert(kmem_fd > 0);
    char buf[0x1000];
    unsigned int crap = 0x00000000;
    do {
        unsigned int phys = virt2phys(crap, 0);
        if(phys) {
            printf("%x -> %x\n", crap, phys);
        }
    } while(crap += 0x1000);
    return 0;
}
