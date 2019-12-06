#
# Makefile for Ralink Tech. PPPD
#

#.EXPORT_ALL_VARIABLES:

include ../.config

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -D_KERNEL -I$(CURR_DIR)/include -I$(CURR_DIR)/../pppd/include

PPPOE_OBJS = $(CURR_DIR)/src/ng_pppoe.o

ALL_OBJS = $(PPPOE_OBJS)

MODULE_OBJS = $(PPPOE_OBJS)



MODULE_OBJS += $(PPPOE_JG)

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE) $(MODULE_OBJS) $(ALL_OBJS)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(MODULE_OBJS) 

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf


