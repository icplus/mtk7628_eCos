#LATFORM = 5VT
#PLATFORM = PC
#PLATFORM = STAR
OS = ECOS

include $(TOPDIR)/rules.mak
include $(TOPDIR)/.config

EXTRA_CFLAGS = -I. -D_KERNEL -pipe -Wall -Wno-unused -O3 -g
ALL_OBJS = osl-ecos.o lltpmain.o event.o util.o packetio.o band.o \
	state.o sessionmgr.o enumeration.o mapping.o seeslist.o \
	tlv.o qospktio.o

ifeq ($(BRANCH),STD)
EXTRA_CFLAGS += -DBRANCH_STD -DDBG
endif

ifeq ($(BRANCH),ADV)
EXTRA_CFLAGS += -DBRANCH_ADV -DDBG
endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm -f $(MODULE) $(ALL_OBJS)
	rm -f *.d
