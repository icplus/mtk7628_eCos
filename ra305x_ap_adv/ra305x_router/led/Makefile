# Makefile

include $(TOPDIR)/.config
include $(TOPDIR)/rules.mak

ALL_OBJS = led_main.o

MODULE = $(shell pwd | sed "s/.*\///" ).o


all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
