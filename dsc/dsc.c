// DySlim is complicated and requires writing 6GB to disk (if only temporarily).
// This lets you mount the dyld shared cache via FUSE; the resulting files are weird but readable by things like otool and strings.
//
// gcc -std=gnu99 -I/opt/local/include -L/opt/local/lib -D_FILE_OFFSET_BITS=64 -o dsc dsc.c -lfuse -framework CoreFoundation

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <mach/shared_region.h> // struct shared_file_mapping_np
#include <mach-o/loader.h>
#include <errno.h>
#include <CoreFoundation/CoreFoundation.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

// from dyld
struct dyld_cache_header
{
    char        magic[16];              // e.g. "dyld_v0     ppc"
    uint32_t    mappingOffset;          // file offset to first shared_file_mapping_np
    uint32_t    mappingCount;           // number of shared_file_mapping_np entries
    uint32_t    imagesOffset;           // file offset to first dyld_cache_image_info
    uint32_t    imagesCount;            // number of dyld_cache_image_info entries
    uint64_t    dyldBaseAddress;        // base address of dyld when cache was built
};

struct dyld_cache_image_info
{
    uint64_t    address;
    uint64_t    modTime;
    uint64_t    inode;
    uint32_t    pathFileOffset;
    uint32_t    pad;
};


struct dyld_cache_header ch;
int fd;
off_t cache_size;

static int dc_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    int path_len = strlen(path);
    bool ends_with_slash = path[path_len-1] == '/';

    lseek(fd, ch.imagesOffset, SEEK_SET);
    for(int i = 0; i < ch.imagesCount; i++) {
        struct dyld_cache_image_info ii;
        assert(pread(fd, &ii, sizeof(ii), ch.imagesOffset + i*sizeof(ii)) == sizeof(ii));
        char stuff[256];
        pread(fd, &stuff, sizeof(stuff), ii.pathFileOffset);
        if(!strcmp(stuff, path)) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = cache_size + 20*1024*1024; // LINKEDIT extends beyond the end of the cache!?
            stbuf->st_ino = ii.inode;
            return 0;
        }
        if(!memcmp(stuff, path, path_len) && (ends_with_slash || stuff[path_len] == '/')) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }

    return -ENOENT;
}

static int dc_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    int path_len = strlen(path);
    bool ends_with_slash = path[path_len-1] == '/';

    lseek(fd, ch.imagesOffset, SEEK_SET);

    CFMutableSetRef heard_of = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);

    for(int i = 0; i < ch.imagesCount; i++) {
        struct dyld_cache_image_info ii;
        assert(pread(fd, &ii, sizeof(ii), ch.imagesOffset + i*sizeof(ii)) == sizeof(ii));
        char stuff[256];
        pread(fd, &stuff, sizeof(stuff), ii.pathFileOffset);

        if(!memcmp(stuff, path, path_len) && (ends_with_slash || stuff[path_len] == '/')) {
            char *ent = strdup(stuff + path_len + (ends_with_slash?0:1));
            char *slash = strchr(ent, '/'); if(slash) *slash = 0;
            CFStringRef ents = CFStringCreateWithCString(NULL, ent, kCFStringEncodingASCII);
            if(!CFSetContainsValue(heard_of, ents)) {
                CFSetAddValue(heard_of, ents);
                filler(buf, ent, NULL, 0);
            }
            CFRelease(ents);
        }
    }

    CFRelease(heard_of);

    return 0;
}

static int dc_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "open\n");
    lseek(fd, ch.imagesOffset, SEEK_SET);
    for(int i = 0; i < ch.imagesCount; i++) {
        struct dyld_cache_image_info ii;
        assert(pread(fd, &ii, sizeof(ii), ch.imagesOffset + i*sizeof(ii)) == sizeof(ii));
        char stuff[256];
        pread(fd, &stuff, sizeof(stuff), ii.pathFileOffset);

        if(!strcmp(stuff, path)) {
            fi->fh = ii.address;
            return 0;
        }
    }
    return -ENOENT;
}

static int dc_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    if(offset < 0x5000) {
        // Beginning of file; either from the client assuming the mach-o will start at 0, or the initial LC_SEGMENT with fileoff=0.
        // Assume this lies within a single shared_file_mapping_np
        offset += fi->fh;
        for(int i = 0; i < ch.mappingCount; i++) {
            struct shared_file_mapping_np sfm;
            assert(pread(fd, &sfm, sizeof(sfm), ch.mappingOffset + i*sizeof(sfm)) == sizeof(sfm));
            if(offset >= sfm.sfm_address && (offset + size) < (sfm.sfm_address + sfm.sfm_size)) {
                return pread(fd, buf, size, offset - sfm.sfm_address + sfm.sfm_file_offset);
            }
        }
    } else {
        return pread(fd, buf, size, offset);
    }

    return 0;
}

static struct fuse_operations dc_oper = {
    .getattr	= dc_getattr,
    .readdir	= dc_readdir,
    .open	= dc_open,
    .read	= dc_read,
};


int main(int argc, char **argv) {
    assert(argc > 1);
    bool verbose = false;
    if(!strcmp(argv[1], "-v")) {
        verbose = true;
        memcpy(argv + 1, argv + 2, (argc - 2) * sizeof(char *));
        argc--;
        assert(argc > 1);
    }
    fd = open(argv[1], O_RDONLY);
    cache_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    assert(read(fd, &ch, sizeof(ch)) == sizeof(ch));
    printf("magic:'%.16s'  mappingOffset:%u mappingCount:%u imagesOffset:%u imagesCount:%u dyldBaseAddress:%llu\n",
        ch.magic, ch.mappingOffset, ch.mappingCount,
        ch.imagesOffset, ch.imagesCount, ch.dyldBaseAddress);

    argv[1] = argv[0];
    if(verbose) {
        lseek(fd, ch.imagesOffset, SEEK_SET);
        int i;
        for(i = 0; i < ch.imagesCount; i++) {
            struct dyld_cache_image_info ii;
            assert(read(fd, &ii, sizeof(ii)) == sizeof(ii));
            char stuff[128];
            pread(fd, &stuff, sizeof(stuff), ii.pathFileOffset);
            printf("%llx %.128s\n", ii.address, stuff);
        }
    }
    return fuse_main(argc - 1, argv + 1, &dc_oper, NULL);
    //return 0;
}
