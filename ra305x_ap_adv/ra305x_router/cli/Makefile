# Makefile

include $(TOPDIR)/.config
include $(TOPDIR)/rules.mak

ALL_OBJS = cmd_proc.o cmd_main.o io_util.o 
ALL_OBJS +=  cmd_wireless.o 

ifdef CONFIG_CLI_CFG_CMD 
ALL_OBJS +=cmd_cfg.o
endif

ifdef CONFIG_CLI_NET_CMD 
ALL_OBJS +=cmd_net.o
endif

ifdef CONFIG_CLI_OS_CMD
ALL_OBJS +=  cmd_os.o
ifdef CONFIG_CPULOAD
ALL_OBJS += cmd_cpuload.o
endif
endif 

ifdef CONFIG_CLI_DBG_CMD
ALL_OBJS +=  cmd_dbg.o
endif

ifdef CONFIG_CLI_MFT_CMD
ALL_OBJS +=  cmd_mft.o
endif





MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)
