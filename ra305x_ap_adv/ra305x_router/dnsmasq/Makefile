#
# Makefile
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include -I$(CURR_DIR)/../dns/include

ALL_OBJS =  $(CURR_DIR)/src/dnsmasq_lib.o $(CURR_DIR)/src/dnsmasq.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	rm $(MODULE) $(ALL_OBJS)
