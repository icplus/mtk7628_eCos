#
# Makefile for XRouter PPPD
#
#.EXPORT_ALL_VARIABLES:

include ../.config

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -D_KERNEL -I$(CURR_DIR)/include -DMPPE -DCHAPMS

PPP_OBJS = \
		$(CURR_DIR)/src/pppmain.o  $(CURR_DIR)/src/options.o \
		$(CURR_DIR)/src/sys-ecos.o $(CURR_DIR)/src/ppp_eth.o $(CURR_DIR)/src/ppp_api.o \
		$(CURR_DIR)/src/fsm.o \
		$(CURR_DIR)/src/lcp.o $(CURR_DIR)/src/magic.o \
		$(CURR_DIR)/src/auth.o $(CURR_DIR)/src/chap.o $(CURR_DIR)/src/upap.o \
		$(CURR_DIR)/src/ipcp.o $(CURR_DIR)/src/sha1.o

CHAPMS_OBJS = \
		$(CURR_DIR)/src/chap_ms.o $(CURR_DIR)/src/crypt.o $(CURR_DIR)/src/extra_crypto.o

MPPE_OBJS = \
		$(CURR_DIR)/src/ppp_ccp.o $(CURR_DIR)/src/ccp.o \
		$(CURR_DIR)/src/ppp_mppe.o $(CURR_DIR)/src/mppe.o \
		$(CURR_DIR)/src/rc4_skey.o $(CURR_DIR)/src/getkey.o

ALL_OBJS = $(PPP_OBJS) $(CHAPMS_OBJS) $(MPPE_OBJS)

MODULE_OBJS = $(PPP_OBJS)

ifdef	CONFIG_CHAPMS
MODULE_OBJS += $(CHAPMS_OBJS)
endif

ifdef	CONFIG_MPPE
MODULE_OBJS += $(MPPE_OBJS)
endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE) $(MODULE_OBJS) $(ALL_OBJS)

include $(TOPDIR)/rules.mak

$(MODULE): $(MODULE_OBJS)
	$(XLD) -r $(ENDIAN) -o $@ $(MODULE_OBJS)

clean:
	find . -name '*.[od] -type f -print | xargs rm -rf

