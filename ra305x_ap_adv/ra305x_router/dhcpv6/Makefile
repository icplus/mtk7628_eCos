

#
# Makefile for Ralink Tech.
#

#.EXPORT_ALL_VARIABLES:
include ../.config
CURR_DIR=$(shell pwd)
EXTRA_CFLAGS =-DINET6  -DHAVE_ARC4RANDOM  -I$(CURR_DIR) -DIOCTL_OK_ON_RAW_SOCKET -I../tcpip/include/ -D__FreeBSD__

ALL_OBJS = common.o if.o config.o timer.o lease.o \
	base64.o auth.o 

ifdef CONFIG_DHCPV6_SERVER
ALL_OBJS += dhcp6s.o 
endif 

ifdef CONFIG_DHCPV6_CLIENT
ALL_OBJS += dhcp6c.o  prefixconf.o dhcp6c_ia.o \
	  addrconf.o  
endif 

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf




