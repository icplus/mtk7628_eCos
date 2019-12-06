#
# Makefile for Ralink Tech.
#

#.EXPORT_ALL_VARIABLES:

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -DINET6 -DPIM -DIOCTL_OK_ON_RAW_SOCKET -DHAVE_GETIFADDRS -D_KERNEL -I$(CURR_DIR) -I../tcpip/include/ -D__FreeBSD__

ALL_OBJS = mld6.o mld6_proto.o inet6.o kern.o config.o routesock.o callout.o route.o vif.o timer.o mrt.o pim6.o pim6_proto.o trace.o main.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf


