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

#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <network.h>
#include <cfg_net.h>
#include <errno.h>
#include <cfg_def.h>
#include <cfg_dns.h>
#include <eventlog.h>
#include <sys_status.h>

#include <dhcpc.h>
#include <options.h>
#include <packet.h>

void DHCPC_inform_mon_change( char *intf);
void DHCPC_inform_mon_stop( char *intf);

#define DHCPC_DEBUG
#ifdef DHCPC_DEBUG
#define DHCPC_DBG(c...) SysLog(LOG_USER|SYS_LOG_INFO|LOGM_DHCPC, ##c)
#else
#define DHCPC_DBG(c...) SysLog(SYS_LOG_INFO|LOGM_DHCPC, ##c)
#endif

#define DHCPC_OUT(a) {cmd=a; goto dhcpc_out;}
#define DEFAULT_IP  "169.254.0.101"

#define DHCPC_MAX_COUNTDOWN     7
#define DHCPC_INIT_RCV_SEC      4


#define DHCPC_MIN_LEASE_TIME        30
#define DHCPC_MIN_RENEW_LEASE_TIME  1


static void reset_countdown(dhcpc_if *difp)
{
    difp->countdown = DHCPC_MAX_COUNTDOWN;
    difp->secs = DHCPC_INIT_RCV_SEC;
}

static void new_xid(dhcpc_if *difp)
{
    difp->xid = difp->macaddr[5];
    difp->xid |= (difp->macaddr[4]) << 8;
    difp->xid |= (difp->macaddr[3]) << 16;
    difp->xid |= (difp->macaddr[2]) << 24;
    difp->xid ^= (arc4random() & 0xffff0000);
}

static void lease_function(dhcpc_if *difp);

static void no_lease(dhcpc_if *difp)
{
    difp->lease.time_stamp = 0;
    untimeout((timeout_fun *)&lease_function, difp);
}

static void lease_function(dhcpc_if *difp)
{
    struct dhcp_lease *lease = &difp->lease;
    lease->curr = lease->next;

    no_lease(difp);

    if (difp->mbox_id)
        cyg_mbox_tryput(difp->mbox_id, (void *)CMD_TIMEOUT);

    if (lease->next & DHCP_LEASE_EX)
    {
        return;
    }
    else if (lease->next & DHCP_LEASE_T2)
    {
        lease->next = DHCP_LEASE_EX;
        timeout((timeout_fun *)&lease_function, difp, (lease->expiry - lease->t2));
    }
    else if ( lease->next & DHCP_LEASE_T1 )
    {
        lease->next = DHCP_LEASE_T2;
        timeout((timeout_fun *)&lease_function, difp, (lease->t2 - lease->t1));
    }
    return;
}


static void new_lease(dhcpc_if *difp)
{
    struct dhcp_lease *lease = &difp->lease;
    cyg_uint32 tag = 0;
    int val = 0;

    no_lease(difp);
    lease->curr = lease->next = 0;

    if (difp->state == DHCPSTATE_SELECTING)
    {
        lease->next = DHCP_LEASE_EX;
        timeout((timeout_fun *)&lease_function, difp, 30*100);
        return;
    }

    // Set time stamp
    lease->time_stamp = cyg_current_time();

    if (dhcpc_get_option(&difp->rxpkt, DHCP_LEASE_TIME, sizeof(tag), &tag) && tag == 0xffffffff)
    {
        lease->leased_time = 0;
        lease->expiry = 0xffffffffUL;
        lease->t2 = 0xffffffffUL;
        lease->t1 = 0xffffffffUL;
        return;
    }

    tag = ntohl(tag);
    // Get lease time;
    DHCPC_DBG("get new lease time: %d secs", tag);
    lease->leased_time = tag;
    lease->expiry = lease->leased_time * 100;


    if (tag <= DHCPC_MIN_RENEW_LEASE_TIME)
    {
        lease->leased_time = 1;
        lease->next = DHCP_LEASE_EX;
        timeout((timeout_fun *)&lease_function, difp, lease->leased_time);
        return;
    }

    // Get t2
    tag = 0;
    if (dhcpc_get_option(&difp->rxpkt, DHCP_T2, sizeof(tag), &tag) && tag != 0)
    {
        lease->t2 = ntohl(tag) * 100;
        DHCPC_DBG("get DHCP_T2: %d secs", ntohl(tag));
    }
    else
    {
        lease->t2 = lease->expiry - lease->expiry/8;
    }

    tag = 0;
    if (dhcpc_get_option(&difp->rxpkt, DHCP_T1, sizeof(tag), &tag) && tag != 0)
    {
        lease->t1 = ntohl(tag) * 100;
        DHCPC_DBG("get DHCP_T1: %d secs", ntohl(tag));
    }
    else
    {
        lease->t1 = lease->expiry/2;
    }

    if ((CFG_get(CFG_WAN_DHCP_MAN, &val) != -1) && (val == 1))
    {
        lease->next = DHCP_LEASE_EX;
        timeout((timeout_fun *)&lease_function, difp, lease->leased_time);
    }
    else
    {
        lease->next = DHCP_LEASE_T1;
        timeout((timeout_fun *)&lease_function, difp, lease->t1);
    }
}

static void add_requests(dhcpc_if *difp)
{
    unsigned char *option = difp->txpkt.options;
    unsigned char _hostname[256];
    unsigned char cid[7];
    unsigned char param_req[] = {DHCP_SUBNET, DHCP_ROUTER, DHCP_DNS_SERVER, DHCP_DOMAIN_NAME};

    cid[0] = 1;
    memcpy(cid+1, difp->macaddr, 6);
    dhcpc_add_option(option, DHCP_CLIENT_ID, 7, cid);

    if (CFG_get( CFG_SYS_NAME, _hostname) == CFG_OK)
        dhcpc_add_option(option, DHCP_HOST_NAME, strlen(_hostname), _hostname);

    dhcpc_add_option(option, DHCP_PARAM_REQ, sizeof(param_req), param_req);
}

// ------------------------------------------------------------------------
// Send DHCPC packets
//
static void make_dhcpc_packet(dhcpc_if *difp, unsigned char type)
{
    struct dhcpc *txpkt = &difp->txpkt;

    memset(txpkt, 0, sizeof(struct dhcpc));

    txpkt->op = BOOTREQUEST;
    txpkt->htype = ETH_10MB;
    txpkt->hlen = ETH_10MB_LEN;
    txpkt->xid = difp->xid;

    memcpy(txpkt->chaddr, difp->macaddr, ETH_10MB_LEN);

    txpkt->cookie = htonl(DHCP_MAGIC);
    txpkt->options[0] = DHCP_END;

    dhcpc_add_option(txpkt->options, DHCP_MESSAGE_TYPE, sizeof(type), &type);
}

static void send_discover(dhcpc_if *difp)
{
    make_dhcpc_packet(difp, DHCPDISCOVER);

    if (difp->req_ip)
        dhcpc_add_option(difp->txpkt.options, DHCP_REQUESTED_IP, sizeof(difp->req_ip), &difp->req_ip);

    add_requests(difp);

    raw_packet(&difp->txpkt, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST, SERVER_PORT,
               difp->macaddr, MAC_BCAST_ADDR, (int)difp->ifnetp);
}

static void send_request(dhcpc_if *difp)
{
    unsigned char *option = difp->txpkt.options;

    make_dhcpc_packet(difp, DHCPREQUEST);

    if (difp->req_ip)
        dhcpc_add_option(option, DHCP_REQUESTED_IP, sizeof(difp->req_ip), &difp->req_ip);

    if (difp->server)
        dhcpc_add_option(option, DHCP_SERVER_ID, sizeof(difp->server), &difp->server);

    add_requests(difp);

    raw_packet(&difp->txpkt, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
               SERVER_PORT, difp->macaddr, MAC_BCAST_ADDR, (int)difp->ifnetp);
}

static void send_renew_request(dhcpc_if *difp)
{
    make_dhcpc_packet(difp, DHCPREQUEST);

    difp->txpkt.ciaddr = difp->req_ip;

    add_requests(difp);

    kernel_packet(&difp->txpkt, difp->req_ip, CLIENT_PORT, difp->server, SERVER_PORT);
}

static void send_rebind_request(dhcpc_if *difp)
{
    make_dhcpc_packet(difp, DHCPREQUEST);

    difp->txpkt.ciaddr = difp->req_ip;

    add_requests(difp);

    raw_packet(&difp->txpkt, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
               SERVER_PORT, difp->macaddr, MAC_BCAST_ADDR, (int)difp->ifnetp);
}

static void send_release(dhcpc_if *difp)
{
    unsigned char *option = difp->txpkt.options;

    make_dhcpc_packet(difp, DHCPRELEASE);

    difp->txpkt.ciaddr = difp->req_ip;

    dhcpc_add_option(option, DHCP_REQUESTED_IP,  sizeof(difp->req_ip), &difp->req_ip);
    dhcpc_add_option(option, DHCP_SERVER_ID, sizeof(difp->server), &difp->server);

    kernel_packet(&difp->txpkt, difp->req_ip, CLIENT_PORT, difp->server, SERVER_PORT);
}

// ------------------------------------------------------------------------
// Receive DHCPC packets
//
static int receive_packet(dhcpc_if *difp)
{
    int s, datalen;
    struct timeval tv;

    tv.tv_sec = difp->secs;
    tv.tv_usec = 0;

    // Set next receive timeval
    difp->secs += 2 + (arc4random() & 3);

    s = dhcpc_rcv(difp->listen_socket, &tv);
    if (s == 0 || s == -1)
    {
        if (--difp->countdown == 0) //timeout
            return 2;

        return 3;
    }

    datalen = LIB_rcvinfo_read(s, (char *)&difp->rxpkt, sizeof(struct dhcpc));
    LIB_rcvinfo_close(s);
    if (datalen < 0)
    {
        diag_printf("couldn't read on listening socket, ignoring\n");
        return 1;
    }

    if (difp->rxpkt.xid   != difp->xid   ||
        difp->rxpkt.htype != difp->txpkt.htype ||
        difp->rxpkt.hlen  != difp->txpkt.hlen  ||
        bcmp( &difp->rxpkt.chaddr, &difp->txpkt.chaddr, difp->txpkt.hlen))
    {
        return 1;
    }

    return 0;
}

static void free_dhcpc(dhcpc_if *ifp)
{
    ifp->state = 0;

#if 0
    if (ifp == dhcpc_interface)
    {
        dhcpc_interface = ifp->next;
    }
    else
    {
        for (preifp = dhcpc_interface ; preifp->next != ifp ; preifp = preifp->next)
            ;

        preifp->next = ifp->next;
    }

    // How about thread
    // How about mailbox
    free(ifp);
#endif
    return;
}

extern char chk_link;
void do_dhcpc(dhcpc_if *difp)
{
    cyg_uint8 *msgtype;
    unsigned char *server_ip;
    char *intf = difp->ifname;
    struct ip_set *setp = difp->setp;
    int rc;
    int cmd = 0;
    cyg_uint32 val;

    difp->listen_socket = 0;
    if ((difp->listen_socket = LIB_pktsocket_create(intf, dhcpc_check)) == 0)
        return;

    reset_countdown(difp);

    if (difp->state == DHCPSTATE_INIT ||
        difp->state == DHCPSTATE_REQUESTING)
    {
        new_xid(difp);
    }

   // netif_up(intf);

    while (1)
    {
        switch ( difp->state )
        {
            case DHCPSTATE_INIT:
                no_lease(difp);

                DHCPC_DBG("DHCPSTATE_INIT sending");
                send_discover(difp);

                difp->state = DHCPSTATE_SELECTING;
                break;

            case DHCPSTATE_SELECTING:

				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}
				
                rc = receive_packet(difp);
                if (rc == 2)
                {
                    new_lease( difp );
                    difp->state = DHCPSTATE_INIT;
                    DHCPC_OUT(0);
                }
                else if (rc == 3)
                {
                    difp->state = DHCPSTATE_INIT;
                    break;
                }
                else if (rc == 1)
                {
                    break;
                }


                DHCPC_DBG("DHCPSTATE_SELECTING received");

                if( (msgtype = dhcpc_get_option(&difp->rxpkt, DHCP_MESSAGE_TYPE, 0, NULL)) )
                {
                    if ( DHCPOFFER == *msgtype )
                    {

                        if ((server_ip = dhcpc_get_option(&difp->rxpkt, DHCP_SERVER_ID, 0, NULL)))
                            memcpy(&difp->server, server_ip, 4);

                        val = 0;
                        /*  Check lease time  */
                        if (dhcpc_get_option(&difp->rxpkt, DHCP_LEASE_TIME, sizeof(val), &val))
                        {
                            val = ntohl(val);
//                      DHCPC_DBG("DHCPSTATE_SELECTING lease = %u\n", val);
                            if (val <= DHCPC_MIN_LEASE_TIME)
                            {
                                DHCPC_DBG("DHCPC: lease too short, rejected\n");
                                break;
                            }
                        }

                        difp->req_ip = difp->rxpkt.yiaddr;

                        difp->state = DHCPSTATE_REQUESTING;
                        DHCPC_OUT(CMD_CHANGE_STATE);
                    }
                }
                break;

            case DHCPSTATE_REQUESTING:
                DHCPC_DBG("DHCPSTATE_REQUEST sending");
                send_request(difp);

                difp->state = DHCPSTATE_REQUEST_RECV;
				
				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}
				
                break;

            case DHCPSTATE_REQUEST_RECV:
				
				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}	
				
                rc = receive_packet(difp);
                if (rc == 2)
                {
                    difp->state = DHCPSTATE_INIT;
                    DHCPC_OUT(CMD_CHANGE_STATE);
                }
                else if (rc == 3)
                {
                    difp->state = DHCPSTATE_REQUESTING;
                    break;
                }
                else if (rc == 1)
                {
                    break;
                }

                DHCPC_DBG("DHCPSTATE_REQUEST_RECV received");
                server_ip = dhcpc_get_option(&difp->rxpkt, DHCP_SERVER_ID, 0, NULL);
				if (server_ip == NULL)
					{
					diag_printf("%s %d: server_ip = NULL\n",__FUNCTION__,__LINE__);
					break;
					}
				
                if( (msgtype = dhcpc_get_option(&difp->rxpkt, DHCP_MESSAGE_TYPE, 0, NULL)) )
                {
                    if ((DHCPACK == *msgtype) &&
                        ((difp->server == 0) || !memcmp(&difp->server, server_ip, sizeof(difp->server))))
                    {
                        if(difp->req_ip == difp->rxpkt.yiaddr)
                        {
                            cyg_uint32 tag = 0;

                            if (dhcpc_get_option(&difp->rxpkt, DHCP_LEASE_TIME, sizeof(tag), &tag))
                            {
                                tag = ntohl(tag);
                                DHCPC_DBG("DHCPSTATE_REQUEST_RECV lease = %ld", tag);
                                if (tag <= DHCPC_MIN_LEASE_TIME)
                                {
                                    DHCPC_DBG("DHCPC: lease too short, rejected\n");
                                    difp->state = DHCPSTATE_INIT;
                                    DHCPC_OUT(CMD_CHANGE_STATE);
                                }
                            }

                            memcpy(&difp->server, server_ip, 4);

                            new_lease(difp);
                            difp->state = DHCPSTATE_BOUND;
                            break;
                        }
                        else
                            DHCPC_DBG("assiged IP was not request IP");
                    }

                    if(DHCPNAK == *msgtype)
                        DHCPC_DBG("NAK received");

                    difp->state = DHCPSTATE_INIT;
                    DHCPC_OUT(CMD_CHANGE_STATE);
                    break;
                }
                break;

            case DHCPSTATE_BOUND:
            {
                unsigned int mask, gw, i;
                unsigned short mtu;
                char domain[256]= {0};
                char *dns;
                int cmd=0;
                int flag1=0, flag2=0;
				
				if(difp->countdown !=0)
				{
                mask = gw = mtu = 0;
                dhcpc_get_option(&difp->rxpkt, DHCP_SUBNET, sizeof(mask), &mask);
                dhcpc_get_option(&difp->rxpkt, DHCP_ROUTER, sizeof(gw), &gw);
                dhcpc_get_option(&difp->rxpkt, DHCP_DOMAIN_NAME, sizeof(domain), domain);
                dhcpc_get_option(&difp->rxpkt, DHCP_MTU, sizeof(mtu), &mtu);

                if(!mtu && (0== CFG_get( CFG_WAN_DHCP_MTU, &i)) )
                    mtu=(unsigned short)i;

                if(strcmp(domain, setp->domain))
                {
                    if( !strcmp(intf, LAN_NAME) )
                        cmd |= MOD_LAN|MOD_DNS;
                    else if( !strcmp(intf, WAN_NAME) )
                        cmd |= MOD_WAN|MOD_DNS;
                }

                if (!(difp->flags & DHCPC_IFFG_NODNS_UPDATE) &&
                    ((dns = dhcpc_get_option(&difp->rxpkt, DHCP_DNS_SERVER, 0, NULL)) != 0))
                {
                    int num = (int)*(dns-1);

                    if ((num % 4) == 0 && (num = num/4) != 0)
                    {
                        DNS_CFG_autodns_update(num, dns);
                        cmd |= MOD_DNS;
                    }
                }

                if (setp->server_ip != difp->server ||
                    setp->mtu != mtu ||
                    strcmp(domain, setp->domain) != 0)
                {
                    flag1 = 1;
                }

#if 0
                /* Temporal fixed the dhcpc bugs by Eddy */
                if ((strcmp(setp->ifname, intf)) || (setp->ip != difp->rxpkt.yiaddr) ||
                    (setp->mode != DHCPMODE) || (setp->gw_ip != gw) || (setp->netmask != mask))
#endif
                {
                    flag2 = 1;
                }

                if (flag1 || flag2)
                {
                    buid_ip_set(setp, intf, difp->rxpkt.yiaddr, mask, 0, gw,
                                difp->server, (int)mtu, DHCPMODE, domain, NULL);
                }

                if (cmd)
                    mon_snd_cmd(cmd);

                if (flag2)
                    DHCPC_inform_mon_change(intf);
				}
            }

            DHCPC_OUT(0);

            case DHCPSTATE_RENEWING:
                DHCPC_DBG("DHCPSTATE_RENEWING sending");
                send_renew_request(difp);

                difp->state = DHCPSTATE_RENEW_RECV;
				
				if(!chk_link)
					difp->state = DHCPSTATE_FREE;
				
                break;

            case DHCPSTATE_RENEW_RECV:
				
				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}
				
                rc = receive_packet(difp);
                if (rc == 2)
                {
                    difp->state = DHCPSTATE_BOUND;
                    break;
                }
                else if (rc == 3)
                {
                    difp->state = DHCPSTATE_RENEWING;
                    break;
                }
                else if (rc == 1)
                {
                    break;
                }

                DHCPC_DBG("DHCPSTATE_RENEW_RECV received");

                if( (msgtype = dhcpc_get_option(&difp->rxpkt, DHCP_MESSAGE_TYPE, 0, NULL)) )
                {
                    if ( DHCPACK == *msgtype && difp->rxpkt.yiaddr == difp->req_ip)
                    {
                        if ((server_ip = dhcpc_get_option(&difp->rxpkt, DHCP_SERVER_ID, 0, NULL)))
                            memcpy(&difp->server, server_ip, 4);

                        new_lease(difp);
                        difp->state = DHCPSTATE_BOUND;
                        break;
                    }
                    if ( DHCPNAK == *msgtype )
                    {
                        difp->state = DHCPSTATE_NOTBOUND;
                        break;
                    }
                }
                break;

            case DHCPSTATE_REBINDING:
                DHCPC_DBG("DHCPSTATE_REBINDING sending");
                send_rebind_request(difp);

                difp->state = DHCPSTATE_REBIND_RECV;
				
				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}

				
                break;

            case DHCPSTATE_REBIND_RECV:
				
				if(!chk_link)
				{
					difp->state = DHCPSTATE_FREE;
					break;
				}

                rc = receive_packet(difp);
                if (rc == 2)
                {
                    difp->state = DHCPSTATE_BOUND;
                    break;
                }
                else if (rc == 3)
                {
                    difp->state = DHCPSTATE_REBINDING;
                    break;
                }
                else if (rc == 1)
                {
                    break;
                }

                DHCPC_DBG("DHCPSTATE_REBIND_RECV received");

                if( (msgtype = dhcpc_get_option(&difp->rxpkt, DHCP_MESSAGE_TYPE, 0, NULL)) )
                {
                    if ( DHCPACK == *msgtype && difp->rxpkt.yiaddr == difp->req_ip)
                    {
                        if ((server_ip = dhcpc_get_option(&difp->rxpkt, DHCP_SERVER_ID, 0, NULL)))
                            memcpy(&difp->server, server_ip, 4);

                        new_lease(difp);
                        difp->state = DHCPSTATE_BOUND;
                        break;
                    }
                    else if ( DHCPNAK == *msgtype )
                    {
                        difp->state = DHCPSTATE_NOTBOUND;
                        break;
                    }
                }
                break;

            case DHCPSTATE_NOTBOUND:
                DHCPC_OUT(0);

            case DHCPSTATE_DO_RELEASE:
                DHCPC_DBG("DHCPSTATE_DO_RELEASE sending");
                send_release(difp);

                difp->state = DHCPSTATE_NOTBOUND;
                break;

            case DHCPSTATE_FREE:
                diag_printf("[DHCPC]: DHCPSTATE_FREE:\n");
                no_lease(difp);
                DHCPC_OUT(0);

            default:
                diag_printf("How get in default state?\n");
                DHCPC_OUT(0);
        }
    }

dhcpc_out:
    LIB_pktsocket_close(difp->listen_socket);

    if (cmd != 0)
        cyg_mbox_tryput(difp->mbox_id, (void *)cmd);

    return;
}

void DHCPC_daemon( cyg_addrword_t data)
{
    int val=0;
    dhcpc_if *ifp = (dhcpc_if *)data;
    unsigned int cmd;

    diag_printf("%s\n", __FUNCTION__);
    while ( 1 )
    {
        cmd = (unsigned int)cyg_mbox_get( ifp->mbox_id );

        switch (cmd)
        {
            case CMD_DO_RELEASE:
                ifp->user_flag = 1;     // User want to stop main thread
                if (ifp->state == DHCPSTATE_BOUND)
                {
                    ifp->state = DHCPSTATE_DO_RELEASE;
                }
                else
                {
                    DHCPC_inform_mon_stop(ifp->ifname);
                }
                break;

            case CMD_FREE:
                ifp->state = DHCPSTATE_FREE;
                break;

            case CMD_DO_RENEW:
                ifp->user_flag = 0;
                if (ifp->state == DHCPSTATE_BOUND)
                {
                    ifp->state = DHCPSTATE_RENEWING;
                }
                else
                {
#ifndef CONFIG_DHCPC_INIT_REBOOT_STATE_DISABLE
                    if(ifp->req_ip)
                        ifp->state = DHCPSTATE_REQUESTING;
                    else
#endif
                        ifp->state = DHCPSTATE_INIT;
                }

                // Do clear old lease anyway!
                no_lease(ifp);
                break;

            case CMD_TIMEOUT:
                if ((DHCPSTATE_DO_RELEASE != ifp->state) && (DHCPSTATE_NOTBOUND != ifp->state))
                {
                    struct dhcp_lease *lease = &ifp->lease;
                    cyg_uint8 lease_state = lease->curr;
                    lease->curr = 0;

                    if ( lease_state & DHCP_LEASE_EX )
                    {
                        ifp->setp->ip = ifp->setp->netmask = ifp->setp->gw_ip = 0;
                        ifp->state = DHCPSTATE_NOTBOUND;
                    }
                    else if ( lease_state & DHCP_LEASE_T2 )
                    {
                        ifp->state = DHCPSTATE_REBINDING;
                    }
                    else if ( lease_state & DHCP_LEASE_T1 )
                    {
                        ifp->state = DHCPSTATE_RENEWING;
                    }
                }
                break;

            case CMD_CHANGE_STATE:
            default:
                break;
        }

        do_dhcpc(ifp);

        if (ifp->state == DHCPSTATE_NOTBOUND)
        {
            diag_printf("[DHCPC]: IP from DHCP was expired!!\n");

            // Change to init state
            ifp->state = DHCPSTATE_INIT;
            DHCPC_inform_mon_stop(ifp->ifname);
            cyg_thread_delay(200);

            if (!ifp->user_flag)    //connection timeout
            {
                if ((CFG_get(CFG_WAN_DHCP_MAN, &val) != -1) && (val == 1))
                {
                    free_dhcpc(ifp);
                }
                else
                {
                    DHCPC_inform_mon_change(ifp->ifname);
                }
            }
        }
        else if (ifp->state == DHCPSTATE_FREE)
        {
            free_dhcpc(ifp);
        }
    }
}
