ROOT := $(realpath $(shell dirname $(lastword $(MAKEFILE_LIST))))
export PYTHONPATH = $(ROOT)/config
ifeq "$(shell arch)" "arm"
GCC_BIN = gcc 
GCC = $(GCC_BASE)
GCC_UNIVERSAL = not_bothering_to_figure_this_out
GCC_NATIVE = $(GCC_BASE)
PYTHON_IF_NECESSARY := python
else
BIN = /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin
GCC_BIN = $(BIN)/gcc-4.2
ifeq "" "$(findstring -DCONFIG_ARCH=armv7,$(shell cat $(ROOT)/config/config.cflags))"
GCC = $(GCC_BASE) -arch armv6
else
GCC = $(GCC_BASE) -arch armv7
endif
GCC_UNIVERSAL = $(GCC_BASE) -arch armv6 -arch armv7
GCC_NATIVE = gcc
endif

CFLAGS = `cat $(ROOT)/config/config.cflags` 
GCC_BASE = $(GCC_BIN) -Os $(CFLAGS) -Wimplicit -isysroot /var/sdk -F/var/sdk/System/Library/Frameworks -F/var/sdk/System/Library/PrivateFrameworks
