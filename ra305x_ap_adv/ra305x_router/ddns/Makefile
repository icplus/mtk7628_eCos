# Makefile

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -DHAVE_CONFIG_H -I$(CURR_DIR)/include

ALL_OBJS = ddns_main.o provider.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
