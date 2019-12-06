include $(TOPDIR)/.config

ifdef CONFIG_IPV6
EXTRA_CFLAGS +=-DHAVE_IPV6 
endif
EXTRA_CFLAGS +=-DUNICAST_DISABLED

COREDIR = mDNSCore
SHAREDDIR ?= mDNSShared
POSIXDIR = mDNSPosix
OBJDIR =mDNSPosix/objects

# Set up diverging paths for debug vs. prod builds
ifdef CONFIG_mDNS_Bonjour_DEBUG
DEBUG=1
else
DEBUG=0
endif

ifeq ($(DEBUG),1)
CFLAGS_DEBUG = -g -DMDNS_DEBUGMSGS=2 
OBJDIR = objects/debug
BUILDDIR = build/debug
STRIP = echo 
else
# We use -Os for two reasons:
# 1. We want to make small binaries, suitable for putting into hardware devices
# 2. Some of the code analysis warnings only work when some form of optimization is enabled
CFLAGS_DEBUG = -Os -DMDNS_DEBUGMSGS=0 
OBJDIR ?= objects/prod
BUILDDIR ?= build/prod
STRIP = strip -S 
endif



CURR_DIR=$(shell pwd)
EXTRA_CFLAGS += $(CFLAGS_DEBUG)-ImDNSCore -ImDNSShared -ImDNSPosix/objects -ImDNSPosix  -I../tcpip/include/ -D__FreeBSD__

CORE_OBJS =$(COREDIR)/anonymous.o $(COREDIR)/CryptoAlg.o $(COREDIR)/DNSCommon.o $(COREDIR)/DNSDigest.o \
		  $(COREDIR)/mDNS.o  $(COREDIR)/nsec3.o \
		$(COREDIR)/uDNS.o 
		#$(COREDIR)/dnssec.o $(COREDIR)/dnsproxy.o$(COREDIR)/nsec.o 
POSIX_OBJS = $(POSIXDIR)/mDNSPosix.o  $(POSIXDIR)/Responder.o $(POSIXDIR)/mDNSUNP.o 
		# $(POSIXDIR)/Identify.o  $(POSIXDIR)/nss_mdns.o \
		#$(POSIXDIR)/NetMonitor.o $(POSIXDIR)/ProxyResponder.o $(POSIXDIR)/client.o $(POSIXDIR)/PosixDaemon.o 
		
SHARE_OBJS = $(SHAREDDIR)/mDNSDebug.o $(SHAREDDIR)/GenLinkedList.o	 $(SHAREDDIR)/PlatformCommon.o	$(SHAREDDIR)/DebugServices.o   	

ALL_OBJS = $(CORE_OBJS)  $(POSIX_OBJS) $(SHARE_OBJS)

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) 

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf
