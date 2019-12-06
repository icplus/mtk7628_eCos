#
# Makefile for Ralink Tech. PPTP
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include -I$(CURR_DIR)/../pppd/include -DMPPE


ALL_OBJS =  $(CURR_DIR)/src/pptpc.o $(CURR_DIR)/src/pptp_ctrlconn.o $(CURR_DIR)/src/pptp_gre.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	rm $(MODULE) $(ALL_OBJS)
