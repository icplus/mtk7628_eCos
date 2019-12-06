#
# Makefile for Ralink Tech. HTTPD

.EXPORT_ALL_VARIABLES:
include $(TOPDIR)/.config
ifdef	CONFIG_WIRELESS
ifeq ($(WIFI_MODE),)
RT28xx_MODE = AP
else
RT28xx_MODE = $(WIFI_MODE)
endif
ifeq ($(CHIPSET),mt7628)
include ../../../ra305x_drivers/Jedi_7628/embedded/os/ecos/config.mk
else
include ../../../ra305x_drivers/wireless_ra305x/os/ecos/config.mk
endif
endif

ifeq ($(CHIPSET),mt7628)
EXTRA_CFLAGS += -DRT7628
else
EXTRA_CFLAGS += -DRT$(CHIPSET)
endif

ifdef	CONFIG_WIRELESS
EXTRA_CFLAGS += $(WFLAGS)
endif

ifeq ($(word 1,$(shell dos2unix -V 2>&1)),tofrodos)
DOS2UNIX=dos2unix -p
else
DOS2UNIX=dos2unix -k
endif


ifeq ($(TOPDIR)/websrc,$(wildcard $(TOPDIR)/websrc))

ifeq ($(WEB_LANG),TChinese)
WEBPAGE_SRC	= $(TOPDIR)/websrc/$(shell echo $(CONFIG_WEB_DIR))_tc/
else
WEBPAGE_SRC	= $(TOPDIR)/websrc/$(shell echo $(CONFIG_WEB_DIR))
endif

USRPAGE_SRC	= $(TOPDIR)/config/$(shell cat $(TOPDIR)/.prjname)/webpages
ifeq ($(USRPAGE_SRC),$(wildcard $(USRPAGE_SRC)))
WEBPAGE_SRC += $(USRPAGE_SRC)/*
endif

else
WEBPAGE_SRC = 
endif

WEB_DIR	= web_html
ALL_OBJS= web_c/web_c.o ./webpage.o

#ifndef	CONFIG_ZWEB
ALL_OBJS += webdata.o
#endif


MODULE	= $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

# Recursively to next directory
CGIDIRS = cgi auth

ifeq (cgi_usr,$(wildcard cgi_usr))
CGIDIRS += cgi_usr
endif

EXTRA_CFLAGS += -I$(TOPDIR)/httpd/include

cgidirs: $(patsubst %, _dir_%, $(CGIDIRS))
$(patsubst %, _dir_%, $(CGIDIRS)) :
	$(MAKE) -C $(patsubst _dir_%, %, $@) "CFLAGS=$(CFLAGS) $(EXTRA_CFLAGS)"

CGIOBJS = $(join $(CGIDIRS), $(patsubst %, /%.o, $(CGIDIRS)))

#Make module here
$(MODULE): cgidirs $(ALL_OBJS) $(CGIOBJS)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) $(CGIOBJS)

#
# Build a set of WEB c files and head files
#
webdata.d: web_c/web_c.o
	touch webdata.d
	ln -s -f ../webdata.bin .
	$(XLD) -r $(ENDIAN) -o webdata.o -bbinary webdata.bin
	rm -f webdata.bin

web_c/web_c.d: $(WEBPAGE_SRC) $(TOPDIR)/include/autoconf.h
	if [ -e $(TOPDIR)/websrc ] ; then \
		touch web_c/web_c.d ; \
		$(MAKE) web -C $(TOPDIR)/config/$(shell cat $(TOPDIR)/.prjname) ; \
	fi

web_c/web_c.o: webcomp_sin $(WEB_DIR)/* $(TOPDIR)/include/autoconf.h
	echo "make web pages library"
	cd $(WEB_DIR) ; $(DOS2UNIX) *.* ; cd ..
	find $(WEB_DIR) -name "*.*" | sort > web_list
	rm -rf webpage.c webpage.h 
	if [ ! -e web_c ] ; then mkdir web_c ; else rm -f web_c/* ; fi
	sh ./webcomp.sh $(WEB_DIR)
	cp web_c_makefile.def ./web_c/Makefile
	make -C web_c "CFLAGS=$(CFLAGS) $(EXTRA_CFLAGS)"

# since webpage.c doesn't exist while make doing the check for generic rules.
# we add the rule to avoid this problem.
./webpage.c:    web_c/web_c.o

webcomp_sin: webcomp_sin.c
	gcc -o webcomp_sin webcomp_sin.c;

.PHONY: all


