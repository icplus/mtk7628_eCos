# Makefile

include $(TOPDIR)/.config
TEMPZ      = --cref -Map zloadtest.map -O2
CFLAGS_uncompress.o := -O2 -I. -fno-builtin -G0

ALL_OBJS = load.o uncompress.o LzmaDec.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS) zwebmap.h
	$(XLD) -r $(ENDIAN) $(TEMPZ) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
