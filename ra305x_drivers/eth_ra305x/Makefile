# Makefile

include $(TOPDIR)/rules.mak
include $(TOPDIR)/.config

EXTRA_CFLAGS = -I. -D_KERNEL
ifeq ($(CHIPSET),)
CHIPSET = 3350
endif

ifeq ($(CHIPSET),7620)
EXTRA_CFLAGS += -DCONFIG_RT7620_ASIC
else
ifeq ($(CHIPSET),3352)
EXTRA_CFLAGS += -DCONFIG_RT3352_ASIC
else
ifeq ($(CHIPSET),5350)
EXTRA_CFLAGS += -DCONFIG_RT5350_ASIC
else # RT3050, RT3052, RT3350
EXTRA_CFLAGS += -DCONFIG_RT3052_ASIC
endif
endif
endif

ALL_OBJS = if_ra305x.o long_loop.o


MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm -f $(MODULE) $(ALL_OBJS)
	rm -f *.d
