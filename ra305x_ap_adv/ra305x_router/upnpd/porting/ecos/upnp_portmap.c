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

#include <porting.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <upnp.h>
#include <upnp_portmap.h>


extern	int NAT_setrule(char *natcmd);
extern	int NAT_delrule(char *natcmd);


UPNP_PORTMAP            *upnp_pmlist = 0;
static int              upnp_pm_limit;
static unsigned short   upnp_pm_num;

static  time_t          upnp_pm_seconds;


/*==============================================================================
 * Function:    command to add or delete from NAT engine
 *------------------------------------------------------------------------------
 * Description: Initialize port mapping list to ipfilter
 *
 *============================================================================*/
static	void
upnp_nat_config
(
	UPNP_PORTMAP *map
)
{
	char command[256], subcmd[50];
	
	char *Proto = map->protocol;
	int ExtPort = map->external_port;
	char *IntIP = map->internal_client;
	int IntPort = map->internal_port;
	char *rthost = map->remote_host;
	int i;
	
	for (i=0; i<PRIMARY_WANIP_NUM; i++) {
		if ((primary_wan_ip_set[i].up != CONNECTED) || 
			!(primary_wan_ip_set[i].flags & IPSET_FG_NATIF))
			continue;
			
		if (rthost[0] == '\0')
		{
			sprintf(subcmd, "%s/32 port %d",
					NSTR(primary_wan_ip_set[i].ip), 
					ExtPort);
		}
		else
		{
			sprintf(subcmd, "from %s/32 to %s/32 port = %d",
					rthost, 
					NSTR(primary_wan_ip_set[i].ip), 
					ExtPort);
		}
		
		sprintf(command, "rdr %s %s -> %s port %d %s\n",
				primary_wan_ip_set[i].ifname, 
				subcmd, 
				IntIP, 
				IntPort, 
				Proto);
		
		// set to NAT kernel
		if (map->enable)
			NAT_setrule(command);
		else
			NAT_delrule(command);
	}
	
}


/*==============================================================================
 * Function:    
 *------------------------------------------------------------------------------
 * Description: 
 *
 *============================================================================*/
static	void upnp_nat_remove(UPNP_PORTMAP *map)
{
	char command[256], subcmd[50];
	
	char *Proto = map->protocol;
	int ExtPort = map->external_port;
	char *IntIP = map->internal_client;
	int IntPort = map->internal_port;
	char *rthost = map->remote_host;
	int i;
	
	for (i=0; i<PRIMARY_WANIP_NUM; i++) {
		if ((primary_wan_ip_set[i].up == CONNECTED) || 
			!(primary_wan_ip_set[i].flags & IPSET_FG_NATIF))
			continue;
			
		if (rthost[0] == '\0')
		{
			sprintf(subcmd, "%s/32 port %d",
					NSTR(primary_wan_ip_set[i].ip), 
					ExtPort);
		}
		else
		{
			sprintf(subcmd, "from %s/32 to %s/32 port = %d",
					rthost, 
					NSTR(primary_wan_ip_set[i].ip), 
					ExtPort);
		}
		
		sprintf(command, "rdr %s %s -> %s port %d %s\n",
				primary_wan_ip_set[i].ifname, 
				subcmd, 
				IntIP, 
				IntPort, 
				Proto);
		
		// delete NAT rule
		NAT_delrule(command);
	}
}

/*==============================================================================
 * Function:    upnp_portmap_into_nat()
 *------------------------------------------------------------------------------
 * Description: Initialize port mapping list to ipfilter
 *
 *============================================================================*/
void upnp_portmap_into_nat
(
	void
)
{
	UPNP_PORTMAP *map;
	int i;
	
	if (!upnp_pmlist)
		return;
	
	/* install all port mapping entries */   
	for (i=0; i<upnp_pm_num; i++)
	{
		map = upnp_pmlist + i;
		if (map->enable)
			upnp_nat_config(map);
	}
}


/*==============================================================================
 * Function:    upnp_portmap_into_nat()
 *------------------------------------------------------------------------------
 * Description: Initialize port mapping list to ipfilter
 *
 *============================================================================*/
void upnp_portmap_clean(void)
{
	UPNP_PORTMAP *map;
	int i;
	
	if (!upnp_pmlist)
		return;
	
	/* install all port mapping entries */   
	for (i=0; i<upnp_pm_num; i++)
	{
		map = upnp_pmlist + i;
		if (map->enable)
			upnp_nat_remove(map);
	}
}


/*==============================================================================
 * Function:    find_portmap_with_index()
 *------------------------------------------------------------------------------
 * Description: Find a port mapping entry with index
 *
 *============================================================================*/
UPNP_PORTMAP *portmap_with_index
(
    int index
)
{
    if (index >= upnp_pm_num)
        return NULL;
        
    return (upnp_pmlist+index);
}


/*==============================================================================
 * Function:    portmap_num()
 *------------------------------------------------------------------------------
 * Description: Get number of port mapping entries
 *
 *============================================================================*/
unsigned short portmap_num
(
	void
)
{
    return upnp_pm_num;
}


void upp_show_pmap
(
	void
)
{
	int i;
	printf("# UPnP Port Mappings\n");
	for (i=0; i<upnp_pm_num; i++)
	{
		printf("\n[%d]\n", i);
		printf("\tremote_host=%s\n",       upnp_pmlist[i].remote_host);
		printf("\texternal_port=%u\n",     upnp_pmlist[i].external_port);
		printf("\tprotocol=%s\n",          upnp_pmlist[i].protocol);
		printf("\tinternal_port=%u\n",     upnp_pmlist[i].internal_port);
		printf("\tinternal_client=%s\n",   upnp_pmlist[i].internal_client);
		printf("\tdescription=%s\n",       upnp_pmlist[i].description);
		printf("\tduration=%lu\n",         upnp_pmlist[i].duration);
		printf("\twan_if=%s\n",            upnp_pmlist[i].wan_if);
	}
}


/*==============================================================================
 * Function:    find_portmap()
 *------------------------------------------------------------------------------
 * Description: Find a port mapping entry
 *
 *============================================================================*/
UPNP_PORTMAP *find_portmap
(
    char            *remote_host,
    unsigned short  external_port,
    char            *protocol,
    char            *wan_if
)
{
    int i;
	UPNP_PORTMAP *map;
    
    for (i=0, map=upnp_pmlist; i<upnp_pm_num; i++, map++)
    {
        if (strcmp(map->remote_host, remote_host) == 0 &&
            map->external_port == external_port &&
            strcmp(map->protocol, protocol) == 0 &&
            strcmp(map->wan_if, wan_if) == 0)
        {
            return map;        
        }
    }
    
    return 0;
}


/*==============================================================================
 * Function:    del_portmap()
 *------------------------------------------------------------------------------
 * Description: Delete a port mapping entry
 *
 *============================================================================*/
static	void  purge_portmap_cache
(
	UPNP_PORTMAP	*map
)
{
	int index;
	int remainder;
	
	if (map->enable)
	{
		map->enable = 0;
		upnp_nat_config(map);
	}
	
	// Pull up remainder
	index = map - upnp_pmlist;
	remainder = upnp_pm_num - (index+1);
	
	memcpy(map, map+1, sizeof(*map)*remainder);
	upnp_pm_num--;
	
	return;
}


int del_portmap
(
    char            *remote_host,
    unsigned short  external_port,
    char            *protocol,
    char            *wan_if
)
{
	UPNP_PORTMAP *map;
	
	map = find_portmap(remote_host, external_port, protocol, wan_if);
	if (!map)
		return -1;
	
	// Purge this entry
	purge_portmap_cache(map);
	
	return 0;
}


/*==============================================================================
 * Function:    add_portmap()
 *------------------------------------------------------------------------------
 * Description: Add a new port mapping entry
 *
 *============================================================================*/
int add_portmap
(
	char            *remote_host,
	unsigned short  external_port,
	char            *protocol,
	unsigned short  internal_port,
	char            *internal_client,
	unsigned int    enable,
	char            *description,
	unsigned long   duration,
	char            *wan_if
)
{
	UPNP_PORTMAP *map;
	
	/* data validation */
	if (strcasecmp(protocol, "TCP") != 0 &&
		strcasecmp(protocol, "UDP") != 0)
	{
		printf( "add_portmap:: Invalid protocol");
		return -1;
	}
	
	/* check duplication */
	map = find_portmap(remote_host, external_port, protocol, wan_if);
	if (map)
	{
		if (strcmp(internal_client, map->internal_client) != 0)
			return -1;
			
		if (enable != map->enable || 
			internal_port != map->internal_port)
		{
			if (map->enable)
			{
				/* Argus, make it looked like shutdown */
				map->enable = 0;
				upnp_nat_config(map);
			}
		}
	}
	else
	{
		if (upnp_pm_num == upnp_pm_limit)
		{
			/* reallocation required */
			UPNP_PORTMAP *new_map;
		
			new_map = (UPNP_PORTMAP *)calloc(sizeof(*new_map), upnp_pm_limit*2);
			if (!new_map)
			{
				/* failure */
				return -1;
			}
			
			memcpy(new_map, upnp_pmlist, sizeof(*new_map)*upnp_pm_limit);
			
			free(upnp_pmlist);
			upnp_pmlist = new_map;
			
			upnp_pm_limit *= 2;
		}
		
		map = upnp_pmlist + upnp_pm_num;
		
		upnp_pm_num++;
	}
	
	/* Update database */
	map->external_port  = external_port;
	map->internal_port  = internal_port;
	map->enable         = enable;
	//map->duration		= (duration==0? 0 : duration + upnp_curr_time(0));
	map->duration		= (duration==0? 0 : duration + time(0));
	
	strcpy(map->remote_host,        remote_host);
	strcpy(map->protocol,           protocol);
	strcpy(map->internal_client,    internal_client);
	strcpy(map->description,        description);
	
	strcpy(map->wan_if, wan_if);
	
	// Set to NAT kernel
	if (map->enable)
		upnp_nat_config(map);
	
	return 0;
}


/*==============================================================================
 * Function:	upnp_portmap_timeout
 *------------------------------------------------------------------------------
 * Description: timed-out a port mapping entry
 *
 *============================================================================*/
void	upnp_portmap_timeout
(
	time_t now
)
{
	unsigned int past, systime;
	int	i;
	
	
	past = now - upnp_pm_seconds;
	systime = time(0);
	if (past != 0)
	{
		for (i=0; i<upnp_pm_num; i++)
		{
			UPNP_PORTMAP *map;
			
			map = upnp_pmlist + i;
			//if (map->duration != 0 && --map->duration == 0)
			if (map->duration != 0 && (systime >= map->duration))
			{
				// Purge this one
				purge_portmap_cache(map);
				
				i--;
				continue;
			}
		}
		
		// Update the new time
		upnp_pm_seconds = now;
	}
	
	return;
}


/*==============================================================================
 * Function:    upnp_portmap_free()
 *------------------------------------------------------------------------------
 * Description: Free port mapping list
 *
 *============================================================================*/
void upnp_portmap_free
(
	void
)
{
	UPNP_PORTMAP *map;
	int i;
	
	if (!upnp_pmlist)
		return;
	
	/* uninstall port mapping entries */
	for (i=0, map=upnp_pmlist; i<upnp_pm_num; i++, map++)
	{
		if (map->enable)
		{
			// Delete this one from NAT kernel
			map->enable = 0;
			upnp_nat_config(map);
		}
	}
	
	/* free port mapping list */
	upnp_pm_num = 0;
	upnp_pm_limit = 0;
	
	free(upnp_pmlist);
	upnp_pmlist = NULL;
}


/*==============================================================================
 * Function:    upnp_portmap_init()
 *------------------------------------------------------------------------------
 * Description: Initialize port mapping list
 *
 *============================================================================*/
int upnp_portmap_init
(
	void
)
{
	upnp_pm_limit = 0;
	upnp_pm_num = 0;
	
	upnp_pmlist = (UPNP_PORTMAP *)calloc(sizeof(*upnp_pmlist), UPNP_PM_SIZE);
	if (!upnp_pmlist)
	{
		printf( "Cannot allocate port mapping buffer");
		return -1;
	}
	
	upnp_pm_limit = UPNP_PM_SIZE;
	
	// Init portmap timeout
	upnp_curr_time(&upnp_pm_seconds);
	
	return 0;
}


