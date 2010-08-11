/*
echo '0b e0' | xxd -r -ps | dd of=/dev/kmem bs=1 seek=$((0xc017a756))
echo '00 20 00 20' | xxd -r -ps | dd of=/dev/kmem bs=1 seek=$((0xc017a840))
*/
#include <fcntl.h>
#include <mach/mach.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <mach-o/loader.h>

int main() {
    int fd = open("/dev/kmem", O_RDWR);
    char patch1[] = {0x0b, 0xe0};
    char patch2[] = {0x00, 0x20, 0x00, 0x20};
    // allow task_for_pid 0, so we can allocate some memory
    assert(pwrite(fd, patch1, sizeof(patch1), 0xc017a756) == sizeof(patch1));
    assert(pwrite(fd, patch2, sizeof(patch2), 0xc017a840) == sizeof(patch2));

    mach_port_t task;
    kern_return_t ret;
    ret = task_for_pid(mach_task_self(), 0, &task);
    assert(!ret);

    vm_deallocate(task, (vm_address_t) 0xf0000000, 0x1000);
    vm_deallocate(task, (vm_address_t) 0xf0001000, 0x1000);
    vm_deallocate(task, (vm_address_t) 0xf0002000, 0x1000);
    vm_deallocate(task, (vm_address_t) 0xf0003000, 0x1000);

    int nfd = open("nullfs.dylib", O_RDONLY);
    off_t offset = 0;
    struct mach_header hdr;
    assert(pread(nfd, &hdr, sizeof(hdr), offset) == sizeof(hdr));
    offset += sizeof(hdr);

    struct load_command lc;
    struct segment_command sc;

    unsigned int fe, ft;

    for(int i = 0; i < hdr.ncmds; i++) {
        assert(pread(nfd, &lc, sizeof(lc), offset) == sizeof(lc));
        if(lc.cmd == LC_SEGMENT) {
            assert(pread(nfd, &sc, sizeof(sc), offset) == sizeof(sc));
            void *buf = malloc(sc.filesize);
            assert(pread(nfd, buf, sc.filesize, sc.fileoff) == sc.filesize);

            unsigned int *probe = (unsigned int *)buf;
            unsigned int probe_br = sc.filesize & ~3;
            while(probe_br > 0) {
                unsigned int magic = *probe++;
                if(magic == 0xdeadf00d) {
                    fe = *probe++;
                    ft = *probe++;
                }
                probe_br -= 4;
            }
            if((sc.vmaddr & 0xf0000000) == 0xf0000000) {
                vm_address_t address = (vm_address_t) sc.vmaddr;
                vm_size_t size = sc.vmsize;
                ret = vm_allocate(task, &address, size, false);
                printf("map @ %x size:%x ==> %d\n", (int) address, (int) size, ret);
                assert(!ret);
                assert(address == (vm_address_t) sc.vmaddr);
                ret = vm_write(task, (vm_address_t) sc.vmaddr, (pointer_t) buf, sc.filesize);
                assert(!ret);
            } else {
                
                printf("writing to %x size=%d\n", (int) sc.vmaddr, (int) sc.filesize);
                assert(pwrite(fd, buf, sc.filesize, (off_t) sc.vmaddr) == sc.filesize);
            }

            free(buf);
        }
        offset += lc.cmdsize;
    }
    
    printf("fe:%x ft:%x\n", (int) fe, (int) ft);
    assert(fe);
    
    FILE *fp = fopen("loader.txt", "r");
    char buf[19];
    while(fgets(buf, sizeof(buf), fp)) {
        unsigned int addr, to;
        sscanf(buf, "%x %x\n", &addr, &to);
        printf("%x -> %x\n", addr, to);
        //assert((addr & 0xf0000000) == 0xf0000000);
        ret = vm_write(task, (vm_address_t) addr, (pointer_t) &to, 4);
        assert(!ret);
    }
    assert(feof(fp));

    printf("ok\n");

    offset = sizeof(hdr);
    for(int i = 0; i < hdr.ncmds; i++) {
        assert(pread(nfd, &lc, sizeof(lc), offset) == sizeof(lc));
        if(lc.cmd == LC_SEGMENT) {
            assert(pread(nfd, &sc, sizeof(sc), offset) == sizeof(sc));
            if((sc.vmaddr & 0xf0000000) == 0xf0000000) {
                printf("protect @ %x+%x: %d,%d\n", (int) sc.vmaddr, sc.vmsize, sc.maxprot, sc.initprot);
                ret = vm_protect(task, (vm_address_t) sc.vmaddr, sc.vmsize, true, sc.maxprot);
                assert(!ret);
                ret = vm_protect(task, (vm_address_t) sc.vmaddr, sc.vmsize, false, sc.initprot);
                assert(!ret);
                vm_machine_attribute_val_t flush = MATTR_VAL_ICACHE_FLUSH;
                ret = vm_machine_attribute(task, (vm_address_t) sc.vmaddr, sc.vmsize,MATTR_CACHE, &flush);
                assert(!ret);
                //ret = vm_wire(mach_host_self(), task, (vm_address_t) sc.vmaddr, sc.vmsize, VM_PROT_READ | VM_PROT_WRITE);
                //assert(!ret);
            }
        }
        offset += lc.cmdsize;
    }
    
    //return 0;
    
    // nobody uses this function anyway I suppose
    // (you can't create a new thread for the kernel task.)
    off_t scratch = (off_t) 0xc017a644;
    
    struct stuff {
        char insns[8];
        unsigned int arg0;
        unsigned int arg1;
        unsigned int func;
    } __attribute__((packed));
    struct stuff stuff;

    char orig[sizeof(stuff)];
    assert(pread(fd, orig, sizeof(stuff), scratch) == sizeof(stuff));
    
    memcpy(&stuff.insns, (char[]) {0x01, 0x48, 0x02, 0x49, 0xdf, 0xf8, 0x08, 0xf0 }, sizeof(stuff.insns));
    stuff.arg0 = fe;
    stuff.arg1 = ft;
    stuff.func = 0xc009162d; // _vfs_fsadd

    assert(pwrite(fd, &stuff, sizeof(stuff), scratch) == sizeof(stuff));
    
    {vm_machine_attribute_val_t flush = MATTR_VAL_DCACHE_FLUSH;
    ret = vm_machine_attribute(task, (vm_address_t) (scratch & ~0xfff), 0x1000, MATTR_CACHE, &flush);
    assert(!ret);}
 
    {vm_machine_attribute_val_t flush = MATTR_VAL_ICACHE_FLUSH;
    ret = vm_machine_attribute(task, (vm_address_t) (scratch & ~0xfff), 0x1000, MATTR_CACHE, &flush);
    assert(!ret);}

    sleep(2);

    mach_port_t task_;
    printf("=> %d\n", task_name_for_pid(mach_task_self(), 999, &task_));

    assert(pwrite(fd, orig, sizeof(stuff), scratch) == sizeof(stuff));
   
}
