#
# Makefile for XRouter TCP/IP
#

include $(TOPDIR)/.config

CURR_DIR=$(shell pwd)
EXTRA_CFLAGS = -DGATEWAY -DIPFILTER -D_KERNEL -D__INSIDE_NET

ifdef	CONFIG_BRIDGE
EXTRA_CFLAGS += -DBRIDGE
endif

ifdef CONFIG_MROUTING
EXTRA_CFLAGS += -DMROUTING
endif


ifdef CONFIG_NATT
EXTRA_CFLAGS += -DIPSEC_NAT_T
endif

FILEIO_DIR = ./fileio
FILEIO = $(FILEIO_DIR)/socket.o  

ETH_DIR = ./eth
ETH = $(ETH_DIR)/eth_drv.o

OS_DIR = ./os
OS = $(OS_DIR)/support.o $(OS_DIR)/synch.o $(OS_DIR)/timeout.o $(OS_DIR)/init.o 

SYS_KERN_DIR = ./sys/kern
SYS_KERN = $(SYS_KERN_DIR)/kern_subr.o $(SYS_KERN_DIR)/kern_sysctl.o \
	$(SYS_KERN_DIR)/md5c.o $(SYS_KERN_DIR)/sockio.o \
	$(SYS_KERN_DIR)/uipc_accf.o $(SYS_KERN_DIR)/uipc_domain.o \
	$(SYS_KERN_DIR)/uipc_mbuf2.o $(SYS_KERN_DIR)/uipc_mbuf.o \
	$(SYS_KERN_DIR)/uipc_socket2.o $(SYS_KERN_DIR)/uipc_socket.o

SYS_NET_DIR = ./sys/net
SYS_NET = $(SYS_NET_DIR)/if.o $(SYS_NET_DIR)/if_ethersubr.o \
	$(SYS_NET_DIR)/if_loop.o $(SYS_NET_DIR)/radix.o \
	$(SYS_NET_DIR)/raw_cb.o $(SYS_NET_DIR)/raw_usrreq.o \
	$(SYS_NET_DIR)/route.o $(SYS_NET_DIR)/rtsock.o
ifdef CONFIG_IPV6
SYS_NET +=$(SYS_NET_DIR)/if_stf.o # IPv6 6to4 Tunnel support
endif
ifdef	CONFIG_BRIDGE
SYS_NET += $(SYS_NET_DIR)/bridge.o
endif

SYS_NETINET_DIR = ./sys/netinet
SYS_NETINET = $(SYS_NETINET_DIR)/if_ether.o $(SYS_NETINET_DIR)/igmp.o $(SYS_NETINET_DIR)/in.o \
	$(SYS_NETINET_DIR)/in_cksum.o $(SYS_NETINET_DIR)/in_pcb.o $(SYS_NETINET_DIR)/in_proto.o \
	$(SYS_NETINET_DIR)/in_rmx.o $(SYS_NETINET_DIR)/ip_ecn.o $(SYS_NETINET_DIR)/ip_encap.o \
	$(SYS_NETINET_DIR)/ip_flow.o $(SYS_NETINET_DIR)/ip_icmp.o $(SYS_NETINET_DIR)/ip_id.o \
	$(SYS_NETINET_DIR)/ip_input.o $(SYS_NETINET_DIR)/ip_mroute.o $(SYS_NETINET_DIR)/ip_output.o \
	$(SYS_NETINET_DIR)/raw_ip.o $(SYS_NETINET_DIR)/tcp_debug.o $(SYS_NETINET_DIR)/tcp_input.o \
	$(SYS_NETINET_DIR)/tcp_output.o $(SYS_NETINET_DIR)/tcp_subr.o $(SYS_NETINET_DIR)/tcp_timer.o \
	$(SYS_NETINET_DIR)/tcp_usrreq.o $(SYS_NETINET_DIR)/udp_usrreq.o

SYS_NETINET6_DIR = ./sys/netinet6
ifdef CONFIG_IPV6
SYS_NETINET6 = $(SYS_NETINET6_DIR)/dest6.o $(SYS_NETINET6_DIR)/frag6.o $(SYS_NETINET6_DIR)/icmp6.o \
	$(SYS_NETINET6_DIR)/in6.o $(SYS_NETINET6_DIR)/in6_cksum.o $(SYS_NETINET6_DIR)/in6_ifattach.o \
	$(SYS_NETINET6_DIR)/in6_pcb.o $(SYS_NETINET6_DIR)/in6_proto.o $(SYS_NETINET6_DIR)/in6_rmx.o \
	$(SYS_NETINET6_DIR)/in6_src.o $(SYS_NETINET6_DIR)/ip6_forward.o $(SYS_NETINET6_DIR)/ip6_input.o \
	$(SYS_NETINET6_DIR)/ip6_mroute.o $(SYS_NETINET6_DIR)/ip6_output.o $(SYS_NETINET6_DIR)/mld6.o \
	$(SYS_NETINET6_DIR)/nd6.o $(SYS_NETINET6_DIR)/nd6_nbr.o $(SYS_NETINET6_DIR)/nd6_rtr.o \
	$(SYS_NETINET6_DIR)/raw_ip6.o $(SYS_NETINET6_DIR)/route6.o $(SYS_NETINET6_DIR)/scope6.o \
	$(SYS_NETINET6_DIR)/udp6_output.o $(SYS_NETINET6_DIR)/udp6_usrreq.o 
# 	Only if firewall enabled
#	$(SYS_NETINET6_DIR)/ip6_fw.o
endif

ifdef CONFIG_IPV6
SYS_IPSEC6 = $(SYS_NETINET_DIR)/key.o $(SYS_NETINET_DIR)/key_debug.o $(SYS_NETINET_DIR)/keydb.o \
	$(SYS_NETINET_DIR)/keysock.o $(SYS_NETINET6_DIR)/ipsec.o $(SYS_NETINET6_DIR)/ah_core.o \
	$(SYS_NETINET6_DIR)/ah_input.o $(SYS_NETINET6_DIR)/ah_output.o $(SYS_NETINET6_DIR)/ipcomp_core.o \
	$(SYS_NETINET6_DIR)/esp_core.o $(SYS_NETINET6_DIR)/esp_output.o $(SYS_NETINET6_DIR)/esp_input.o \
	$(SYS_NETINET6_DIR)/esp_rijndael.o $(SYS_NETINET6_DIR)/esp_twofish.o $(SYS_NETINET6_DIR)/ipcomp_core.o \
	$(SYS_NETINET6_DIR)/ipcomp_output.o $(SYS_NETINET6_DIR)/ipcomp_input.o $(SYS_NETINET6_DIR)/ip_ecn.o \
	$(SYS_NETINET6_DIR)/in6_gif.o
endif

ifdef CONFIG_MROUTING
SYS_NETINET += $(SYS_NETINET_DIR)/ip_mroute.o
endif

ALL_OBJS =  $(FILEIO) $(ETH) $(OS) $(SYS_KERN) $(SYS_NET) $(SYS_NETINET) $(SYS_NETINET6)


MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)

clean:
	rm $(MODULE) $(ALL_OBJS)


