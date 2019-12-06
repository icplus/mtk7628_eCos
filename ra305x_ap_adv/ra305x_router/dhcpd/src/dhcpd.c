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


//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <config.h>
#include <cyg/kernel/kapi.h>
#include <cyg/fileio/fileio.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>

#include <unistd.h>
#include <network.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lib_util.h>
#include <lib_packet.h>
#include <cfg_def.h>
#include <cfg_net.h>
#include <cfg_dns.h>
#include <sys_status.h>

#include <dhcpd.h>
#include <common_subr.h>
#include <leases.h>
#include <packet.h>
#include <dnsmasq.h>


//==============================================================================
//                                    MACROS
//==============================================================================
#define STACK_SIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL)*2

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
enum
{
    INITIATE=1,
    SHUTDOWN=2,
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
cyg_handle_t dhcpd_thread_h = 0;
cyg_thread   dhcpd_thread;

static cyg_uint8 dhcpd_stack[5120];//[ STACK_SIZE ];//change dhcpd_stack size:8192->5*1024

struct dhcpOfferedAddr *leases;
struct server_config_t server_config;
static int dhcpd_running = 0;
static int dhcpd_cmd = 0;

#ifdef CONFIG_SUPERDMZ
static unsigned char dmz_enable = 0;
static unsigned char wan_connected = 0;
static unsigned char dmzmac[6];
static void add_replys_to_DMZ(struct dhcpd *packet, u_int32_t lease_time);
#endif


// Simon added for pass WAN domain
int pass_wan_domain = 0;
struct in_addr  DHCPD_GetGwConf(struct in_addr gw);


//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//      format: [hostname];[IP address];[MAC address];[flag];[expired time]
//          flag: 0 -> dynamic, 1 -> static, 2 -> static but disabled
//          ex. mars0;10.10.10.15;00:11:22:33:44:55;0;1111
//
// PARAMETERS
//
// RETURN
//
//
//------------------------------------------------------------------------------
void DHCPD_LoadStaticLeases(void)
{
    int i=0;
    char lease_str[100], buf[100];
    char *arglists[5];
    struct in_addr sa;
    char mac[6];
    unsigned int flag;
    struct dhcpOfferedAddr *lease;

    // Set LAN IP and LAN mask
    in_addr_t lan_ip = ntohl(server_config.server.s_addr);
    in_addr_t lan_mask = ntohl(server_config.mask.s_addr);
    in_addr_t lan_net = (lan_ip & lan_mask);

    while(++i && (CFG_get(CFG_LAN_DHCPD_SLEASE+i, lease_str) != -1))
    {
        if (str2arglist(lease_str, arglists, ';', 5) != 5)
            continue;

        inet_aton(arglists[1], &sa);
        ether_aton_r(arglists[2], mac);
        flag = atoi(arglists[3]);

        /* maybe LAN IP has been change, we need to update CFG */
        if ((ntohl(sa.s_addr) & lan_mask) != lan_net)
        {
            sa.s_addr = htonl(lan_net | (ntohl(sa.s_addr) & ~lan_mask));

            sprintf(buf, "%s;%s;%s;%s;%s", arglists[0], inet_ntoa(sa), arglists[2], arglists[3], arglists[4]);
            CFG_set(CFG_LAN_DHCPD_SLEASE+i, buf);
        }

        /* can not assign LAN IP and broadcast address */
        if (ntohl(sa.s_addr) == lan_ip ||
            ntohl(sa.s_addr) == (lan_net | ~lan_mask))
        {
            CFG_del(CFG_LAN_DHCPD_SLEASE+i);
        }

        if (flag & DISABLED)
            continue;
        diag_printf("dhcpd: check sip=%08x\n", sa.s_addr);
        /*  Check IP conflic */
        if ((lease = DHCPD_FindLeaseByYiaddr(sa)) != NULL)
        {
            if (lease->flag & STATICL)
            {
                /*  The IP is occupied by the other static host  */
                continue;
            }
            else
            {
                /*  The IP is occupied by the other dynamic host  */
//              diag_printf("dhcpd: flush lease=%08x\n", (uint32_t) lease);
                DHCPD_FlushLeaseEntry(lease);
            }
        }


        if (DHCPD_AddLease(mac, sa, atoi(arglists[4]), arglists[0], 1) == NULL)
        {
            diag_printf("no space to add lease\n");
            return;
        }

        DHCPD_FindAndSetLease(mac, sa, flag);
    }

    CFG_commit(CFG_NO_EVENT);

    return;
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
int DHCPD_leases_set(char *buf)
{
    char tbuf[255], lease_str[255];
    char *arglists[5];
    char *arglists1[5];

    int flag;
    int i, found=0;
    struct in_addr sa;
    char mac[6];

    if (buf == 0 || strlen(buf) >= sizeof(tbuf))
        return -1;

    strcpy(tbuf, buf);
    if (str2arglist(tbuf, arglists, ';', 5) != 5)
        return -1;

    inet_aton(arglists[1], &sa);
    ether_aton_r(arglists[2], mac);
    flag = atoi(arglists[3]);

    i = 1;
    while (CFG_get(CFG_LAN_DHCPD_SLEASE+i, lease_str) != -1)
    {
        if (str2arglist(lease_str, arglists1, ';', 5) != 5)
            continue;

        if (strcmp(arglists[2], arglists1[2]) == 0)
        {
            found = 1;
            break;
        }
        i++;
    }

    if (found)
    {
        if ((flag == DYNAMICL) || (flag & DELETED))
        {
            CFG_del(CFG_LAN_DHCPD_SLEASE+i);
            DHCPD_FindAndSetLease(mac, sa, flag);
        }
        else
        {
            CFG_set(CFG_LAN_DHCPD_SLEASE+i, buf);
        }
    }
    else
    {
        DHCPD_FindAndSetLease((unsigned char *)mac, sa, flag);

        if (flag == STATICL)
        {
            CFG_set(CFG_LAN_DHCPD_SLEASE+i, buf);
        }
    }

    /* flush static leases, then reload from CFG */
    DHCPD_FlushStaticLeases();
    DHCPD_LoadStaticLeases();

    /* fix lease IP or expired time */
    if (fix_lease_error() != 0)
    {
        DHCPD_WriteDynLeases();
    }

    return 0;
}

//***********************************
// dhcpd packet process
//
static void add_replys(struct dhcpd *packet, u_int32_t lease_time)
{
    extern int pass_wan_domain;
    unsigned char *option = packet->options;

    // Set bootp fields
    packet->siaddr = server_config.siaddr.s_addr;
    if (server_config.sname)
        strncpy(packet->sname, server_config.sname, sizeof(packet->sname) - 1);

    if (server_config.boot_file)
        strncpy(packet->file, server_config.boot_file, sizeof(packet->file) - 1);

    // Set grand options
    if (lease_time)
    {
        lease_time = htonl(lease_time);
        dhcpd_add_option(option, DHCP_LEASE_TIME, sizeof(lease_time), &lease_time);
    }

    dhcpd_add_offsers(option);

    if (pass_wan_domain)
    {
	if (strlen(SYS_wan_domain))
        dhcpd_add_option(option, DHCP_DOMAIN_NAME, strlen(SYS_wan_domain), SYS_wan_domain);
    }

}
static void make_dhcpd_packet(struct dhcpd *packet, struct dhcpd *oldpacket, char type)
{
    unsigned char *option = packet->options;

    memset(packet, 0, sizeof(struct dhcpd));

    packet->op = BOOTREPLY;
    packet->htype = ETH_10MB;
    packet->hlen = ETH_10MB_LEN;
    packet->xid = oldpacket->xid;

    memcpy(packet->chaddr, oldpacket->chaddr, 16);

    packet->flags = oldpacket->flags;
    packet->ciaddr = oldpacket->ciaddr;
    packet->giaddr = oldpacket->giaddr;

    packet->cookie = htonl(DHCP_MAGIC);

    packet->options[0] = DHCP_END;
    dhcpd_add_option(option, DHCP_MESSAGE_TYPE, sizeof(type), &type);
    dhcpd_add_option(option, DHCP_SERVER_ID, sizeof(server_config.server), &server_config.server);
//    dhcpd_add_option(option, DHCP_DNS_SERVER, sizeof(server_config.server), &server_config.server);
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
static int send_nack(struct dhcpInfo *oldpacketinfo)
{
    struct dhcpd *oldpacket = &(oldpacketinfo->payload);
    struct dhcpd packet;

    make_dhcpd_packet(&packet, oldpacket, DHCPNAK);

    DHCPD_DIAG("sending NAK");
    return DHCPD_SendPacket(&packet, 1, oldpacketinfo->rcv_ifp);
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
int send_ack(struct dhcpInfo *oldpacketinfo, struct in_addr yiaddr)
{
    struct dhcpd *oldpacket = &(oldpacketinfo->payload);
    struct dhcpd packet;
    unsigned char _hostname[255] = {0};
    u_int32_t lease_time = server_config.lease;

    if (dhcpd_pickup_opt(oldpacket, DHCP_LEASE_TIME, sizeof(lease_time), &lease_time))
    {
        lease_time = ntohl(lease_time);
        if (lease_time > server_config.lease ||
            lease_time < server_config.min_lease)
        {
            lease_time = server_config.lease;
        }
    }

    // Save hostname to lease
    dhcpd_pickup_opt(oldpacket, DHCP_HOST_NAME, sizeof(_hostname)-1, hostname);
    DHCPD_AddLease(oldpacket->chaddr, yiaddr, lease_time, _hostname, 0);

    // Send ACK here
    make_dhcpd_packet(&packet, oldpacket, DHCPACK);
    packet.yiaddr = yiaddr.s_addr;

#ifdef CONFIG_SUPERDMZ
    if (dmz_enable && wan_connected && (memcmp(oldpacket->chaddr, &dmzmac[0], 6) == 0))
    {
        add_replys_to_DMZ(&packet, lease_time);
    }
    else
#endif
        add_replys(&packet, lease_time);

    DHCPD_DIAG("sending ACK to %s", inet_ntoa(yiaddr));
    return DHCPD_SendPacket(&packet, 0, oldpacketinfo->rcv_ifp);
}


static int discover(struct dhcpInfo *packetinfo)
{
    struct dhcpd *oldpacket = &(packetinfo->payload);
    struct dhcpd packet;
    struct dhcpOfferedAddr *lease = NULL;
    u_int32_t lease_time = server_config.lease;
    struct in_addr req_ip;
    struct in_addr addr;

    /* block all 0 and all 0xff mac address */
    if (memcmp(oldpacket->chaddr, "\x00\x00\x00\x00\x00\x00", 6) == 0 ||
        memcmp(oldpacket->chaddr, "\xff\xff\xff\xff\xff\xff", 6) == 0)
    {
        return -1;
    }

#ifdef CONFIG_SUPERDMZ
    if (dmz_enable && (memcmp(oldpacket->chaddr, &dmzmac[0], 6) == 0))
    {
        if ((wan_connected) && (SYS_wan_up == CONNECTED))
        {
            addr.s_addr = SYS_wan_ip;
            DHCPD_CheckIp(addr, oldpacket->chaddr, 1);
            goto SEND_OFFER;
        }
        else
            return -1;
    }
#endif

    /* if static, short circuit */
    /* the client is in our lease/offered table */
    lease = DHCPD_FindLeaseByChaddr(oldpacket->chaddr);
    if (lease &&
        ((lease->flag & STATICL) ||
         (ntohl(lease->yiaddr.s_addr) >= ntohl(server_config.start.s_addr) && ntohl(lease->yiaddr.s_addr) <= ntohl(server_config.end.s_addr))))
    {
        DHCPD_DIAG("mac has been exist!");

        // check same IP, if we found static lease
        // and conflict with other PC, it not need to reserve
        if (DHCPD_CheckIp(lease->yiaddr, oldpacket->chaddr, (lease->flag & STATICL ? 0: 1)) > 0)
            return -1;

        // reseved IP
        if (lease->flag & RESERVED)
            lease->flag = lease->flag & ~RESERVED;

        if (!DHCPD_LeaseExpired(lease))
            lease_time = lease->expires  -   GMTtime(0);

        addr = lease->yiaddr;
        /* Or the client has a requested ip */
    }
    else if (dhcpd_pickup_opt(oldpacket, DHCP_REQUESTED_IP, sizeof(req_ip), &req_ip) &&
             ntohl(req_ip.s_addr) >= ntohl(server_config.start.s_addr) &&
             ntohl(req_ip.s_addr) <= ntohl(server_config.end.s_addr) &&
             ((!(lease = DHCPD_FindLeaseByYiaddr(req_ip)) || DHCPD_LeaseExpired(lease))) )
    {
        DHCPD_DIAG("requested IP=%x", ntohl(req_ip.s_addr));

        /* check same IP */
        if (DHCPD_CheckIp(req_ip, oldpacket->chaddr, 1) > 0)
            return -1;

        addr = req_ip;
        /* otherwise, find a free IP */ /*ADDME: is it a static lease? */
    }
    else
    {
        addr = DHCPD_FindAddress(0);

        /* try for an expired lease */
        if (addr.s_addr == 0)
            addr = DHCPD_FindAddress(1);
    }

SEND_OFFER:
    // Check your address
    if (!addr.s_addr)
    {
        DHCPD_DIAG("no IP addresses to give -- OFFER abandoned");
        return -1;
    }

    if (!DHCPD_AddLease(oldpacket->chaddr, addr, server_config.offer_time, NULL, 0))
    {
        DHCPD_DIAG("lease pool is full -- OFFER abandoned");
        return -1;
    }

    if (dhcpd_pickup_opt(oldpacket, DHCP_LEASE_TIME, sizeof(lease_time), &lease_time))
    {
        lease_time = ntohl(lease_time);
        if (lease_time > server_config.lease)
            lease_time = server_config.lease;
    }

    /* Make sure we aren't just using the lease time from the previous offer */
    if ((lease_time < server_config.min_lease)||(lease_time > server_config.lease))
        lease_time = server_config.lease;

    /* Send offer here */
    make_dhcpd_packet(&packet, oldpacket, DHCPOFFER);
    packet.yiaddr = addr.s_addr;

#ifdef CONFIG_SUPERDMZ
    if (dmz_enable && wan_connected && (memcmp(oldpacket->chaddr, &dmzmac[0], 6) == 0))
    {
        add_replys_to_DMZ(&packet, lease_time);
    }
    else
#endif
        add_replys(&packet, lease_time);

    DHCPD_DIAG("sending OFFER of %s", inet_ntoa(addr));
    return DHCPD_SendPacket(&packet, 0, packetinfo->rcv_ifp);
}

static int request(struct dhcpInfo *packetinfo)
{
    struct dhcpd *packet = &(packetinfo->payload);
    unsigned char *requested;
    unsigned char *server_id;

    struct in_addr req_ip = {0};
    struct in_addr server = {0};
    struct in_addr addr;

    struct dhcpOfferedAddr *lease;


    DHCPD_DIAG("Recive REQUEST");

    requested = dhcpd_pickup_opt(packet, DHCP_REQUESTED_IP, sizeof(req_ip), &req_ip);
    server_id = dhcpd_pickup_opt(packet, DHCP_SERVER_ID, sizeof(server), &server);

    if (server_id && server.s_addr != server_config.server.s_addr)
    {
        DHCPD_DIAG("Not my server ID");
        return 0;
    }

#ifdef CONFIG_SUPERDMZ
    if (dmz_enable && !wan_connected && (memcmp(packet->chaddr, &dmzmac[0], 6) == 0))
    {
        send_nack(packetinfo);
        return 0;
    }
#endif

    lease = DHCPD_FindLeaseByChaddr(packet->chaddr);
    if (lease)
    {
        if (requested)
        {
            /* INIT-REBOOT State */
            if (lease->yiaddr.s_addr != req_ip.s_addr)
            {
                send_nack(packetinfo);
                return 0;
            }
        }
        else
        {
            /* RENEWING or REBINDING State */
            if (lease->yiaddr.s_addr != packet->ciaddr)
            {
                send_nack(packetinfo);
                return 0;
            }
        }

        addr = lease->yiaddr;
    }
    else
    {
        if (requested)
        {
            DHCPD_DIAG("no leases, req_ip:%x", ntohl(req_ip.s_addr));
            addr = req_ip;
        }
        else if (packet->ciaddr)
        {
            DHCPD_DIAG("no leases, packet->ciaddr:%x", ntohl(packet->ciaddr));
            addr.s_addr = packet->ciaddr;
        }
        else
        {
            send_nack(packetinfo);
            return 0;
        }

        if ((ntohl(addr.s_addr) < ntohl(server_config.start.s_addr)) ||
            (ntohl(addr.s_addr) > ntohl(server_config.end.s_addr)))
        {
            send_nack(packetinfo);
            return 0;
        }
        if ((lease=DHCPD_FindLeaseByYiaddr(addr)) != NULL && !DHCPD_LeaseExpired(lease))
        {
            /*  The IP is occupied by some others  */
            send_nack(packetinfo);
            return 0;
        }
        if (DHCPD_CheckIp(addr, packet->chaddr, 0) > 0)
        {
            send_nack(packetinfo);
            return 0;
        }
    }

    if (send_ack(packetinfo, addr) >=0)
        DHCPD_WriteDynLeases();

    return 0;
}

static void decline(struct dhcpInfo *packetinfo)
{
    struct dhcpOfferedAddr *lease;
    struct dhcpd *oldpacket = &(packetinfo->payload);

    lease = DHCPD_FindLeaseByChaddr(oldpacket->chaddr);
    if (lease)
    {
        /* we only clean dynamic leases */
        if ((lease->flag & STATICL) == 0)
        {
            struct in_addr addr;

            addr.s_addr = oldpacket->ciaddr;

            //DHCPD_ClearLease(oldpacket->chaddr, addr);
            DHCPD_FlushLeaseEntry(lease);
            DHCPD_CheckIp(addr, oldpacket->chaddr, 1);
            DHCPD_WriteDynLeases();
        }
    }
}

static void release(struct dhcpInfo *packetinfo)
{
    struct dhcpOfferedAddr *lease;
    struct dhcpd *oldpacket = &(packetinfo->payload);

    lease = DHCPD_FindLeaseByChaddr(oldpacket->chaddr);
    if (lease)
    {
        DHCPD_DIAG("%s Released", inet_ntoa(lease->yiaddr));

        lease->expires = GMTtime(0);
        DHCPD_WriteDynLeases();
    }
}

static void inform(struct dhcpInfo *oldpacketinfo)
{
    struct dhcpd *oldpacket = &(oldpacketinfo->payload);
    struct dhcpd packet;
    struct in_addr addr;

    make_dhcpd_packet(&packet, oldpacket, DHCPACK);

#ifdef CONFIG_SUPERDMZ
    if (dmz_enable && wan_connected && (memcmp(oldpacket->chaddr, &dmzmac[0], 6) == 0))
    {
        add_replys_to_DMZ(&packet, 0);
    }
    else
#endif
        add_replys(&packet, 0);

    DHCPD_SendPacket(&packet, 0, oldpacketinfo->rcv_ifp);

    addr.s_addr = oldpacket->ciaddr;
    DHCPD_DIAG("Receive %s inform", inet_ntoa(addr));
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
static int init_config(void)
{
    struct in_addr start;
    struct in_addr end;
    struct in_addr sa;

    in_addr_t lan_ip;
    in_addr_t lan_mask;
    in_addr_t lan_net;


    int i, val;
    char valstr[255] = {0};
    extern int config_dhcps_wan_domain;

    memset(&server_config, 0, sizeof(struct server_config_t));

    /* we use first listen interface as server IP */
    server_config.interface = LAN_NAME;
    if (read_interface(server_config.interface, &server_config.server.s_addr, &server_config.mask.s_addr, server_config.arp) < 0)
        return -1;

    lan_ip = ntohl(server_config.server.s_addr);
    lan_mask = ntohl(server_config.mask.s_addr);
    lan_net = (lan_ip & lan_mask);

    //start address
    if (CFG_get(CFG_LAN_DHCPD_START, &start) == -1)
        inet_aton(DEFAULT_START, &start);

    server_config.start.s_addr = htonl(lan_net | (ntohl(start.s_addr) & ~lan_mask));

    // end address
    if (CFG_get( CFG_LAN_DHCPD_END, &end) == -1)
        inet_aton(DEFAULT_END, &end);

    server_config.end.s_addr = htonl(lan_net | (ntohl(end.s_addr) & ~lan_mask));

    server_config.max_leases = DHCPD_MAX_LEASES;

    // lease time
    if (CFG_get( CFG_LAN_DHCPD_LEASET, &val) == -1)
        val = DEFAULT_LEASE_TIME;

    server_config.lease = val;
    server_config.conflict_time = DEFAULT_CONFLICT_TIME;
    server_config.decline_time = DEFAULT_DECLINE_TIME;
    server_config.min_lease = DEFAULT_MIN_LEASE_TIME;
    server_config.offer_time = DEFAULT_MIN_LEASE_TIME;
    server_config.auto_time = DEFAULT_AUTO_TIME;

    server_config.sname = DEFAULT_SNAME;
    server_config.boot_file = DEFAULT_BOOT_FILE;

    inet_aton(DEFAULT_BOOT_IP, &sa);
    server_config.siaddr = sa;

    //
    // Attach embedded options
    //
    dhcpd_flush_embed_options();

    //
    // add netmask option
    //
    memcpy(&sa, &SYS_lan_mask, sizeof(struct in_addr));
    dhcpd_attach_embed_option(DHCP_SUBNET, &sa);

    // add gateway option
    sa = DHCPD_GetGwConf(server_config.server);
    dhcpd_attach_embed_option(DHCP_ROUTER, &sa);

    // add dns options
    for (i=0; (sa.s_addr = DNS_dhcpd_dns_get(i)) != 0; i++)
    {
        dhcpd_attach_embed_option(DHCP_DNS_SERVER, &sa);
    }

    // add domain name
    pass_wan_domain = config_dhcps_wan_domain;              // Menuconfig set the default.
    CFG_get(CFG_LAN_DHCPD_PASS_WDN, &pass_wan_domain);      // Can be overridden by profile
    if (pass_wan_domain == 0)
    {
        if (CFG_get_str( CFG_SYS_DOMAIN, valstr) != -1)
        	{
		if (strlen(valstr))
            dhcpd_attach_embed_option(DHCP_DOMAIN_NAME, valstr);
        	}
    }

    /* flush static leases, then reload from CFG */
    DHCPD_LoadDynLeases();

    DHCPD_FlushStaticLeases();
    DHCPD_LoadStaticLeases();

    /* fix lease IP or expired time */
    if (fix_lease_error() != 0)
        DHCPD_WriteDynLeases();

#ifdef CONFIG_SUPERDMZ
    {
        int i, val;
        char line[20];

        /* DMZ to rule */
        if(CFG_get(CFG_NAT_DMZ_EN, &val) != -1)
        {
            memset(&dmzmac[0], 0, 6);

            dmz_enable = 1;
            if ((val == 2) && (CFG_get(CFG_NAT_SDMZ_MAC, line) != -1))
            {
                for(i=0; i<6; i++)
                    val |= line[i];

                if(val != 0)
                    memcpy(&dmzmac[0], line, 6);
            }

        }
    }
#endif

    return 0;
}

static void (*dhcpd_message_handler_table[9])(void*)=
{
		NULL,// 0
		discover, // 1
		NULL,// 2
		request,// 3
		decline,// 4
		NULL,// 5
		NULL,// 6
		release,// 7
		inform// 8
};

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
void DHCPD_daemon(void)
{
    struct timeval tv;
    int bytes;
    struct dhcpInfo packetinfo;
    struct dhcpd *packet = &(packetinfo.payload);
    unsigned char state;
    int server_socket=-1, listen_socket=0;

    leases = malloc(sizeof(struct dhcpOfferedAddr) * DHCPD_MAX_LEASES);
	if (leases == NULL)
		{	
		diag_printf("%s:%d leases malloc fail \n",__FUNCTION__,__LINE__);
		return ;
		}
    memset(leases, 0, sizeof(struct dhcpOfferedAddr) * DHCPD_MAX_LEASES);

    while (1)
    {
        switch (dhcpd_cmd)
        {
            case SHUTDOWN:
                if (listen_socket)
                {
                    LIB_pktsocket_close(listen_socket);
                    listen_socket = 0;
                    DHCPD_FlushLeases();
                    DHCPD_WriteDynLeases();
                }
                dhcpd_cmd &= ~SHUTDOWN;
                break;

            case INITIATE:
                init_config();
                if ((listen_socket == 0) && ((listen_socket =  LIB_pktsocket_create(LAN_NAME, DHCPD_PktCheck)) == 0))
                {
                    DHCPD_DIAG("FATAL: couldn't create listen socket");
                    dhcpd_cmd &= ~INITIATE;
                    return;
                }

                tv.tv_sec = server_config.auto_time;
                tv.tv_usec = 0;
                dhcpd_cmd &= ~INITIATE;
                break;

            default:
                break;
        }

        if (listen_socket == 0)
        {
            cyg_thread_delay(10);
            continue;
        }

        server_socket = DHCPD_rcv(listen_socket, &tv, 1);
        if (server_socket == 0)
        {
            continue;
        }
        else if (server_socket == -1)
        {
            DHCPD_DIAG("error on dhcpd_rcv");
            continue;
        }

        if ((bytes = DHCPD_ReadPacket(&packetinfo, server_socket)) < 0)
        {
            LIB_rcvinfo_close(server_socket);
            server_socket = -1;
            continue;
        }

        if (dhcpd_pickup_opt(packet, DHCP_MESSAGE_TYPE, sizeof(state), &state) == NULL)
        {
            DHCPD_DIAG("[DHCPD]: couldn't get option from packet, ignoring");
            LIB_rcvinfo_close(server_socket);
            server_socket = -1;
            continue;
        }
		/*
		switch (state)
		{
		case DHCPDISCOVER:
			discover(&packetinfo);
			break;
			
		case DHCPREQUEST:
			request(&packetinfo);
			break;
			
		case DHCPDECLINE:
			decline(&packetinfo);
			break;
			
		case DHCPRELEASE:
			release(&packetinfo);
			break;
			
		case DHCPINFORM:
			inform(&packetinfo);
			break;
			
		default:
			diag_printf("unknown message");
		}*/
		
		if(dhcpd_message_handler_table[state]!=NULL)
			(*dhcpd_message_handler_table[state])(&packetinfo);
		
        LIB_rcvinfo_close(server_socket);
    }

    dhcpd_running = 0;
    return;
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
void DHCPD_init(void)
{
    int val;

    if((CFG_get( CFG_LAN_DHCPD_EN, &val) == -1) || (val != 1))
        return;

    dhcpd_cmd = INITIATE;

    if (dhcpd_running == 0)
    {
        cyg_thread_create
        (
            8,                                  /* scheduling info (eg pri) */
            (cyg_thread_entry_t *)DHCPD_daemon, /* entry function */
            0,                                  /* entry data */
            "DHCP server",                      /* optional thread name */
            dhcpd_stack,                        /* stack base, NULL = alloc */
       		5120,//STACK_SIZE,							/* stack size, 0 = default */
            &dhcpd_thread_h,                    /* returned thread handle */
            &dhcpd_thread                       /* put thread here */
        );

        cyg_thread_resume(dhcpd_thread_h);
        dhcpd_running = 1;
    }
}

void DHCPD_down(void)
{
    int val;

    if ((CFG_get( CFG_LAN_DHCPD_EN, &val) == -1) || (val != 0))
        return;

    dhcpd_cmd = SHUTDOWN;
}

#ifdef CONFIG_SUPERDMZ
void DHCPD_superdmz_set(char * mac, int cmd)
{
    if (mac && (cmd == 1)) //Add
    {
        wan_connected = 1;
        memcpy(&dmzmac[0], mac, 6);
    }
    else if (cmd == 0) //Del
    {
        wan_connected = 0;
        memset(&dmzmac[0], 0, 6);
    }

    return;
}

static void add_replys_to_DMZ(struct dhcpd *packet, u_int32_t lease_time)
{
    extern int pass_wan_domain;
    extern DNSSERVER dns_servers[];
    extern int dns_servers_num;
    unsigned char *option = packet->options;

    // Set bootp fields
    packet->siaddr = server_config.siaddr.s_addr;
    if (server_config.sname)
        strncpy(packet->sname, server_config.sname, sizeof(packet->sname) - 1);

    if (server_config.boot_file)
        strncpy(packet->file, server_config.boot_file, sizeof(packet->file) - 1);

    // Set grand options
    if (lease_time)
    {
        lease_time = htonl(lease_time);
        dhcpd_add_option(option, DHCP_LEASE_TIME, sizeof(lease_time), &lease_time);
    }

    if (dmz_enable && (memcmp(packet->chaddr, &dmzmac[0], 6) == 0))
    {
        struct in_addr sa;
        char valstr[255] = {0};

        if (SYS_wan_up == CONNECTED)
        {
            sa.s_addr =SYS_wan_mask;
            dhcpd_add_option(option, DHCP_SUBNET, 4,&sa);
            sa.s_addr = SYS_wan_gw;
            dhcpd_add_option(option, DHCP_ROUTER, 4,&sa);

            if (dns_servers_num == 1)
            {
                dhcpd_add_option(option, DHCP_DNS_SERVER, 4,&dns_servers[0].addr.in.sin_addr);
            }
            else if (dns_servers_num > 1)
            {
                memcpy(valstr, &dns_servers[0].addr.in.sin_addr, 4);
                memcpy(valstr + 4, &dns_servers[1].addr.in.sin_addr, 4);
                dhcpd_add_option(option, DHCP_DNS_SERVER, 8,&valstr);
            }
        }

    }
    else

        if (pass_wan_domain)
        {
        if (strlen(SYS_wan_domain))
            dhcpd_add_option(option, DHCP_DOMAIN_NAME, strlen(SYS_wan_domain), SYS_wan_domain);
        }
}

#endif

