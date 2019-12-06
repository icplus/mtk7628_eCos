#
# Makefile
#
include ../.config

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include

ifdef CONFIG_DHCPC
DHCPC = ./src/dhcpc_api.o ./src/dhcpc.o ./src/options.o ./src/packet.o
else
DHCPC = 
endif

ALL_OBJS = $(DHCPC)

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)


