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

#include <network.h>
#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <cfg_def.h>
#include <dhcpc.h>
#include <sys_status.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void DHCPC_daemon( cyg_addrword_t data);

dhcpc_if *dhcpc_interface = NULL;

void DHCPC_inform_mon_change( char *intf)
{
    int cmd;

    if( !strcmp(intf, LAN_NAME) )
        cmd = MON_CMD_LAN_INIT;
    else if( !strcmp(intf, WAN_NAME) )
        cmd = MON_CMD_WAN_INIT;
    else
        return;

    mon_snd_cmd( cmd );
}

void DHCPC_inform_mon_stop( char *intf)
{
    int cmd;

    if( !strcmp(intf, LAN_NAME) )
        cmd = MON_CMD_LAN_STOP;
    else if( !strcmp(intf, WAN_NAME) )
        cmd = MON_CMD_WAN_STOP;
    else
        return;

    mon_snd_cmd( cmd );
}

dhcpc_if *find_ifp(char *intf)
{
    dhcpc_if *ifp;

    for( ifp = dhcpc_interface; ifp ; ifp = ifp->next )
    {
        if( !strcmp(ifp->ifname, intf))
            break;
    }

    if(!ifp)
    {
        printf("%s do not do DHCPC_init()\n", intf);
    }

    return ifp;
}

void DHCPC_free( char *intf)
{
    dhcpc_if *ifp;

    ifp = find_ifp(intf);
    if(!ifp) return;

    cyg_mbox_tryput(ifp->mbox_id, (void *)CMD_FREE);
}

void DHCPC_release_one(char *intf )
{
    dhcpc_if *ifp;

    ifp = find_ifp(intf);
    if (!ifp)
        return;

    cyg_mbox_tryput(ifp->mbox_id, (void *)CMD_DO_RELEASE);
}

void DHCPC_renew(char *intf )
{
    dhcpc_if *ifp;

    ifp = find_ifp(intf);
    if (!ifp)
    {
        DHCPC_inform_mon_change(intf);
        return;
    }

    cyg_mbox_tryput(ifp->mbox_id, (void *)CMD_DO_RENEW);
}

cyg_uint8 DHCPC_state(char *intf)
{
    dhcpc_if *ifp;

    ifp = find_ifp(intf);
    if(ifp)
    {
        return ifp->state;
    }
    else
    {
        return 0;
    }
}

void DHCPC_init( char *intf, struct ip_set *setp, unsigned int req_ip)
{
    dhcpc_if *ifpt, *ifp;
    int flag=0;
	char *ptr  = NULL;

	if (intf ==NULL) 
		{
		diag_printf("%s:%d intf =NULL\n",__FUNCTION__,__LINE__);
		return ;
		}
    ifp = find_ifp(intf);
    if (ifp && (ifp->setp == setp))
        flag = 1;

    if (!flag)
    {
        /* no dhcpc interface exist and create new one */
        ifp = (dhcpc_if *)malloc(sizeof(dhcpc_if));
		if (ifp ==NULL)
			{
			diag_printf("%s:%d ifp malloc fail\n",__FUNCTION__,__LINE__);
			return ;
			}
        memset(ifp, 0, sizeof(*ifp));

        if (!dhcpc_interface)
            dhcpc_interface = ifp;
        else
        {
            for( ifpt = dhcpc_interface; ifpt->next ; ifpt = ifpt->next ) ;
            ifpt->next = ifp;
        }
    }

    //--
    // Should we move these code to dhcpc.c ????
    ifp->ifname = intf;
	ptr =net_get_maca(intf);
	if (ptr == NULL)
		{
		diag_printf("ERROR:%s %d :ptr = NULL(this shouldn't happan)\n",__FUNCTION__,__LINE__);
		return ;
		}
    memcpy(ifp->macaddr,ptr, 6);
    ifp->ifnetp = ifunit(intf);
    ifp->setp = setp;
    ifp->req_ip = req_ip;
    ifp->server = 0;
    ifp->flags = 0;

    bzero( ifp->setp, sizeof( struct ip_set ) );
    bzero( &ifp->lease, sizeof( struct dhcp_lease ) );
    //--

    if (!flag)
    {
        cyg_mbox_create( &ifp->mbox_id, &ifp->mbox_obj );

        cyg_thread_create
        (
            8,                                  /* scheduling info (eg pri) */
            (cyg_thread_entry_t *)DHCPC_daemon, /* entry point function */
            (cyg_addrword_t)ifp,                /* entry data */
            "DHCPC",                            /* optional thread name */
            ifp->dhcpc_stack,                   /* stack base, NULL = alloc */
			4096,//STACK_SIZE,							/* stack size, 0 = default */
            &ifp->dhcpc_thread_h,               /* returned thread handle */
            &ifp->dhcpc_thread                  /* put thread here */
        );

        cyg_thread_resume(ifp->dhcpc_thread_h);
    }

    return;
}

int DHCPC_remaining_time(char *intf)
{
    dhcpc_if *ifp;

    ifp = find_ifp(intf);
    if (!ifp ||
        ifp->lease.time_stamp == 0 ||
        (cyg_current_time() - ifp->lease.time_stamp) > ifp->lease.expiry)
    {
        return 0;
    }

    if (ifp->lease.expiry == 0xffffffffUL)
        return ifp->lease.expiry;

    return (ifp->lease.time_stamp + ifp->lease.expiry - cyg_current_time()) /100;
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
cyg_bool_t DHCPC_set_flags( const char *intf, cyg_uint16 flags )
{
    dhcpc_if *difp;

    for(difp = dhcpc_interface; difp; difp = difp->next)
    {
        if( !strcmp(difp->ifname, intf))
        {
            difp->flags = flags;
            return 0;
        }
    }

    printf("dhcpc_setfg: %s do not do DHCPC_init()\n", intf);
    return -1;
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
cyg_bool_t DHCPC_get_flags( const char *intf, cyg_uint16 *flags )
{
    dhcpc_if *difp;

    if (flags == NULL)
        return -1;

    for(difp = dhcpc_interface; difp; difp = difp->next)
    {
        if( !strcmp(difp->ifname, intf))
        {
            *flags = difp->flags;
            return 0;
        }
    }
    printf("dhcpc_getfg: %s do not do DHCPC_init()\n", intf);
    return -1;
}


void DHCPC_stop_DNS_update(const char *intf)
{
    cyg_uint16 flags;

    if ((DHCPC_get_flags(intf, &flags) == 0) && !(flags & DHCPC_IFFG_NODNS_UPDATE))
        DHCPC_set_flags(intf, flags | DHCPC_IFFG_NODNS_UPDATE);

}


void DHCPC_start_DNS_update(const char *intf)
{
    cyg_uint16 flags;

    if ((DHCPC_get_flags(intf, &flags) == 0) && (flags & DHCPC_IFFG_NODNS_UPDATE))
        DHCPC_set_flags(intf, flags & ~DHCPC_IFFG_NODNS_UPDATE);

}


