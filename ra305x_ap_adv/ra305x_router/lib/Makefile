# Makefile

#ALL_OBJS = ping.o dns.o lib_util.o net_cfg.o net_stat.o net_arp.o lib_eth.o net_rt.o \
	clone_mac.o priq.o inet_addr.o inet_ntoa.o inet_ntop.o inet_pton.o \
	getaddrinfo.o getproto.o getserv.o ifaddrs.o inflate.o md5.o base64.o \
	md4.o blowfish.o

ALL_OBJS = ping.o dns.o lib_util.o net_cfg.o net_stat.o net_arp.o lib_eth.o net_rt.o \
	clone_mac.o inet_addr.o inet_ntoa.o inet_ntop.o inet_pton.o \
	getaddrinfo.o getproto.o getserv.o ifaddrs.o inflate.o md5.o base64.o \
	md4.o blowfish.o tfq.o LzmaDec.o uncompress.o
	
MODULE = $(shell pwd | sed "s/.*\///").o

all: $(MODULE)

include $(TOPDIR)/rules.mak

$(MODULE): $(ALL_OBJS:.o=.d)
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS)
	
clean:
	rm $(MODULE) $(ALL_OBJS)
