include ../env.mk
all: machdump pmap
essential: machdump
machdump: machdump.c
	$(GCC_NATIVE) -o machdump machdump.c
pmap: pmap.c ../config/config.cflags
	$(GCC) -o pmap pmap.c
pmap_: pmap_.c ../config/config.cflags
	$(GCC) -o pmap_ pmap_.c
pmap2: pmap2.c ../config/config.cflags
	$(GCC_UNIVERSAL) -o pmap2 pmap2.c -I../cry/headers -std=gnu99 -framework IOKit -framework CoreFoundation -framework IOSurface
	ldid -S pmap2
pmaparb: pmaparb.c ../config/config.cflags
	$(GCC_UNIVERSAL) -o pmaparb pmaparb.c -I../cry/headers -std=gnu99 -framework IOKit -framework CoreFoundation -framework IOSurface
	ldid -S pmaparb

clean:
	rm -f machdump pmap
