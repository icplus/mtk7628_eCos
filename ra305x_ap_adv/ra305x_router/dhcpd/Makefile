#
# Makefile
#
include ../.config

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include

DHCPD = ./src/leases.o ./src/packet.o ./src/common_subr.o ./src/dhcpd.o

ALL_OBJS = $(DHCPD) ./src/dynamic_leases.o

MODULE_OBJS = $(DHCPD)

ifdef	DHCPD_SAVE_DYNAMIC_CLIENTS
MODULE_OBJS += ./src/dynamic_leases.o
endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE) $(MODULE_OBJS) $(ALL_OBJS)

include $(TOPDIR)/rules.mak

$(MODULE): $(MODULE_OBJS)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(MODULE_OBJS)

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf


