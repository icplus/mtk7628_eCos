#PLATFORM = 5VT
#PLATFORM = PC
#PLATFORM = STAR
OS = ECOS

include $(TOPDIR)/rules.mak
include $(TOPDIR)/.config

EXTRA_CFLAGS = -I. -D_KERNEL
ALL_OBJS = rtdot1x.o eloop.o eapol_sm.o radius.o md5.o  \
	config.o ieee802_1x.o  \
	sta_info.o   radius_client.o

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
