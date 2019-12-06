#
# Makefile for Ralink IPFILTER
#
CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -I$(CURR_DIR)/include -DINET -DKERNEL -D_KERNEL -D__INSIDE_NET

MAINOBJ = ip_fil.o fil.o ip_nat.o ip_frag.o ip_state.o ip_proxy.o ip_fastnat.o ip_def.o
IPNAT = ipnat.o natparse.o common.o printnat.o natcmd.o
FILS=printstate.o
IPF=parse.o opt.o facpri.o filcmd.o
MACFIL=macfilter.o 
URLFIL=urlfilter.o
DNSFIL=dnsfilter.o

ifdef CONFIG_IPFILTER_LOG
EXTRA_CFLAGS += -DIPFILTER_LOG
MAINOBJ += ip_log.o
endif

ALL_OBJS = $(MAINOBJ) $(IPNAT) $(IPF) $(FILS) $(MACFIL) $(URLFIL) $(DNSFIL)

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)


