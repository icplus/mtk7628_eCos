#	Ralink ECOS project

ifeq ($(IMAGE_NAME),)
IMAGE_NAME = eCos
endif

ifeq (.config,$(wildcard .config))
include .config
ifneq	"$(CONFIG_BSP_DIR)" "" 
	INSTALL_DIR=$(shell cd $(CONFIG_BSP_DIR) ; pwd )
else
	INSTALL_DIR=$(shell cd ../ra305x_bsp/install ; pwd )
endif

APPS_DIR=$(shell pwd )
PRJ_DIR=$(shell cat .prjname)

-include $(INSTALL_DIR)/include/pkgconf/ecos.mak

do-it-all: xrouter xrouter.map xrouter.bin zxrouter zxrouter.bin cptftp

else

do-it-all:	initmake initconfig

endif

CROSS_PREFIX=$(ECOS_COMMAND_PREFIX)
ENDIAN=$(shell echo $(ECOS_GLOBAL_CFLAGS) | sed -e "s/.*-EL.*/-EL/" )
TOPDIR=$(shell pwd)
export TOPDIR ENDIAN CROSS_PREFIX

CFLAGS        = -I$(shell pwd)/include -I$(shell pwd)/tcpip/include -I$(INSTALL_DIR)/include
CXXFLAGS      = $(CFLAGS)
LDFLAGS       = -nostartfiles -Ttarget.ld -L$(INSTALL_DIR)/lib
LDMAP          = -Wl,--cref -Wl,-Map,xrouterLink.map -Wl,-O2

CFLAGS += -D__ECOS -D_KERNEL $(ECOS_GLOBAL_CFLAGS) -include config.h
ifdef CONFIG_IPV6
CFLAGS += -DCYGPKG_NET_INET6
endif

ifeq ($(INIC_WIFI),7603E)
CFLAGS += -DCONFIG_PCI_SUPPORT #-DSECOND_WIFI -DRT$(INIC_WIFI) -DMT$(INIC_WIFI)
CONFIG_PCI_SUPPORT=y
endif

ifeq ($(WIFI_MODE),AP)
CFLAGS += -DCONFIG_AP_SUPPORT
endif

ifeq ($(WIFI_MODE),STA)
CFLAGS += -DCONFIG_STA_SUPPORT
endif

#ifeq ($(INIC_FLASH),flash)
#CFLAGS += -DCONFIG_SECOND_IF_EERPROM_FLASH
#endif

include rules.mak

ifeq ($(word 1,$(shell dos2unix -V 2>&1)),tofrodos)
DOS2UNIX=dos2unix -p
else
DOS2UNIX=dos2unix -k
endif


all: do-it-all

# LIBRARY

SUBDIRS = init cfg
DRVSUBDIRS = drivers gpio
APPSUBDIRS = cli lib tcpip

# memory debug info log: default off
#CFLAGS += -DMEM_DEBUG_LEVEL_1

# MPPE BOOST: default off
#CFLAGS += -DMPPE_BOOST_VERSION

#In virtual server,some application need ALG,such as ftp,tftp
#CFLAGS += -DVIRTUAL_SERVER_ALG
ifeq ($(BRANCH),ADV)
CFLAGS += -DBRANCH_ADV
endif
# Add by Lea
SUBDIRS += helperd

ifdef CONFIG_HTTPD
	SUBDIRS += webpages
	SUBDIRS += httpd	
endif # CONFIG_HTTPD

ifdef CONFIG_IPV6
	SUBDIRS += pim6dd
	SUBDIRS += rtadvd
endif # CONFIG_IPV6

ifdef CONFIG_RA305X
	DRVSUBDIRS += ../../ra305x_drivers/eth_ra305x
endif # CONFIG_RA305X

ifdef CONFIG_PCI_SUPPORT
	DRVSUBDIRS += ../../ra305x_drivers/pcie
endif

ifdef CONFIG_WIRELESS

ifeq ($(CHIPSET),mt7628)
	DRVSUBDIRS += ../../ra305x_drivers/Jedi_7628/embedded
else
	DRVSUBDIRS += ../../ra305x_drivers/wireless_ra305x
endif
ifdef CONFIG_ATE_DAEMON
	DRVSUBDIRS += ../../ra305x_drivers/wireless_ate
	CFLAGS +=  -DRALINK_ATE_SUPPORT
endif # CONFIG_ATE_DAEMON
ifdef CONFIG_80211X_DAEMON
	DRVSUBDIRS += ../../ra305x_drivers/wireless_rtdot1x
	CFLAGS += -DRALINK_1X_SUPPORT
endif # CONFIG_80211X_DAEMON
ifdef CONFIG_LLTD_DAEMON
	DRVSUBDIRS += ../../ra305x_drivers/lltd
	CFLAGS += -DRALINK_LLTD_SUPPORT
endif # CONFIG_LLTD_DAEMON
endif # CONFIG_WIRELESS

ifdef CONFIG_HARDWARE_WATCHDOG
	APPSUBDIRS += watchdog
endif # CONFIG_HARDWARE_WATCHDOG

ifdef CONFIG_LED
	#DRVSUBDIRS +=led
endif # CONFIG_LED

ifdef CONFIG_DHCPC
	APPSUBDIRS += dhcpc
endif # CONFIG_DHCPC

ifdef CONFIG_DHCPS
	APPSUBDIRS += dhcpd
endif # CONFIG_DHCPS

ifdef CONFIG_DHCPV6
	APPSUBDIRS +=dhcpv6
endif #CONFIG_DHCPV6

ifdef CONFIG_SYSLOG
	APPSUBDIRS += eventlog
endif #CONFIG_SYSLOG

ifneq ("$(CONFIG_NAT)$(CONFIG_FW)" , "")
	APPSUBDIRS += ipfilter
endif

ifdef CONFIG_PPPOE
	APPSUBDIRS += pppoe
	NEED_PPPD=y
endif # CONFIG_PPPOE

ifdef CONFIG_PPTP
	APPSUBDIRS += pptp
ifdef CONFIG_PPTP_BMODE	
	CFLAGS += -DPPTP_BOOST_VERSION
endif	
endif # CONFIG_PPTP

ifdef CONFIG_L2TP
	APPSUBDIRS += l2tp
endif # CONFIG_L2TP

ifdef CONFIG_SNIFFER_DAEMON
	APPSUBDIRS += sniffer
endif # CONFIG_SNIFFER_DAEMON

ifneq ("$(CONFIG_PPPOE)$(CONFIG_PPTP)$(CONFIG_L2TP)" , "")
	APPSUBDIRS += pppd
endif

ifdef CONFIG_UPNP
	APPSUBDIRS += upnpd
endif # CONFIG_UPNP

ifdef CONFIG_DDNS
	APPSUBDIRS += ddns
endif # CONFIG_DDNS

ifdef CONFIG_NTP
	APPSUBDIRS += ntp
endif # CONFIG_NTP

ifdef CONFIG_DNS
	APPSUBDIRS += dnsmasq
endif # CONFIG_DNS

ifeq	"$(CONFIG_ZLOAD_BUF)" ""
	CONFIG_ZLOAD_BUF = 0x80500000
endif
ifdef CONFIG_mDNS_Bonjour
APPSUBDIRS +=mDNSResponder-522.1.11
endif

ifdef	CONFIG_SUPPORT_FTPD
	SUBDIRS += ftpd
endif

ifdef CONFIG_FILE_SYSTEM
APPSUBDIRS += fs
endif

XROUTER_LIBS += $(INSTALL_DIR)/lib/libtarget.a

NOW=$(shell date -d "now +8 hour" +"%s")
BT_H=./include/build_time.h
$(BT_H): target.ld 
	. scripts/mkversion.sh > .tmpversion
	@mv -f .tmpversion .version
	echo "#define SYS_BUILD_TIME $(NOW)"  > $(BT_H)
	echo "#define SYS_BUILD_COUNT `cat .version`" >> $(BT_H)

XROUTER_OBJS = $(join $(SUBDIRS), $(foreach n,$(SUBDIRS), $(shell echo "/"$(shell echo /$(n).o | sed "s/.*\///"))))

DRV_OBJS = $(join $(DRVSUBDIRS), $(foreach n,$(DRVSUBDIRS), $(shell echo "/"$(shell echo /$(n).o | sed "s/.*\///"))))

APP_OBJS = $(join $(APPSUBDIRS), $(foreach n,$(APPSUBDIRS), $(shell echo "/"$(shell echo /$(n).o | sed "s/.*\///"))))

xroutersubdirs: $(patsubst %, _dir_%, $(SUBDIRS))
$(patsubst %, _dir_%, $(SUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C $(patsubst _dir_%, %, $@)

drvsubdirs: $(patsubst %, _dir_%, $(DRVSUBDIRS))
$(patsubst %, _dir_%, $(DRVSUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C $(patsubst _dir_%, %, $@)

appsubdirs: $(patsubst %, _dir_%, $(APPSUBDIRS))
$(patsubst %, _dir_%, $(APPSUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C $(patsubst _dir_%, %, $@)

drivers.o: drvsubdirs $(DRV_OBJS)
	$(XLD) -r $(ENDIAN) -o drivers.o $(DRV_OBJS)

apps.o: appsubdirs $(APP_OBJS)
	$(XLD) -r $(ENDIAN) -o apps.o $(APP_OBJS)

xrouter: $(BT_H)  xroutersubdirs $(XROUTER_LIBS) $(XROUTER_OBJS) drivers.o apps.o target.ld
	$(XCC) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) $(LDMAP) -o $@ $(XROUTER_OBJS) 
	
zxrouter:  xrouter.bin zload/zwebmap.h
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C zload
	chmod 777 scripts/mkimage
	../../tools/bin/lzma e xrouter.bin  bin.gz
#ifdef	CONFIG_ZWEB
	#$(XLD) $(ENDIAN) $(LD_EXTRA) -Ttext=$(CONFIG_ZLOAD_BUF) -Tzload/zload.ld -o zxrouter zload/zload.o -\( -bbinary bin.gz -bbinary webdata.bin -\)
#else
	$(XLD) $(ENDIAN) $(LD_EXTRA) -Ttext=$(CONFIG_ZLOAD_BUF) -Tzload/zload.ld -o zxrouter zload/zload.o -\( -bbinary bin.gz -\)
#endif
#	rm bin.gz webdata.bin
	$(XNM) $@ | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | sort > $@.map

zxrouter.bin: zxrouter
	chmod -Rf 777 scripts/imghdr/imghdr
	$(XOC) -O binary $(@:.bin=) $@
	scripts/imghdr/imghdr zxrouter.bin t.bin 0x`$(XOD) -s init/version.o | grep "data.sys_sw_version" -A 1 | tail -n 1 | cut -b7-14` $(NOW) "$(FWM_DESC)" ; rm zxrouter.bin ; mv t.bin zxrouter.bin
#	cat zxrouter.bin scripts/zero.bin > zxrouter.pad.bin
	scripts/mkimage -A mips -T standalone -C none -a $(CONFIG_ZLOAD_BUF) -e $(CONFIG_ZLOAD_BUF) -n "zxrouter" -d zxrouter.bin zxrouter.img
#	$(XOD) -d xrouter > ecos.dump
#	$(XOD) -d -S xrouter > eCosS.dump

zload/zwebmap.h: xrouter.map
	grep "zweb_location" xrouter.map | sed -e 's/^\(.*\) \(.*\) \(.*\)$\/\#define ZWEB_LOC 0x\1/' > zload/zwebmap.h

target.ld: .config
	sh ./arch/init_arch.sh $(APPS_DIR)

FWM_DESC += $(shell date -d "1970/01/01 +$(NOW) sec" +"%T %x")

cptftp:	xrouter.bin zxrouter.bin
	cp -f xrouter.bin $(IMAGE_NAME).bin
	cp -f zxrouter.bin z$(IMAGE_NAME).bin
	cp -f zxrouter.img $(IMAGE_NAME).img
	cp -f zxrouter.img $(TFTP_DIR)/$(IMAGE_NAME).img
	#if [ -e ${INSTALL_SCRIPT} ] ; then source ${INSTALL_SCRIPT} ; fi
	$(XOD) -h -EL -m mips32 apps.o >appsh.dump
	$(XOD) -h -EL -m mips32 drivers.o >driversh.dump
	$(XOD) -h -EL -m mips32 xrouter >xrouterh.dump
	rm -Rf xrouter.* zxrouter*

zip: clean
	@echo "tar source & install dir"
	cd .. ; tar zcvf apps.tmp $(notdir $(APPS_DIR)) ; mv -f apps.tmp $(notdir $(APPS_DIR))_`date +%D | sed "s/\///" | sed "s/\///"`.tgz

zipbsp:
	cd .. ; tar zcvf bsp.tmp $(notdir $(shell cd $(INSTALL_DIR)/.. ; pwd)) ; mv -f bsp.tmp $(notdir $(shell cd $(INSTALL_DIR)/.. ; pwd))_`date +%D | sed "s/\///" | sed "s/\///"`.tgz

bsp:
	make -C $(INSTALL_DIR)/../ecos_build

.PHONY: all clean webpages $(BT_H)

clean:	cleanpages
	@echo "clean all.."
	-rm -f profile.txt
	-rm -f xrouter* zxrouter* $(IMAGE_NAME).* z$(IMAGE_NAME).* bin.gz
	-rm -f drivers.o app.o ecos.dump eCosS.dump
	find . \( -name '*.[oasd]' -o -name core -o -name '.*.flags' \) -type f -print | xargs rm -f
	find '../../ra305x_drivers' \( -name '*.[oasd]' -o -name core -o -name '.*.flags' \) -type f -print | xargs rm -f
	make -C ../../ra305x_drivers/Jedi_7628/embedded clean
	-rm -f zxrouter.pad.bin zxrouter.img
	
release:
	@echo "build release wifi driver package"
	make -C ../../ra305x_drivers/Jedi_7628/embedded release	
	
cleanpages:
	@echo "clean webpages.."
	-rm -f webdata.bin
	-rm -rf webpages/web_c
	-rm -f ./webpages/web_list ./webpages/webpage.c ./webpages/webpage.h
	find webpages \( -name '*.[oasd]' -o -name '.*.flags' \) -type f -print | xargs rm -f
	
webpages:
	if [ -e ./.prjname ] ; then make webpages -C config/$(PRJ_DIR) ; fi
	
initmake: permission
	@echo "Init make environment"
	$(DOS2UNIX) `find -name \*.sh`
	$(DOS2UNIX) defconfig config.in ./scripts/Menuconfig
	touch $(BT_H)
	
cookie:
	@echo make download signature
ifdef	CONFIG_DL_COOKIE
	./scripts/imghdr/cookie $(CONFIG_DL_COOKIE_STR)
endif
	
initconfig: permission
	bash scripts/Menuconfig config.in save
	
menuconfig: permission
	bash scripts/Menuconfig config.in
	make cookie

permission:
	chmod u+rx scripts/lxdialog/lxdialog
	chmod u+rx scripts/imghdr/imghdr
	chmod u+rx scripts/imghdr/imghdr_bn
	chmod u+rx scripts/imghdr/cookie

checkin: clean
	-rm -rf webpages
	@echo "clean configuration"
	-rm -f .config .config.old .menuconfig.log .version .prjname
	-rm -f defconfig profile.txt target.ld
	-rm -f ./include/dl_cookie.h ./include/autoconf.h ./include/build_time.h
	-rm -f ./include/cgi_aux.h
	-rm -f ./init/start.S ./init/board_spec.c
	-rm -f ./zload/load.S ./zload/zload.ld ./zload/zwebmap.h
	-rm -f ./upnpd/device/xml_InternetGatewayDevice.c
	find . -name '*~' -type f -print | xargs rm -f


