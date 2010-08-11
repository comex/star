import os, mmap

# In the file 'filename', search for instances of the binary strings in the values of patterns (which is a dict).

def search_for_things(filename, patterns):
    size = os.path.getsize(filename)
    fd = os.open(filename, 0)
    stuff = mmap .mmap(fd, 0, prot=mmap.PROT_READ)

    results = {}
    for k, v in patterns.items():
        results[k] = stuff.find(v)

    os.close(fd)
    return results
