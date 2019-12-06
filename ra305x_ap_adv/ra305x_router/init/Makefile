# Makefile

include $(TOPDIR)/.config
ALL_OBJS = lan_start.o wan_start.o main.o mon_task.o start.o mod_init.o version.o config_options.o dns_config.o httpd_config.o serial_communication.o cpuid.o

#ifdef	CONFIG_BRIDGE
#ALL_OBJS += br_start.o 
#endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

EXTRA_CFLAGS = -I../dhcpc/include -I../httpd/include

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
	
