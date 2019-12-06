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
#include <unistd.h>
#include <network.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cfg_net.h>
#include <sys_status.h>
#include <lib_packet.h>
#include <lib_util.h>

#include <dhcpd.h>
#include <leases.h>
#include <common_subr.h>

extern struct server_config_t server_config;
extern struct dhcpOfferedAddr *leases;

//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_FlushStaticLeases
//
// DESCRIPTION
//      DHCPD_Sleases
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
void DHCPD_ClearLease(u_int8_t *chaddr, struct in_addr yiaddr)
{
    unsigned int i, j;

    for (j=0; j<6 && !chaddr[j]; j++);

    for (i=0; i<server_config.max_leases; i++)
    {
        if ((j != 6 && memcmp(leases[i].chaddr, chaddr, 6) == 0) ||
            (yiaddr.s_addr && leases[i].yiaddr.s_addr == yiaddr.s_addr))
        {
            if (leases[i]._hostname)
                free(leases[i]._hostname);

            memset(&leases[i], 0, sizeof(struct dhcpOfferedAddr));
        }
    }
}

void DHCPD_FlushLeaseEntry(struct dhcpOfferedAddr *lease)
{
    if (lease->_hostname)
        free(lease->_hostname);

    memset(lease, 0, sizeof(*lease));
}

void DHCPD_FlushLeases(void)
{
    unsigned int i;

    for (i = 0; i < server_config.max_leases; i++)
    {
        if (leases[i]._hostname)
            free(leases[i]._hostname);

        memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
    }
}

void DHCPD_FlushStaticLeases(void)
{
    unsigned int i;

    for (i = 0; i < server_config.max_leases; i++)
    {
        if (leases[i].flag & STATICL)
        {
            if (leases[i]._hostname)
                free(leases[i]._hostname);

            memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
        }
    }
}

//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_AddLease
//
// DESCRIPTION
//      Add a lease into the table, clearing out any old ones
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
struct dhcpOfferedAddr *DHCPD_AddLease(unsigned char *chaddr, struct in_addr yiaddr, unsigned long lease, unsigned char *_hostname, int restore)
{
    struct dhcpOfferedAddr *oldest;
    int len;
    int flag = 0;

    oldest = DHCPD_FindLeaseByChaddr(chaddr);
    if (oldest)
    {
        flag = oldest->flag;
    }
    else
    {
        oldest = DHCPD_OldestExpiredLease();
    }

    if (oldest)
    {
        DHCPD_FlushLeaseEntry(oldest);

        memcpy(oldest->chaddr, chaddr, 6);
        oldest->yiaddr = yiaddr;
        if (restore)
            oldest->expires = lease;
        else
            oldest->expires = GMTtime(0) + lease;

        if (_hostname)
        {
            // add hostname
            len = strlen(_hostname);
            oldest->_hostname = malloc(len+1);
			if(oldest->_hostname == NULL)
				return NULL;
            memcpy(oldest->_hostname, _hostname, len);
            oldest->_hostname[len] = '\0';
        }

        oldest->flag = flag;
    }

    return oldest;
}

//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_FindAndSetLease
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
struct dhcpOfferedAddr *DHCPD_FindAndSetLease(unsigned char *chaddr, struct in_addr yiaddr, int flag)
{
    struct dhcpOfferedAddr *lease;

    lease = DHCPD_FindLeaseByChaddr(chaddr);
    if (lease)
    {
        if (flag & DELETED)
        {
            DHCPD_FlushLeaseEntry(lease);
            return 0;
        }

        if (lease->yiaddr.s_addr != yiaddr.s_addr)
        {
            DHCPD_FlushLeaseEntry(lease);
        }
    }
    else
    {
        lease = DHCPD_OldestExpiredLease();
		if(lease)
        	DHCPD_FlushLeaseEntry(lease);
    }

    if (lease)
    {
        memcpy(lease->chaddr, chaddr, 6);
        lease->yiaddr = yiaddr;
        lease->flag = flag;
    }

    return lease;
}


//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_LeaseExpired
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
int DHCPD_LeaseExpired(struct dhcpOfferedAddr *lease)
{
    return (lease->expires < (unsigned long) GMTtime(0) && !(lease->flag & STATICL));
}

struct dhcpOfferedAddr *DHCPD_OldestExpiredLease(void)
{
    struct dhcpOfferedAddr *oldest = NULL;
    unsigned long oldest_lease = GMTtime(0);
    unsigned int i;

    if (oldest_lease == 0)
        oldest_lease = 1;

    for (i = 0; i < server_config.max_leases; i++)
    {
        if (oldest_lease > leases[i].expires && (leases[i].flag & STATICL) == 0)
        {
            oldest_lease = leases[i].expires;
            oldest = &(leases[i]);
        }
    }
    return oldest;

}


//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_FindLeaseByChaddr
//
// DESCRIPTION
//      Find the first lease that matches chaddr, NULL if no match
//
// PARAMETERS
//
//
// RETURN
//
//
//------------------------------------------------------------------------------
struct dhcpOfferedAddr *DHCPD_FindLeaseByChaddr(u_int8_t *chaddr)
{
    unsigned int i;

    for (i = 0; i < server_config.max_leases; i++)
    {
        if (memcmp(leases[i].chaddr, chaddr, 6) == 0)
            return &(leases[i]);
    }

    return NULL;
}

struct dhcpOfferedAddr *DHCPD_FindLeaseByYiaddr(struct in_addr yiaddr)
{
    unsigned int i;

    for (i = 0; i < server_config.max_leases; i++)
    {
        if (leases[i].yiaddr.s_addr == yiaddr.s_addr)
            return &leases[i];
    }

    return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_FindAddress
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
struct in_addr DHCPD_FindAddress(int check_expired)
{
    in_addr_t addr;
    struct in_addr ret;
    struct dhcpOfferedAddr *lease = 0;

    for(addr=ntohl(server_config.start.s_addr);
        addr <= ntohl(server_config.end.s_addr);
        addr++)
    {
        // ie, xx.xx.xx.0 or xx.xx.xx.255 or itself
        if ((addr & 0xFF) == 0 ||
            (addr & 0xFF) == 0xFF ||
            (addr == ntohl(server_config.server.s_addr)) )
        {
            continue;
        }

        /* lease is not taken */
        ret.s_addr = htonl(addr);
        if ( ((lease = DHCPD_FindLeaseByYiaddr(ret)) == 0 || (check_expired  && DHCPD_LeaseExpired(lease)))
             && DHCPD_CheckIp(ret, 0, 1) == 0)
        {
            return ret;
        }
    }

    ret.s_addr = 0;
    return ret;
}


/* check is an IP is taken, if it is, add it to the lease table */
//------------------------------------------------------------------------------
// FUNCTION
//      DHCPD_CheckIp
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
int DHCPD_CheckIp(struct in_addr addr, unsigned char *chaddr, u_int32_t reserved)
{
    unsigned char testchaddr[6] = {0};
    unsigned char zerochaddr[6] = {0};
    struct in_addr ip, mask, gw;

    /* delete ARP entry */
    ip = addr;
    mask.s_addr = 0xffffffff;
    gw = addr;
    RT_del(NULL, &ip, &mask, &gw, 1);

    icmp_ping(ntohl(addr.s_addr), 0, 1000 );

    if (memcmp(netarp_get_one(addr.s_addr, testchaddr), zerochaddr, 6))
    {
        if (!chaddr || (chaddr && memcmp(chaddr, testchaddr, 6)))
        {
            diag_printf("%s belongs to someone\n", inet_ntoa(ip));
            if (reserved)
            {
                diag_printf("reserving %s for %ld seconds", inet_ntoa(ip), server_config.conflict_time);

                DHCPD_AddLease(testchaddr, addr, server_config.conflict_time, NULL, 0);
                DHCPD_FindAndSetLease(testchaddr, addr, RESERVED);
            }
            return 1;
        }
    }
    else
    {
        /* no station exist */
        RT_del(NULL, &ip, &mask, &gw, 1);
    }

    diag_printf("check ip passed\n");
    return 0;
}


typedef void pfun1(char *fmt, ...);

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//
// PARAMETERS
//      option: 0xb -> dump all leases pool
//              0x3 -> dump dynamic and static leases
//              0x2 -> dump static leases only
//              0x1 -> dump dynamic leases only
//
// RETURN
//
//
//------------------------------------------------------------------------------
int DHCPD_dumplease(pfun1 *dentry, int option, void *arg1)
{
    int i, j;
    struct in_addr addr;
    char buf[255];
    char *mac;
    char *_hostname;

    for(i=0, j=0 ; i<server_config.max_leases; i++)
    {
        if ((option & SHOW_ALL) == 0)
        {
            if (leases[i].yiaddr.s_addr == 0 ||
                memcmp(leases[i].chaddr, "\x00\x00\x00\x00\x00\x00", 6) == 0 ||
                DHCPD_LeaseExpired(&leases[i])
               )
            {
                continue;
            }
        }

        switch(leases[i].flag)
        {
            case DYNAMICL:
                if ((option & SHOW_DYNAMIC) == 0)
                    continue;
                break;

            case STATICL: //static
                if ((option & SHOW_STATIC) == 0)
                    continue;
                break;

            case DISABLED: // reseved
            default:
                if ((option & SHOW_ALL) == 0)
                    continue;
                break;
        }

        char NullMac[] = "00:00:00:00:00:00";
        mac = (char *)ether_ntoa((struct ether_addr *)&(leases[i].chaddr));
        if (strcmp(mac, NullMac) == 0)
            continue;

        addr = leases[i].yiaddr;
        _hostname = leases[i]._hostname;
        if (_hostname == 0)
            _hostname = "";

		if (j == 0)
       	 	sprintf(buf, "'%s;%s;%s;%d;%u'", _hostname, inet_ntoa(addr), mac, leases[i].flag, leases[i].expires);
		else	
        	{
        	sprintf(buf, ",\n'%s;%s;%s;%d;%u'", _hostname, inet_ntoa(addr), mac, leases[i].flag, leases[i].expires);
			if (j%31 == 0)
				cyg_thread_delay(25);
			}
			
        j++;
        if (dentry)
            dentry(buf, j, arg1);
    }
	if (j > 30)
	cyg_thread_delay(25);

    return j;
}

//------------------------------------------------------------------------------
// FUNCTION
//      fix_lease_error
//
// DESCRIPTION
//      while configuration has changed, original leases maybe need to fix(include
//  expired time, lease IP).  After leases fixed, we have to save back to flash.
//
// PARAMETERS
//      NONE
//
// RETURN
//      ret: 0 -> nothing changed, -1 -> something changed
//
//------------------------------------------------------------------------------
int fix_lease_error(void)
{
    int i;
    int ret=0;

    for(i=0 ; i<server_config.max_leases; i++)
    {
        if ((leases[i].yiaddr.s_addr != 0) && (leases[i].expires != 0))
        {
            /* if expire time excess than expire time   30 years, we think it was worng */
            if (leases[i].expires < (int)(86400*365*30))
            {
                leases[i].expires = leases[i].expires + GMTtime_offset();
                ret = -1;
            }
        }
        if (leases[i].yiaddr.s_addr != 0 &&
            ((ntohl(leases[i].yiaddr.s_addr) < ntohl(server_config.start.s_addr)) ||
             (ntohl(leases[i].yiaddr.s_addr) > ntohl(server_config.end.s_addr))) )
        {
            if ((leases[i].flag & STATICL) == STATICL)
            {
                continue;
            }
            else
            {
                /* clean old dynamic leases */
                if (leases[i]._hostname)
                    free(leases[i]._hostname);

                memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
                ret = -1;
            }
        }
    }

    return ret;
}

