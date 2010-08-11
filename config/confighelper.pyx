cdef extern from "sys/mman.h":
    void *mmap(void *start, size_t length, int prot, int flags, int fd, long long offset)
    int munmap(void *start, size_t len)
    cdef int PROT_READ
    cdef int MAP_SHARED
    cdef void *MAP_FAILED
cdef extern from "stdlib.h":
    void *calloc(size_t count, size_t size)
    void free(void *ptr)
    long unlikely(long thing)
import os


# In the file 'filename', search for instances of the binary strings in the values of patterns (which is a dict).

def search_for_things(filename, patterns):
    cdef int fd = os.open(filename, 0)
    cdef size_t size = os.lseek(fd, 0, 2)
    os.lseek(fd, 0, 0)
    cdef unsigned char *stuff = <unsigned char *> mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)
    if stuff == MAP_FAILED:
        raise Exception('Could not mmap')

    cdef int num_patterns = len(patterns)
    cdef signed char *progress = <signed char *> calloc(num_patterns, sizeof(signed char))
    cdef char *pattern_lens = <char *> calloc(num_patterns, sizeof(char))
    cdef unsigned char **pattern_bufs = <unsigned char **> calloc(num_patterns, sizeof(char *))
    keys = []
    results = {}
    cdef int i = 0
    for k, v in patterns.items():
        pattern_bufs[i] = v
        pattern_lens[i] = len(v)
        results[k] = None
        keys.append(k)
        i += 1
    cdef int pos = 0
    cdef unsigned char c
    cdef int n
    cdef char options[256]
    for i in range(256):
        options[i] = 0
    while pos < size:
        c = stuff[pos]   
        for i in range(num_patterns):
            n = progress[i]
            options[0] = n + 1
            n = options[pattern_bufs[i][n] ^ c]
            progress[i] = n
            if unlikely(n == pattern_lens[i]):
                results[keys[i]] = pos - n + 1
                for j in range(i, num_patterns - 1):
                    pattern_bufs[j] = pattern_bufs[j+1]
                    pattern_lens[j] = pattern_lens[j+1]
                    progress[j] = progress[j+1]
                    keys[j] = keys[j+1]
                num_patterns -= 1
                if num_patterns == 0:
                    free(progress)
                    free(pattern_bufs)
                    free(pattern_lens)
                    munmap(stuff, size)
                    os.close(fd)
                    return results
                    
        pos += 1

    free(progress)
    free(pattern_bufs)
    free(pattern_lens)
    munmap(stuff, size)
    os.close(fd)
    return results
