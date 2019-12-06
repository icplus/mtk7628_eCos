#
# Makefile for Ralink Tech. Web Authentication
#

ALL_OBJS =  auth_check.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE) 

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)

.PHONY: all clean

