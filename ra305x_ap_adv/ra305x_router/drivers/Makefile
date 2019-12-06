#
# Makefile for XRouter USBhost
#

include $(TOPDIR)/.config

CURR_DIR=$(shell pwd)
#EXTRA_CFLAGS = -DGATEWAY -DIPFILTER -D_KERNEL -D__INSIDE_NET

ifdef	CONFIG_USBHOST
#  EXTRA_CFLAGS = -DMUSB_MHDRC -DMUSB_HOST -DMUSB_MSD -DMUSB_HFI_SUPPORT
#    -DMTK_MHDRC \
#    -DMUSB_HDRC \
#   -DMUSB_HOST \
#    -DMUSB_HSHC \
#    -DMUSB_MSD \
#    -DMUSB_HUB \
#    -DMUSB_HFI_SUPPORT

EXTRA_CFLAGS = -I../pppd/include -I$(CURR_DIR)/usbhost/include -I$(CURR_DIR)/usbhost/include/usb -I$(CURR_DIR)/usbhost/include/class -I$(CURR_DIR)/usbhost/include/inc -I$(CURR_DIR)/usbhost/src/cd\
               -I$(CURR_DIR)/usbhost/src/host -I$(CURR_DIR)/usbhost/src -I$(CURR_DIR)/usbhost/src/drivers/msd -I$(CURR_DIR)/usbhost/src/drivers/dongle -I$(CURR_DIR)/usbhost/src/drivers/sicd \
               -I$(CURR_DIR)/usbhost/include/53xx_inc/inc -I$(CURR_DIR)/usbhost/src/systems/ecos -I$(CURR_DIR)/usbhost/include/nptv_inc/inc/drv -I$(CURR_DIR)/usbhost/include/nptv_inc/inc/hw

EXTRA_CFLAGS += -DMTK_MHDRC -DMUSB_HDRC -DMUSB_HOST -DMUSB_MSD -DMUSB_HFI_SUPPORT
          
ifeq ($(CHIPSET),7620) 
  EXTRA_CFLAGS += -DCONFIG_RALINK_MT7620
endif
               
CORE_DIR = ./usbhost/src/cd
CORE = $(CORE_DIR)/mu_cntlr.o $(CORE_DIR)/mu_descs.o $(CORE_DIR)/mu_drc.o \
   $(CORE_DIR)/mu_hdr.o $(CORE_DIR)/mu_hsdma.o $(CORE_DIR)/mu_hst.o $(CORE_DIR)/mu_mdr.o \
   $(CORE_DIR)/mu_list.o $(CORE_DIR)/mu_pip.o $(CORE_DIR)/mu_hc.o $(CORE_DIR)/mu_fun.o

HOST_DIR = ./usbhost/src/host
HOST = $(HOST_DIR)/ehci-hub.o $(HOST_DIR)/ehci-hcd.o $(HOST_DIR)/ehci-rt3xxx.o \
     $(HOST_DIR)/ehci-lpm.o $(HOST_DIR)/ehci-mem.o $(HOST_DIR)/ehci-q.o \
     $(HOST_DIR)/ohci-rt3xxx.o $(HOST_DIR)/ohci-hcd.o $(HOST_DIR)/ohci-hub.o \
     $(HOST_DIR)/ohci-mem.o $(HOST_DIR)/ohci-q.o 

ifeq ($(CHIPSET),7628)
  EXTRA_CFLAGS += -DCONFIG_RALINK_MT7628
  HOST += $(HOST_DIR)/mtk/mtk-phy-ahb.o $(HOST_DIR)/mtk/mtk-phy-7628.o $(HOST_DIR)/mtk/mtk-phy.o
endif
     
HUBDRIVER_DIR = ./usbhost/src/drivers/hub
HUBDRIVER = $(HUBDRIVER_DIR)/mu_hcore.o $(HUBDRIVER_DIR)/mu_hcreq.o \
  $(HUBDRIVER_DIR)/mu_hub.o $(HUBDRIVER_DIR)/mu_hapi.o 
MASTERSTORAGE_DIR = ./usbhost/src/drivers/msd
MASTERSTORAGE = $(MASTERSTORAGE_DIR)/mu_mapi.o $(MASTERSTORAGE_DIR)/mu_mbot.o $(MASTERSTORAGE_DIR)/mu_mcbt.o $(MASTERSTORAGE_DIR)/mu_mscsi.o 
ifdef CONFIG_USBDONGLE_SUPPORT
DONGLE_DIR = ./usbhost/src/drivers/dongle
DONGLE = $(DONGLE_DIR)/mu_dongle.o $(DONGLE_DIR)/mu_pppd.o $(DONGLE_DIR)/mu_comgt.o $(DONGLE_DIR)/dongle_frame.o \
         $(DONGLE_DIR)/dongle_l2tp.o
endif
LIB_DIR = ./usbhost/src/lib
LIB = $(LIB_DIR)/mu_bits.o $(LIB_DIR)/mu_stdio.o \
	$(LIB_DIR)/mu_strng.o $(LIB_DIR)/mu_ramfl.o $(LIB_DIR)/mu_stack.o
SYSTEM_DIR =./usbhost/src/systems
SYSTEM = $(SYSTEM_DIR)/mu_hfios.o $(SYSTEM_DIR)/mu_hfisicdos.o
SYSTEMECOS_DIR = ./usbhost/src/systems/ecos
SYSTEM += $(SYSTEMECOS_DIR)/mu_sys.o 
SYSTEMECOSAFS_DIR = ./usbhost/src/systems/ecos
SYSTEM +=	$(SYSTEMECOSAFS_DIR)/brd_main.o
endif

#ALL_OBJS = board.o leddrv.o swrst.o watchdog.o flshdrv.o
ALL_OBJS = board.o  flshdrv.o

ifdef CONFIG_RST
	MODULE_OBJS += swrst.o
endif

ifdef	CONFIG_USBHOST
ALL_OBJS += $(CORE) $(HOST) $(HUBDRIVER) $(MASTERSTORAGE) $(LIB) $(SYSTEM) $(CURR_DIR)/usbhost/src/musb_if.o
ifdef CONFIG_USBDONGLE_SUPPORT
ALL_OBJS += $(DONGLE)
endif
endif

MODULE_OBJS = board.o flshdrv.o
ifdef	CONFIG_USBHOST
MODULE_OBJS += $(CORE) $(HOST) $(HUBDRIVER) $(MASTERSTORAGE) $(LIB) $(SYSTEM) $(CURR_DIR)/usbhost/src/musb_if.o
ifdef CONFIG_USBDONGLE_SUPPORT
MODULE_OBJS += $(DONGLE)
endif
endif

ifdef CONFIG_LED
#	MODULE_OBJS += leddrv.o
endif

ifdef CONFIG_RST
	MODULE_OBJS += swrst.o
endif

ifdef CONFIG_WATCHDOG
	MODULE_OBJS += watchdog.o
endif


MODULE = $(shell pwd | sed "s/.*\///" ).o

CFLAGS_flshdrv.o = -O0

all: $(MODULE) $(ALL_OBJS)
include $(TOPDIR)/rules.mak
$(MODULE):  $(ALL_OBJS:.o=.d) $(MODULE_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(MODULE_OBJS)

#all: $(MODULE)
#include $(TOPDIR)/rules.mak
#$(MODULE):  $(ALL_OBJS:.o=.d)
#	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
