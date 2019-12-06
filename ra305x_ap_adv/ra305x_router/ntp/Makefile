#
# Makefile for Xrouter NTP
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include -I$(CURR_DIR)/../sntp/include -DNTP

ALL_OBJS =  $(CURR_DIR)/src/corrtime.o $(CURR_DIR)/src/ntpc.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	rm $(MODULE) $(ALL_OBJS)
