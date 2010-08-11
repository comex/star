include ../env.mk
all: one.dylib .ran_zero 
.ran_zero: zero.py one.dylib ../installui/installui.dylib ../config/config.json goop.bin
	$(PYTHON_IF_NECESSARY) ./zero.py
	touch .ran_zero
one.dylib: one.py zero.py ../config/config.json goop.bin
	$(PYTHON_IF_NECESSARY) ./one.py
goop.bin: goop.S
	$(GCC) -mthumb -c -o goop.o goop.S
	../misc/machdump goop.o goop.bin
clean:
	rm -f .ran_zero one.dylib *.pyc
