# Makefile

EXTRA_CFLAGS = -DHAVE_CONFIG_H -DRT_DEBUG -DCONFIG_WPS -I./include -I./ -I./wsc -I./device

#assign value to define_wps base on FLAG:CONFIG_WPS define
FIND_INIC = SECOND_WIFI
ifneq	("$(findstring	$(FIND_INIC),$(CFLAGS))" , "")
	define_inic = y
endif

ALL_OBJS= ./xmldata.o

ALL_OBJS += upnpxmldata.o


MODULE	= $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

#include $(TOPDIR)/rules.mak
include ../rules.mak

# Recursively to next directory
UPNPDIR = upnp
DEVDIR = device
PORTDIR = porting/ecos
WSCDIR = wsc

UPNPOBJS = \
	$(UPNPDIR)/upnp.o \
	$(UPNPDIR)/upnp_http.o \
	$(UPNPDIR)/ssdp.o \
	$(UPNPDIR)/soap.o \
	$(UPNPDIR)/gena.o \
	$(UPNPDIR)/upnp_description.o \
	$(UPNPDIR)/upnp_util.o

DEVOBJS = \
	$(DEVDIR)/device_init.o \
	$(DEVDIR)/table_advertise.o \
	$(DEVDIR)/table_service.o \
	$(DEVDIR)/table_description.o \
	$(DEVDIR)/soap_x_lanhostconfigmanagement.o \
	$(DEVDIR)/soap_x_layer3forwarding.o \
	$(DEVDIR)/soap_x_wancommoninterfaceconfig.o \
	$(DEVDIR)/soap_x_wanipconnection.o \
	$(DEVDIR)/soap_x_wfawlanconfig.o \
	$(DEVDIR)/xml_WFADeviceDesc.o \
	$(DEVDIR)/xml_x_wfawlanconfigscpd.o \
	$(DEVDIR)/xml_InternetGatewayDevice.o \
	$(DEVDIR)/xml_x_lanhostconfigmanagement.o \
	$(DEVDIR)/xml_x_layer3forwarding.o \
	$(DEVDIR)/xml_x_wancommoninterfaceconfig.o \
	$(DEVDIR)/xml_x_wanipconnection.o \
	$(DEVDIR)/xml_WFA5GDeviceDesc.o \
	$(DEVDIR)/xml_x_wfa5Gwlanconfigscpd.o \
	$(DEVDIR)/xml_InternetGatewayDevice2WLAN.o 

PORTOBJS = \
	$(PORTDIR)/upnpd.o \
	$(PORTDIR)/os_info.o \
	$(PORTDIR)/model_info.o \
	$(PORTDIR)/upnp_portmap.o
	
WSCOBJS = \
	$(WSCDIR)/wsc_main.o \
	$(WSCDIR)/wsc_common.o \
	$(WSCDIR)/wsc_ioctl.o \
	$(WSCDIR)/wsc_netlink.o \
	$(WSCDIR)/wsc_upnp_device.o \
	$(WSCDIR)/wsc_msg.o

UPNP_OBJS = $(UPNPOBJS) $(DEVOBJS) $(PORTOBJS) $(WSCOBJS)

#Make module here
$(MODULE): upnpxmldata.d $(ALL_OBJS) $(UPNP_OBJS)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) $(UPNP_OBJS)

#
# Build a set of WEB c files and head files
#
upnpxmldata.d:
	touch upnpxmldata.d
	echo "make xml data library"
	rm -rf xmldata.c upnpxmldata.bin
	if [ "${define_inic}" = "y" ] ; then sh ./upnpcomp_inic.sh ; else sh ./upnpcomp.sh ; fi
	ln -s -f ../upnpxmldata.bin .
	$(XLD) -r $(ENDIAN) -o upnpxmldata.o -bbinary upnpxmldata.bin
	#rm -f upnpxmldata.bin


# since webpage.c doesn't exist while make doing the check for generic rules.
# we add the rule to avoid this problem.
#./xmldata.c:    xmldataprepare.d


.PHONY: all


clean:
	rm $(MODULE) $(ALL_OBJS) $(UPNP_OBJS)
