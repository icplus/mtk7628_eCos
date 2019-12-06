# Makefile

include ../.config

ifeq ($(word 1,$(shell dos2unix -V 2>&1)),tofrodos)
DOS2UNIX=dos2unix -p
else
DOS2UNIX=dos2unix -k
endif

ALL_OBJS = cfg_prof.o cfg_chk.o cfg_data.o

ifdef	CONFIG_SYSLOG
ALL_OBJS += cfg_eventlog.o
endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d) ../profile.txt
	cp ../profile.txt ./profile.txt
	$(DOS2UNIX) ./profile.txt
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) -bbinary profile.txt
	rm profile.txt

clean:
	rm $(MODULE) $(ALL_OBJS)


