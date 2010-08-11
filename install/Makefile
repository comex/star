# install.c is in the download.  installui.m is bundled.
include ../env.mk
all: install.dylib wad.bin
install.dylib: install.o copier.o 
	$(GCC_UNIVERSAL) -dynamiclib -o $@ $^ -framework Foundation -framework CoreFoundation libtar.a liblzma.a -framework GraphicsServices
%.o: %.c common.h 
	$(GCC_UNIVERSAL) -Iheaders -std=gnu99 -c -o $@ $< -Werror
wad.bin: install.dylib Cydia-4.0.1r2-Raw.txz
	python wad.py $^
clean:
	rm -f install.dylib *.o wad.bin
	
