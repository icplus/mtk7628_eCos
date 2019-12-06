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

#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <errno.h>

#include <network.h>
#include <netinet/if_ether.h>
#include <sys/mbuf.h>
#include <netinet/ip_var.h>

#include <stdlib.h>

#include <dhcpc.h>
#include <options.h>
#include <packet.h>


unsigned short checksum(void *addr, int count)
{
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register int sum = 0;
    unsigned short *source = (unsigned short *) addr;

    while( count > 1 )
    {
        /*  This is the inner loop */
        sum += *source++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if( count > 0 )
        sum += * (unsigned char *) source;

    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}


/* Constuct a ip/udp header for a packet, and specify the source and dest hardware address */
int raw_packet(struct dhcpc *payload, u_int32_t source_ip, int source_port,
               u_int32_t dest_ip, int dest_port, unsigned char *src_arp, unsigned char *dest_arp, int txifp)
{
    int result;
    struct sockaddr_in dest;
    struct udp_dhcpc_packet packet;

    /* construct IP+UDP */
    memset(&dest, 0, sizeof(dest));
    memset(&packet, 0, sizeof(packet));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_ip;

    packet.ip.ip_p = IPPROTO_UDP;
    packet.ip.ip_src.s_addr = source_ip;
    packet.ip.ip_dst.s_addr = dest_ip;
    packet.udp.uh_sport = htons(source_port);
    packet.udp.uh_dport = htons(dest_port);
    packet.udp.uh_ulen = htons(sizeof(packet.udp) + sizeof(struct dhcpc)); /* cheat on the psuedo-header */
    packet.ip.ip_len = packet.udp.uh_ulen;
    memcpy(&(packet.data), payload, sizeof(struct dhcpc));
    packet.udp.uh_sum = checksum(&packet, sizeof(struct udp_dhcpc_packet));

    packet.ip.ip_len = htons(sizeof(struct udp_dhcpc_packet));
    packet.ip.ip_hl = sizeof(packet.ip) >> 2;
    packet.ip.ip_v = IPVERSION;
    packet.ip.ip_ttl = IPDEFTTL;
    packet.ip.ip_sum = checksum(&(packet.ip), sizeof(packet.ip));

    /* send out to specified interface */
    result = LIB_etherpacket_send((struct ifnet *)txifp, NULL, dest_arp, (char *)&packet, sizeof(struct udp_dhcpc_packet));
    if (result <= 0)
    {
        diag_printf("raw_packet(): sendto failed");
    }

    return result;
}

/* Let the kernel do all the work for packet generation */
int kernel_packet(struct dhcpc *payload, u_int32_t source_ip, int source_port,
                  u_int32_t dest_ip, int dest_port)
{
    int n = 1;
    int fd, result;
    struct sockaddr_in client;

    if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        return -1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)) == -1)
    {
        close(fd);
        return -1;
    }

    memset(&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(source_port);
    client.sin_addr.s_addr = source_ip;

    if (bind(fd, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
    {
        close(fd);
        return -1;
    }

    memset(&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(dest_port);
    client.sin_addr.s_addr = dest_ip;

    if (connect(fd, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
    {
        close(fd);
        return -1;
    }

    result = write(fd, payload, sizeof(struct dhcpc));
    close(fd);
    return result;
}

//------------------------------------------------------------------------------
// FUNCTION
//  Note: should link the dhcpc_check to tcpip module
//        And implement dhcpc_rcv wiht netgraph
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
void m_freem(struct mbuf *m);
int in_cksum(struct mbuf *, int);

int dhcpc_check(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn)
{
    struct udphdr *udp;
    struct ip *ip;

    if (m->m_pkthdr.len < 40)
        return 0;

    ip =  mtod(m, struct ip *);
    if (ip->ip_p != IPPROTO_UDP)
        return 0;

    if ((ntohs(ip->ip_off) & 0x3fff) != 0)
        return 0;

    udp =  (struct udphdr *) ((char *)ip + (ip->ip_hl << 2));
    if (ntohs(udp->uh_dport) != CLIENT_PORT)
        return 0;

    if (LIB_pktsocket_enQ(ifp, head, m, hn) < 0)
        return 1;

    return 2;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
int dhcpc_rcv(int fd, struct timeval *tv)
{
    struct rcv_info *info;
    struct udphdr *udp;
    struct ip *ip;

    info = LIB_pktsocket_deQ((struct pktsocket *)fd, tv);
    if(info == 0)
        return 0;

    if(info->m == 0)
        goto bad;

    ip =  mtod(info->m, struct ip *);
    if (ntohs(ip->ip_off) & IP_OFFMASK)
        goto bad;

    /* IP checksum */
    if((ip->ip_sum = checksum((short *)ip, ip->ip_hl << 2)) != 0)
        goto bad;

    udp =  (struct udphdr *) ((char *)ip + (ip->ip_hl << 2));
    if((info->len-(ip->ip_hl << 2)) != ntohs(udp->uh_ulen))
        goto bad;

    /* UDP checksum */
    if(udp->uh_sum)
    {
        bzero(((struct ipovly *)ip)->ih_x1, sizeof ((struct ipovly *)ip)->ih_x1);
        ((struct ipovly *)ip)->ih_len = udp->uh_ulen;
        if ((udp->uh_sum = in_cksum(info->m, info->len)) != 0)
            goto bad;
    }

    info->buf = (char *)udp+sizeof(*udp);
    info->len = (int)ip+(info->len)-(int)(info->buf);
    return (int)info;

bad:
    if (info->m)
        m_freem(info->m);

    free(info);
    return -1;
}

