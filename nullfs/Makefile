include ../env.mk
all: loader loader.txt nullfs.dylib
loader.txt: nullfs.dylib nm.py
	python nm.py 
loader: loader.c 
	$(GCC) -std=gnu99 -o loader loader.c
	ldid -S loader
nullfs.dylib: null_subr.o null_vfsops.o null_vnops.o 
	$(GCC) -dynamiclib -undefined dynamic_lookup -o $@ $^ -image_base 0xf0000000 -segaddr __TEXT 0xc06ed000

#           If the
#           directory dir is a standard system include directory, the option is ignored to ensure that
#           the default search order for system directories and the special treatment of system headers
#           are not defeated .
#           arggghhhh...
#

%.o: %.c Makefile null.h
	$(GCC) -dynamiclib -c -o $@ $< -DKERNEL -DXNU_KERNEL_PRIVATE -DPRIVATE -DBSD_KERNEL_PRIVATE -DKERNEL_PRIVATE -D__APPLE_API_PRIVATE -DNULLFS_DIAGNOSTIC -DDIAGNOSTIC=1 -Ifink/  -Ixnu/bsd -Ixnu/osfmk -I.

clean:
	rm -f *.o nullfs.dylib loader loader.txt
