// Dump the first section of a Mach-O file.
// objcopy could do this, but MacPorts doesn't have a working ARM one
// otool could... probably do this
// Anyway, this is more portable.
#include <mach-o/loader.h>
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    assert(argc > 2);
    int fd = open(argv[1], O_RDONLY);
    assert(fd > 0);
    off_t size = lseek(fd, 0, SEEK_END);
    assert(size != 0);
    char *input = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(input != MAP_FAILED);
    uint32_t offset = sizeof(struct mach_header);
    int ncmds = ((struct mach_header *) input)->ncmds;
    while(ncmds--) {
        struct load_command *lc = (void *) (input + offset);
        if(lc->cmd == LC_SEGMENT) {
            struct segment_command *sc = (void *) lc;
            int nsects = sc->nsects;
            struct section *sec = (void *) (input + offset + sizeof(*sc));
            while(nsects--) {
                if(sec->size) {
                    int fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    assert(fd2 > 0);
                    write(fd2, input + sec->offset, sec->size);
                    close(fd2);
                    return 0;
                }
                sec++;
            }
        }
        offset += lc->cmdsize;
    }
    fprintf(stderr, "Couldn't find any segments in %s\n", argv[1]);
    return 1;
}
