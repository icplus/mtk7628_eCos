#
# Makefile for Ralink Tech.
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -D_KERNEL -DHAVE_ARC4RANDOM  -I$(CURR_DIR)  -I../tcpip/include/ -D__FreeBSD__

ALL_OBJS = rtadvd_main.o rrenum.o if.o config.o timer.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf


