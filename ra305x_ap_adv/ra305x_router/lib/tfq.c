/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/
/* Traffic Flow Queneing */

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cfg_id.h>
#include <sys_status.h>
#include <network.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <sys/time.h>

#include <net/if_qos.h>
#include <tfq.h>

extern int (*qos_tfq)(struct mbuf *m, struct ether_header *eh);

struct tfq_attr_t tfq_attr = {0};
unsigned int lan_ip_gw = 0;
unsigned int lan_ip_range = 0;
unsigned int lan_netmask = 0;
unsigned int qos_enable = 0;

void tfq_load_config()
{
	char ip[20];
	char config_str[100];
	char *arglists[4], *iplists[2];;
	int i = 0, j = 0, val=0;
	unsigned int flag;	

	CFG_get(CFG_QOS_EN, &val);
	qos_enable = val;
	if (val) {
		CFG_get_str( CFG_LAN_IP, ip);
		lan_ip_gw =  inet_addr(ip);
		CFG_get_str( CFG_LAN_MSK, ip);
		lan_netmask =  inet_addr(ip);
		lan_ip_range = (lan_ip_gw & lan_netmask);
		memset(&tfq_attr, 0, sizeof(struct tfq_attr_t));


		while(++i && (CFG_get(CFG_QOS_IPTABLE+i, config_str) != -1))
		{
			int index_s = 0, index_e = 0, uplink = 0, downlink = 0;
			int count = str2arglist(config_str, arglists, ';', 4);
			if (count != 4)
				continue;

			uplink = atoi(arglists[1]);
			downlink = atoi(arglists[2]);
			flag = atoi(arglists[3]);
			if (flag == 0) //Disable
				continue;

			count = str2arglist(arglists[0], iplists, '~', 2);
			if (count == 1) {
				index_s = atoi(iplists[0]);
				index_e = atoi(iplists[0]);
			} else if (count == 2) {
				index_s = atoi(iplists[0]);
				index_e = atoi(iplists[1]);			
			}

			if (index_s != 0) {
				for (j = index_s; j <= index_e && j < 255;j++) {
					gettimeofday(&tfq_attr.list[j].uplast_, NULL);
					gettimeofday(&tfq_attr.list[j].downlast_, NULL);
					tfq_attr.list[j].upmaxrate_ = uplink*1.1;
					tfq_attr.list[j].downmaxrate_ = downlink*1.1;

					diag_printf("%s, ip=%d, uplink=%d, downlink=%d\n", __FUNCTION__, j, tfq_attr.list[j].upmaxrate_, tfq_attr.list[j].downmaxrate_);
				}
			}				
		}

			
	}
}

int tfq_uplink(struct mbuf *m, struct ether_header *eh)
{
	register struct ip *ip;
	struct timeval	 now;
	int index =0;	

	if (qos_enable == 0)
		return 0;
				
	/* check wan status */
	if (SYS_wan_up != CONNECTED)
		return 0;
	
	if(eh->ether_type == htons(ETHERTYPE_IP))
	{
		ip = mtod(m, struct ip *);

		/* TCP ACK */
		if (htons(ip->ip_len) == 40)
			return 0;

#if 0 //For Debug
				diag_printf("tfq_uplink ip->ip_src= %s\n",inet_ntoa(ip->ip_src));
				diag_printf("\tip->ip_dst= %s\n",inet_ntoa(ip->ip_dst));
				diag_printf("\t\t%d\n",htons(ip->ip_len));
#endif

		/* send to wan_ip packet */
		if ((ip->ip_src.s_addr == SYS_wan_ip) || (ip->ip_dst.s_addr == SYS_wan_ip))
			return 0;
	
		/* send to lan_ip packet */
		if ((ip->ip_src.s_addr == lan_ip_gw) || (ip->ip_dst.s_addr == lan_ip_gw))
			return 0;
	
		/* lan to lan packet */
		if ((ip->ip_src.s_addr & lan_netmask) == (ip->ip_dst.s_addr & lan_netmask))
			return 0;
	
		if ((ip->ip_src.s_addr & lan_netmask) == lan_ip_range) {
			/* uplink */
			index = (ip->ip_src.s_addr & 0xff000000) >> 24;
			if ((index != 0) && (tfq_attr.list[index].upmaxrate_ != 0)) {
				struct tfq_entry_t *entry = &tfq_attr.list[index];
					
				gettimeofday(&now, NULL);
				if (now.tv_sec > 1)
					now.tv_sec--;
				if (timercmp(&entry->uplast_, &now, <)) {
					entry->upbytes = 0;
					gettimeofday(&entry->uplast_, NULL);
				}
					
				if (entry->upbytes > entry->upmaxrate_)
					return -1;
					
				entry->upbytes += htons(ip->ip_len);
			}			
		}
	}
	
	return 0;
}



int tfq_downlink(struct mbuf *m)
{
	register struct ip *ip;
	register struct ether_header *eh;
	struct timeval	 now;
	int index =0;	

	if (qos_enable == 0)
		return 0;

	/* check wan status */
	if (SYS_wan_up != CONNECTED)
		return 0;

	eh = mtod(m, struct ether_header *);

	if(eh->ether_type == htons(ETHERTYPE_IP))
	{
		if(m->m_len >= 34)
			ip = (struct ip *)((char *)eh+14);
		else
			ip = mtod(m->m_next, struct ip *);

		/* TCP ACK */
		if (htons(ip->ip_len) == 40)
			return 0;

#if 0 //For Debug
		diag_printf("tfq_downlink ip->ip_src= %s\n",inet_ntoa(ip->ip_src));
		diag_printf("\tip->ip_dst= %s\n",inet_ntoa(ip->ip_dst));
		diag_printf("\t\t%d\n",htons(ip->ip_len));
#endif

		/* send to wan_ip packet */
		if ((ip->ip_src.s_addr == SYS_wan_ip) || (ip->ip_dst.s_addr == SYS_wan_ip))
			return 0;

		/* send to lan_ip packet */
		if ((ip->ip_src.s_addr == lan_ip_gw) || (ip->ip_dst.s_addr == lan_ip_gw))
			return 0;

		/* lan to lan packet */
		if ((ip->ip_src.s_addr & lan_netmask) == (ip->ip_dst.s_addr & lan_netmask))
			return 0;

		if ((ip->ip_dst.s_addr & lan_netmask) == lan_ip_range) {
			/* downlink */
			index = (ip->ip_dst.s_addr & 0xff000000) >> 24;
			if ((index != 0) && (tfq_attr.list[index].downmaxrate_ != 0)) {
				struct tfq_entry_t *entry = &tfq_attr.list[index];
				
				gettimeofday(&now, NULL);
				if (now.tv_sec > 1)
					now.tv_sec--;
				if (timercmp(&entry->downlast_, &now, <)) {
					entry->downbytes = 0;
					gettimeofday(&entry->downlast_, NULL);
				}
				
				if (entry->downbytes > entry->downmaxrate_)
					return -1;
				
				entry->downbytes += htons(ip->ip_len);
			}			
		}
	}

	return 0;
}

int tfq_enqueue(struct schedq *sq, struct mbuf *m)
{
	/* Direction: downlink */
	struct ifqueue *ifq = sq->ifq;

	if (tfq_downlink(m) == 0) {
		(m)->m_nextpkt = 0;
		if ((ifq)->ifq_tail == 0)
			(ifq)->ifq_head = m;
		else
			(ifq)->ifq_tail->m_nextpkt = m;
		(ifq)->ifq_tail = m;
		(ifq)->ifq_len++;
		return 0;
	} else {
		m_freem(m);
		return (-1);
	}
}

struct mbuf * tfq_dequeue(struct schedq *sq)
{
	struct mbuf *m = NULL;

	struct ifqueue *ifq = sq->ifq;

	(m) = (ifq)->ifq_head;
	if (m) {
		if (((ifq)->ifq_head = (m)->m_nextpkt) == 0)
			(ifq)->ifq_tail = 0;
		(m)->m_nextpkt = 0;
		(ifq)->ifq_len--;
	} 

	return m;
}

void tfq_requst(struct schedq *sq, int flag)
{
	
	switch(flag)
	{
		case SCHED_FLUSHQ:
			break;
		default:
			break;
	}
}

void tfq_schedQ_register(struct ifnet *ifp, struct schedq *sq)
{
	int val=0;

	CFG_get(CFG_QOS_EN, &val);
	if (val) {
		struct ifqueue *ifq = &ifp->if_snd;
		
		/* flush older packet */
		if(ifq->ifq_len != 0)
			if_qflush(&ifp->if_snd);
		
		sq->ifq = ifq;
		sq->qattr = &tfq_attr;
		ifq->schedq = sq;		
		qos_tfq = tfq_uplink;
	}
}

void tfq_schedQ_unregister(struct ifnet *ifp)
{
	struct ifqueue *ifq = &ifp->if_snd;
	
	ifq->schedq = NULL;
}

