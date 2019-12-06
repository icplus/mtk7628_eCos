#
# Makefile for XRouter L2TP
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include -I$(CURR_DIR)/../pppd/include


ALL_OBJS =  $(CURR_DIR)/src/debug.o $(CURR_DIR)/src/frame.o \
			$(CURR_DIR)/src/l2tp.o \
	        $(CURR_DIR)/src/session.o $(CURR_DIR)/src/l2tp_pppd.o \
	        $(CURR_DIR)/src/tunnel.o $(CURR_DIR)/src/utils.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	rm $(MODULE) $(ALL_OBJS)
