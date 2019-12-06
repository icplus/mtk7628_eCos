#include <sys/types.h>
#include <network.h>
#include <stdio.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <ifaddrs.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>
#include <string.h>

#ifdef DBG
#include <stdarg.h>
#endif // DBG //

#include "sniffer_handle.h"
#include "../../../ra305x_drivers/Jedi_7628/embedded/include/sniffer/sniffer.h"
#include "../../../ra305x_drivers/Jedi_7628/embedded/include/sniffer/radiotap.h"


#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif


void SnifferPktReceive(struct ifnet *ifp, struct mbuf *m, int len);
	

/*
SnifferPktReceive()
handle sniffer packet:

packet format:

*/
extern int sniffer_debug_level;

void sniffer_packet_hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
	unsigned char *pt;
	int x;
	
	pt = pSrcBufVA;
	printf("%s: %p, len = %d\n",str,  pSrcBufVA, SrcBufLen);
	for (x=0; x<SrcBufLen; x++)
	{
		if (x % 16 == 0) 
			printf("0x%04x : ", x);
		printf("%02x ", ((unsigned char)pt[x]));
		if (x%16 == 15) printf("\n");
	}
    printf("\n");
}


void SnifferPktReceive(struct ifnet *ifp, struct mbuf *m, int len)
{
	struct mtk_radiotap_header *pMtk_rt_hdr, mtk_rt_hdr;
	DOT_11_HDR *pDOT_11_HDR, dot11_hdr;
	int mtk_radiotap_header_len=0;
	
	if(sniffer_debug_level > 2)
	{
		//diag_printf("SnifferPktReceive %s0 packet\n",ifp->if_name);
		sniffer_packet_hex_dump("ether input Pkt content", m->m_data, 48);
	}
	
	/*
get mtk_radiotap_header  */
	pMtk_rt_hdr = m->m_data;
	memcpy(&mtk_rt_hdr, pMtk_rt_hdr, sizeof(struct mtk_radiotap_header));
	mtk_radiotap_header_len = mtk_rt_hdr.rt_hdr.it_len;

	//User Todo:Parse and Process mtk_radiotap_header
	if(sniffer_debug_level > 2)
	{
		sniffer_packet_hex_dump("mtk_radiotap_header:", &mtk_rt_hdr, mtk_radiotap_header_len);
	}	

	/*  get 802.11 header  */	
	pDOT_11_HDR = m->m_data + mtk_radiotap_header_len;
	memcpy(&dot11_hdr, pDOT_11_HDR, sizeof(dot11_hdr));
	
	//User Todo:Parse and Process 802.11 header
	if(sniffer_debug_level > 1)
	{
		sniffer_packet_hex_dump("802.11 header:", &dot11_hdr, sizeof(dot11_hdr));
	}

	
	//Release
	//Not release mbuf *m, will release in caller

	return;
	
}

