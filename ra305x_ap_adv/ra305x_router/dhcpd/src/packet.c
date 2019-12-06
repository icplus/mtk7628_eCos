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
#include <network.h>
#include <arpa/inet.h>
#include <sys/mbuf.h>
#include <netinet/ip_var.h>

#include <stdlib.h>
#include <string.h>

#include <sys_status.h>
#include <lib_packet.h>

#include <dhcpd.h>
#include <common_subr.h>
#include <leases.h>
#include <packet.h>

struct in_ifaddr *in_iawithaddr(struct in_addr, struct mbuf *);
int in_cksum(struct mbuf *, int);
void m_freem(struct mbuf *);


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
unsigned short DHCPD_checksum(void *addr, int count)
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


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//      send a packet to giaddr using the kernel ip stac
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static int DHCPD_SendToRelay(struct dhcpd *payload)
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
    client.sin_port = htons(SERVER_PORT);
    client.sin_addr.s_addr = server_config.server.s_addr;

    if (bind(fd, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
    {
        close(fd);
        return -1;
    }

    memset(&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(SERVER_PORT);
    client.sin_addr.s_addr = payload->giaddr;

    if (connect(fd, (struct sockaddr *)&client, sizeof(struct sockaddr)) == -1)
    {
        close(fd);
        return -1;
    }

    result = write(fd, payload, sizeof(struct dhcpd));
    close(fd);
    return result;
}


//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_SendToClient
//
// DESCRIPTION
//  send a packet to a specific arp address and ip address by creating our own ip packet
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
static int DHCPD_SendToClient(struct dhcpd *payload, int force_broadcast, int txifp)
{
    unsigned char *chaddr;
    u_int32_t ciaddr;
    int result;
    struct sockaddr_in dest;
    struct udp_dhcp_packet packet;

    if (force_broadcast)
    {
        DHCPD_DIAG("broadcasting packet to client (NAK)");
        ciaddr = INADDR_BROADCAST;
        chaddr = MAC_BCAST_ADDR;
    }
    else if (payload->ciaddr)
    {
        DHCPD_DIAG("unicasting packet to client ciaddr");
        ciaddr = payload->ciaddr;
        chaddr = payload->chaddr;
    }
    else
    {
#ifndef IGNORE_BROADCASTFLAG
        if (!(ntohs(payload->flags) & BROADCAST_FLAG))
        {
            DHCPD_DIAG("unicasting packet to client yiaddr");
            ciaddr = payload->yiaddr;
            chaddr = payload->chaddr;
        }
        else
#endif
        {
            DHCPD_DIAG("broadcasting packet to client");
            ciaddr = INADDR_BROADCAST;
            chaddr = MAC_BCAST_ADDR;
        }
    }

    /* construct IP+UDP */
    memset(&dest, 0, sizeof(dest));
    memset(&packet, 0, sizeof(packet));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = ciaddr;


    packet.ip.ip_p = IPPROTO_UDP;
    packet.ip.ip_src.s_addr = server_config.server.s_addr;
    packet.ip.ip_dst.s_addr = ciaddr;
    packet.udp.uh_sport = htons(SERVER_PORT);
    packet.udp.uh_dport = htons(CLIENT_PORT);
    packet.udp.uh_ulen = htons(sizeof(packet.udp) + sizeof(struct dhcpd)); /* cheat on the psuedo-header */
    packet.ip.ip_len = packet.udp.uh_ulen;
    memcpy(&(packet.data), payload, sizeof(struct dhcpd));
    packet.udp.uh_sum = DHCPD_checksum(&packet, sizeof(struct udp_dhcp_packet));

    packet.ip.ip_len = htons(sizeof(struct udp_dhcp_packet));
    packet.ip.ip_hl = sizeof(packet.ip) >> 2;
    packet.ip.ip_v = IPVERSION;
    packet.ip.ip_ttl = IPDEFTTL;
    packet.ip.ip_sum = DHCPD_checksum(&(packet.ip), sizeof(packet.ip));

    /* send out to specified interface */
    result = LIB_etherpacket_send((struct ifnet *)txifp, NULL, chaddr, (char *)&packet, sizeof(struct udp_dhcp_packet));
    if (result <= 0)
    {
        diag_printf("raw_packet(): sendto failed");
    }

    return result;
}


//------------------------------------------------------------------------------
// FUNCTION
//      send_packet
//
// DESCRIPTION
//      send a dhcp packet, if force broadcast is set,
//      the packet will be broadcast to the client
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
int DHCPD_SendPacket(struct dhcpd *payload, int force_broadcast, int txifp)
{
    int ret;

    if (payload->giaddr)
        ret = DHCPD_SendToRelay(payload);
    else
        ret = DHCPD_SendToClient(payload, force_broadcast, txifp);

    return ret;
}


//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_ReadPacket
//
// DESCRIPTION
//      read a packet from socket fd, return -1 on read error, -2 on packet error
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
int DHCPD_ReadPacket(struct dhcpInfo *packetinfo, int fd)
{
    struct dhcpd *packet = &(packetinfo->payload);
    int bytes;
    int i;
    const char broken_vendors[][8] =
    {
        "MSFT 98",
        ""
    };
    char unsigned *vendor;

    memset(packet, 0, sizeof(struct dhcpd));

    bytes = LIB_rcvinfo_read(fd, packet, sizeof(struct dhcpd));
    if (bytes < 0)
    {
        diag_printf("couldn't read on listening socket, ignoring\n");
        return -1;
    }

    if (ntohl(packet->cookie) != DHCP_MAGIC)
    {
        //diag_printf("received bogus message, ignoring");
        return -2;
    }
    LIB_rcvinfo_getifp(fd, &(packetinfo->rcv_ifp));

    if (packet->op == BOOTREQUEST && (vendor = dhcpd_pickup_opt(packet, DHCP_VENDOR, 0, 0)))
    {
        for (i = 0; broken_vendors[i][0]; i++)
        {
            if (vendor[OPT_LEN - 2] == (unsigned char) strlen(broken_vendors[i]) &&
                !strncmp(vendor, broken_vendors[i], vendor[OPT_LEN - 2]))
            {
                diag_printf("broken client (%s), forcing broadcast\n",
                            broken_vendors[i]);
                packet->flags |= htons(BROADCAST_FLAG);
            }
        }
    }


    return bytes;
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
int DHCPD_PktCheck(struct ifnet *ifp, char *head, struct mbuf *m, struct eth_input_func *hn)
{
    struct udphdr *udp;
    struct ip *ip;

    if(m->m_pkthdr.len < 40)
        return 0;

    ip =  mtod(m, struct ip *);
    if(ip->ip_p != IPPROTO_UDP)
        return 0;

    if((ntohs(ip->ip_off) & 0x3fff) != 0)
        return 0;

    udp =  (struct udphdr *) ((char *)ip + (ip->ip_hl << 2));
    if(ntohs(udp->uh_dport) != SERVER_PORT)
        return 0;

    if(LIB_pktsocket_enQ(ifp, head, m, hn) < 0)
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
int DHCPD_rcv(int fd, struct timeval *tv, int checkaddr)
{
    struct rcv_info *info;
    struct udphdr *udp;
    struct ip *ip;

    info = LIB_pktsocket_deQ((struct pktsocket *)fd, tv);
    if (info == 0)
        return 0;

    if (info->m == 0)
        goto bad;

    ip =  mtod(info->m, struct ip *);
    if (ntohs(ip->ip_off) & IP_OFFMASK)
        goto bad;

    /* IP checksum */
    if ((ip->ip_sum = DHCPD_checksum((short *)ip, ip->ip_hl << 2)) != 0)
        goto bad;

#if 0 //Eddy6 ToDo: check the below code 
    if (checkaddr)
    {
        if (ip->ip_dst.s_addr != INADDR_BROADCAST &&
            ip->ip_dst.s_addr != INADDR_ANY &&
            (in_iawithaddr(ip->ip_dst, info->m)) == NULL)
            goto bad;
    }
#endif

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

