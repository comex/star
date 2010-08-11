# install.c is in the download.  installui.m is bundled.
include ../env.mk
all: installui.dylib
installui.dylib: installui.o dddata.o ../config/config.cflags common.h
	$(GCC) -dynamiclib -Iheaders -std=gnu99 -o installui.dylib installui.o dddata.o -framework Foundation -framework UIKit -framework IOKit -framework CoreGraphics -lz -Werror
%.o: %.[cm] crc32.h common.h  ../config/config.cflags
	$(GCC) -Iheaders -std=gnu99 -c -o $@ $< -Werror
clean:
	rm -f installui.dylib *.o
	
