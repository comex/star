include ../env.mk
all: sandbox-mac-replace.bin
sandbox-mac-replace.bin: sandbox-mac-replace.s
	$(GCC) -mthumb -c -o sandbox-mac-replace.o sandbox-mac-replace.s
	../misc/machdump sandbox-mac-replace.o sandbox-mac-replace.bin
clean:
	rm -f sandbox-mac-replace.o sandbox-mac-replace.bin
