# Makefile

include $(TOPDIR)/.config
include $(TOPDIR)/rules.mak

ALL_OBJS = watchdog_main.o

MODULE = $(shell pwd | sed "s/.*\///" ).o

ifeq ($(CHIPSET),3050)
EXTRA_CFLAGS += -DCONFIG_RT3050_ASIC
endif

ifeq ($(CHIPSET),3052)
EXTRA_CFLAGS += -DCONFIG_RT3052_ASIC
endif

ifeq ($(CHIPSET),3352)
EXTRA_CFLAGS += -DCONFIG_RT3352_ASIC
endif

ifeq ($(CHIPSET),5350)
EXTRA_CFLAGS += -DCONFIG_RT5350_ASIC
endif

all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
