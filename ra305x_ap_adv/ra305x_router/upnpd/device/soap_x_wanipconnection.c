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


#include <upnp.h>
#include <upnp_portmap.h>

#ifndef __ECOS
#include <sys/sysinfo.h>
#endif


/***********************************************************
  WARNNING: PLEASE LEAVE YOUR CODES BETWEEN 
           "<< USER CODE BEGIN" AND ">> USER CODE END"
  AND DON'T REMOVE TAG :
           "<< AUTO GENERATED FUNCTION: "
           ">> AUTO GENERATED FUNCTION"
           "<< USER CODE BEGIN"
           ">> USER CODE END"
************************************************************/

//<< AUTO GENERATED FUNCTION: statevar_ConnectionType()
static int statevar_ConnectionType
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
//<< USER CODE BEGIN
	
	strcpy(UPNP_STR(value), "IP_Routed");
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_PossibleConnectionTypes()
static int statevar_PossibleConnectionTypes
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
	UPNP_HINT(char *pUnconfigured = "Unconfigured";)
	UPNP_HINT(char *pIP_Routed = "IP_Routed";)
	UPNP_HINT(char *pIP_Bridged = "IP_Bridged";)

//<< USER CODE BEGIN

	strcpy(UPNP_STR(value), "IP_Routed");
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_ConnectionStatus()
static int statevar_ConnectionStatus
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
	UPNP_HINT( char *pUnconfigured = "Unconfigured"; )
	UPNP_HINT( char *pConnected = "Connected"; )
	UPNP_HINT( char *pDisconnected = "Disconnected"; )

//<< USER CODE BEGIN
	if(SYS_wan_up == CONNECTED)
		strcpy(UPNP_STR(value), "Connected");
	else
		strcpy(UPNP_STR(value), "Disconnected");
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_Uptime()
static int statevar_Uptime
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )
	
//<< USER CODE BEGIN
#ifdef __ECOS

	//UPNP_UI4(value) = time(0);
	if(SYS_wan_conntime)
		UPNP_UI4(value) = time(0) - SYS_wan_conntime;
	else
		UPNP_UI4(value) = 0;
	
#else

	struct sysinfo info;

	sysinfo(&info);
	UPNP_UI4(value) = info.uptime;
	
#endif

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_LastConnectionError()
static int statevar_LastConnectionError
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
	UPNP_HINT( char *pERROR_NONE = "ERROR_NONE"; )
	UPNP_HINT( char *pERROR_UNKNOWN = "ERROR_UNKNOWN"; )

//<< USER CODE BEGIN

	strcpy(UPNP_STR(value), "ERROR_NONE");
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_RSIPAvailable()
static int statevar_RSIPAvailable
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BOOL(value) )
	
//<< USER CODE BEGIN

	UPNP_BOOL(value) = 0;    /* FALSE */
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_NATEnabled()
static int statevar_NATEnabled
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BOOL(value) )
	
//<< USER CODE BEGIN
#ifdef __ECOS
	int nat;
	
	CFG_get(CFG_NAT_EN, &nat);
	UPNP_BOOL(value) = (nat == 1) ? 1 : 0;
#else
    char *nat = cf_GetValue("/etc/usrcfg/rc.conf", "ipnat_enable");
    
	UPNP_BOOL(value) = 1;    /* TRUE */
    
    if (!nat)
        return SOAP_DEVICE_INTERNAL_ERROR;
	
    if (strcasecmp(nat, "yes"))
        UPNP_BOOL(value) = 0;
#endif

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_PortMappingNumberOfEntries()
static int statevar_PortMappingNumberOfEntries
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI2(value) )
	
//<< USER CODE BEGIN

    /* XXX -- Port mapping implementation */
	UPNP_UI2(value) = portmap_num();

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_ExternalIPAddress()
static int statevar_ExternalIPAddress
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
//<< USER CODE BEGIN
#ifdef __ECOS

	strcpy(UPNP_STR(value), NSTR(SYS_wan_ip));
	
#else

    struct ifreq ifr;
    int fd;
    char wanif[IFNAMSIZ];
    char *wan_opt;
    char *ptr;

    strcpy(UPNP_STR(value), "");

    wan_opt = cf_GetValue("/etc/usrcfg/interface.conf", "wan_opt");
    if (!wan_opt)
    {
        return SOAP_DEVICE_INTERNAL_ERROR;
    }
    
    if (strcasecmp(wan_opt, "dhcpc") == 0 || 
        strcasecmp(wan_opt, "static") == 0)
    {
        ptr = cf_GetValue("/etc/usrcfg/interface.conf", "wan_if");
        if (!ptr)
            return SOAP_DEVICE_INTERNAL_ERROR;
        strcpy(wanif, ptr);
    }
	else if (strcasecmp(wan_opt, "pppoe") == 0)
	{
		strcpy(wanif, "ppp0");
		/* overwrite wanif if multiple pppoe enabled */
		ptr = cf_GetValue("/etc/usrcfg/rc.conf", "pppoe_multiple");
		if (ptr && strcasecmp(ptr, "yes") == 0)
		{
			FILE *fp = fopen("/tmp/default_ppp", "r");
			if (fp)
			{
				if (fgets(wanif, IFNAMSIZ-1, fp) != NULL)
					wanif[strlen(wanif)-1] = 0;
				else
					strcpy(wanif, "ppp0");
				fclose(fp);
			}
		}
	}
    else if (strcasecmp(wan_opt, "pptp") == 0 ||
             strcasecmp(wan_opt, "l2tp") == 0)
    {
        strcpy(wanif, "ppp0");
    }
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    strcpy(ifr.ifr_name, wanif);
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        strcpy(UPNP_STR(value), inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }
    
    close(fd);
	
#endif

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetConnectionType()
static int action_SetConnectionType
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewConnectionType = UPNP_IN_ARG("NewConnectionType"); )
	
	UPNP_HINT( UPNP_STR(in_NewConnectionType) )
	
//<< USER CODE BEGIN

	UPNP_INPUT *in_NewConnectionType = UPNP_IN_ARG("NewConnectionType");
	
	if (strcmp(UPNP_STR(in_NewConnectionType), "IP_Routed") != 0)
		return SOAP_INVALID_ARGS;
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetConnectionTypeInfo()
static int action_GetConnectionTypeInfo
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewConnectionType = UPNP_OUT_ARG("NewConnectionType"); )
	UPNP_HINT( UPNP_VALUE *out_NewPossibleConnectionTypes = UPNP_OUT_ARG("NewPossibleConnectionTypes"); )
	
	UPNP_HINT( UPNP_STR(out_NewConnectionType) )
	UPNP_HINT( UPNP_STR(out_NewPossibleConnectionTypes) )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewConnectionType = UPNP_OUT_ARG("NewConnectionType");
	UPNP_VALUE *out_NewPossibleConnectionTypes = UPNP_OUT_ARG("NewPossibleConnectionTypes");
	
    strcpy(UPNP_STR(out_NewConnectionType), "IP_Routed");
    strcpy(UPNP_STR(out_NewPossibleConnectionTypes), "IP_Routed");
    
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_RequestConnection()
static int action_RequestConnection
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{

//<< USER CODE BEGIN

    if(SYS_wan_up == DISCONNECTED)
    	mon_snd_cmd( MON_CMD_WAN_INIT );
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_ForceTermination()
static int action_ForceTermination
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{

//<< USER CODE BEGIN

    if(SYS_wan_up == CONNECTED)
    {
    	if(SYS_wan_mode == STATICMODE)
    		mon_snd_cmd( MON_CMD_WAN_UPDATE );
    	else
    		mon_snd_cmd( MON_CMD_WAN_STOP );
    		
    	netif_clear_cnt(WAN_NAME);
    }
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetStatusInfo()
static int action_GetStatusInfo
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewConnectionStatus = UPNP_OUT_ARG("NewConnectionStatus"); )
	UPNP_HINT( UPNP_VALUE *out_NewLastConnectionError = UPNP_OUT_ARG("NewLastConnectionError"); )
	UPNP_HINT( UPNP_VALUE *out_NewUptime = UPNP_OUT_ARG("NewUptime"); )
	
	UPNP_HINT( UPNP_STR(out_NewConnectionStatus) )
	UPNP_HINT( UPNP_STR(out_NewLastConnectionError) )
	UPNP_HINT( UPNP_UI4(out_NewUptime) )
	
//<< USER CODE BEGIN
	UPNP_VALUE *out_NewConnectionStatus = UPNP_OUT_ARG("NewConnectionStatus");
	UPNP_VALUE *out_NewLastConnectionError = UPNP_OUT_ARG("NewLastConnectionError");
	UPNP_VALUE *out_NewUptime = UPNP_OUT_ARG("NewUptime");
	
	int ret;
	
    ret = statevar_ConnectionStatus(service, out_NewConnectionStatus, error_string);
    if (ret != OK)
        return ret;

    ret = statevar_LastConnectionError(service, out_NewLastConnectionError, error_string);
    if (ret != OK)
        return ret;
    
    return statevar_Uptime(service, out_NewUptime, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetNATRSIPStatus()
static int action_GetNATRSIPStatus
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewRSIPAvailable = UPNP_OUT_ARG("NewRSIPAvailable"); )
	UPNP_HINT( UPNP_VALUE *out_NewNATEnabled = UPNP_OUT_ARG("NewNATEnabled"); )
	
	UPNP_HINT( UPNP_BOOL(out_NewConnectionStatus) )
	UPNP_HINT( UPNP_BOOL(out_NewLastConnectionError) )

//<< USER CODE BEGIN
	UPNP_VALUE *out_NewRSIPAvailable = UPNP_OUT_ARG("NewRSIPAvailable");
	UPNP_VALUE *out_NewNATEnabled = UPNP_OUT_ARG("NewNATEnabled");
	
    UPNP_BOOL(out_NewRSIPAvailable) = 0; /* FALSE */
    UPNP_BOOL(out_NewNATEnabled) = 1; /* TRUE */

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetGenericPortMappingEntry()
static int action_GetGenericPortMappingEntry
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewPortMappingIndex = UPNP_IN_ARG("NewPortMappingIndex"); )
	UPNP_HINT( UPNP_VALUE *out_NewRemoteHost = UPNP_OUT_ARG("NewRemoteHost"); )
	UPNP_HINT( UPNP_VALUE *out_NewExternalPort = UPNP_OUT_ARG("NewExternalPort"); )
	UPNP_HINT( UPNP_VALUE *out_NewProtocol = UPNP_OUT_ARG("NewProtocol"); )
	UPNP_HINT( UPNP_VALUE *out_NewInternalPort = UPNP_OUT_ARG("NewInternalPort"); )
	UPNP_HINT( UPNP_VALUE *out_NewInternalClient = UPNP_OUT_ARG("NewInternalClient"); )
	UPNP_HINT( UPNP_VALUE *out_NewEnabled = UPNP_OUT_ARG("NewEnabled"); )
	UPNP_HINT( UPNP_VALUE *out_NewPortMappingDescription = UPNP_OUT_ARG("NewPortMappingDescription"); )
	UPNP_HINT( UPNP_VALUE *out_NewLeaseDuration = UPNP_OUT_ARG("NewLeaseDuration"); )
	
	UPNP_HINT( UPNP_UI2(in_NewPortMappingIndex) )
	UPNP_HINT( UPNP_STR(out_NewRemoteHost) )
	UPNP_HINT( UPNP_UI2(out_NewExternalPort) )
	UPNP_HINT( UPNP_STR(out_NewProtocol) )
	UPNP_HINT( UPNP_UI2(out_NewInternalPort) )
	UPNP_HINT( UPNP_STR(out_NewInternalClient) )
	UPNP_HINT( UPNP_BOOL(out_NewEnabled) )
	UPNP_HINT( UPNP_STR(out_NewPortMappingDescription) )
	UPNP_HINT( UPNP_UI4(out_NewLeaseDuration) )

//<< USER CODE BEGIN
	UPNP_INPUT *in_NewPortMappingIndex = UPNP_IN_ARG("NewPortMappingIndex");
	UPNP_VALUE *out_NewRemoteHost = UPNP_OUT_ARG("NewRemoteHost");
	UPNP_VALUE *out_NewExternalPort = UPNP_OUT_ARG("NewExternalPort");
	UPNP_VALUE *out_NewProtocol = UPNP_OUT_ARG("NewProtocol");
	UPNP_VALUE *out_NewInternalPort = UPNP_OUT_ARG("NewInternalPort");
	UPNP_VALUE *out_NewInternalClient = UPNP_OUT_ARG("NewInternalClient");
	UPNP_VALUE *out_NewEnabled = UPNP_OUT_ARG("NewEnabled");
	UPNP_VALUE *out_NewPortMappingDescription = UPNP_OUT_ARG("NewPortMappingDescription");
	UPNP_VALUE *out_NewLeaseDuration = UPNP_OUT_ARG("NewLeaseDuration");
   
    UPNP_PORTMAP *map;
    
    map = portmap_with_index(UPNP_UI2(in_NewPortMappingIndex));
    
    if (!map)
        return SOAP_INVALID_ARGS;
    
    strcpy(UPNP_STR(out_NewRemoteHost), map->remote_host);
    UPNP_UI2(out_NewExternalPort) = map->external_port;
    strcpy(UPNP_STR(out_NewProtocol), map->protocol);
    UPNP_UI2(out_NewInternalPort) = map->internal_port;
    strcpy(UPNP_STR(out_NewInternalClient), map->internal_client);
    UPNP_BOOL(out_NewEnabled) = map->enable;
    strcpy(UPNP_STR(out_NewPortMappingDescription), map->description);
    UPNP_UI4(out_NewLeaseDuration) = map->duration;

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetSpecificPortMappingEntry()
static int action_GetSpecificPortMappingEntry
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost"); )
	UPNP_HINT( UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort"); )
	UPNP_HINT( UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol"); )
	UPNP_HINT( UPNP_VALUE *out_NewInternalPort = UPNP_OUT_ARG("NewInternalPort"); )
	UPNP_HINT( UPNP_VALUE *out_NewInternalClient = UPNP_OUT_ARG("NewInternalClient"); )
	UPNP_HINT( UPNP_VALUE *out_NewEnabled = UPNP_OUT_ARG("NewEnabled"); )
	UPNP_HINT( UPNP_VALUE *out_NewPortMappingDescription = UPNP_OUT_ARG("NewPortMappingDescription"); )
	UPNP_HINT( UPNP_VALUE *out_NewLeaseDuration = UPNP_OUT_ARG("NewLeaseDuration"); )
	
	UPNP_HINT( UPNP_STR(in_NewRemoteHost) )
	UPNP_HINT( UPNP_UI2(in_NewExternalPort) )
	UPNP_HINT( UPNP_STR(in_NewProtocol) )
	UPNP_HINT( UPNP_UI2(out_NewInternalPort) )
	UPNP_HINT( UPNP_STR(out_NewInternalClient) )
	UPNP_HINT( UPNP_BOOL(out_NewEnabled) )
	UPNP_HINT( UPNP_STR(out_NewPortMappingDescription) )
	UPNP_HINT( UPNP_UI4(out_NewLeaseDuration) )
	
//<< USER CODE BEGIN
	
	UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost");
	UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort");
	UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol");
	UPNP_VALUE *out_NewInternalPort = UPNP_OUT_ARG("NewInternalPort");
	UPNP_VALUE *out_NewInternalClient = UPNP_OUT_ARG("NewInternalClient");
	UPNP_VALUE *out_NewEnabled = UPNP_OUT_ARG("NewEnabled");
	UPNP_VALUE *out_NewPortMappingDescription = UPNP_OUT_ARG("NewPortMappingDescription");
	UPNP_VALUE *out_NewLeaseDuration = UPNP_OUT_ARG("NewLeaseDuration");
	
    UPNP_PORTMAP *map;
#ifdef __ECOS
    map = find_portmap(UPNP_STR(in_NewRemoteHost), 
                       UPNP_UI2(in_NewExternalPort),
                       UPNP_STR(in_NewProtocol),
                       SYS_wan_if);
#else
    char *wan_if = cf_GetValue("/etc/usrcfg/interface.conf", "wan_if");

    if (!wan_if)
        return SOAP_DEVICE_INTERNAL_ERROR;

    map = find_portmap(UPNP_STR(in_NewRemoteHost), 
                       UPNP_UI2(in_NewExternalPort),
                       UPNP_STR(in_NewProtocol),
                       wan_if);
#endif
    if (!map)
        return SOAP_INVALID_ARGS;
    
    UPNP_UI2(out_NewInternalPort) = map->internal_port;
    strcpy(UPNP_STR(out_NewInternalClient), map->internal_client);
    UPNP_BOOL(out_NewEnabled) = map->enable;
    strcpy(UPNP_STR(out_NewPortMappingDescription), map->description);
    UPNP_UI4(out_NewLeaseDuration) = map->duration;

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_AddPortMapping()
static int action_AddPortMapping
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost"); )
	UPNP_HINT( UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort"); )
	UPNP_HINT( UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol"); )
	UPNP_HINT( UPNP_INPUT *in_NewInternalPort = UPNP_IN_ARG("NewInternalPort"); )
	UPNP_HINT( UPNP_INPUT *in_NewInternalClient = UPNP_IN_ARG("NewInternalClient"); )
	UPNP_HINT( UPNP_INPUT *in_NewEnabled = UPNP_IN_ARG("NewEnabled"); )
	UPNP_HINT( UPNP_INPUT *in_NewPortMappingDescription = UPNP_IN_ARG("NewPortMappingDescription"); )
	UPNP_HINT( UPNP_INPUT *in_NewLeaseDuration = UPNP_IN_ARG("NewLeaseDuration"); )
	
	UPNP_HINT( UPNP_STR(in_NewRemoteHost) )
	UPNP_HINT( UPNP_UI2(in_NewExternalPort) )
	UPNP_HINT( UPNP_STR(in_NewProtocol) )
	UPNP_HINT( UPNP_UI2(in_NewInternalPort) )
	UPNP_HINT( UPNP_STR(in_NewInternalClient) )
	UPNP_HINT( UPNP_BOOL(in_NewEnabled) )
	UPNP_HINT( UPNP_STR(in_NewPortMappingDescription) )
	UPNP_HINT( UPNP_UI4(in_NewLeaseDuration) )
	
//<< USER CODE BEGIN

	UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost");
	UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort");
	UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol");
	UPNP_INPUT *in_NewInternalPort = UPNP_IN_ARG("NewInternalPort");
	UPNP_INPUT *in_NewInternalClient = UPNP_IN_ARG("NewInternalClient");
	UPNP_INPUT *in_NewEnabled = UPNP_IN_ARG("NewEnabled");
	UPNP_INPUT *in_NewPortMappingDescription = UPNP_IN_ARG("NewPortMappingDescription");
	UPNP_INPUT *in_NewLeaseDuration = UPNP_IN_ARG("NewLeaseDuration");

    char buf[sizeof("65535")];
    if (add_portmap(UPNP_STR(in_NewRemoteHost),
                    UPNP_UI2(in_NewExternalPort), 
                    UPNP_STR(in_NewProtocol),
                    UPNP_UI2(in_NewInternalPort),
                    UPNP_STR(in_NewInternalClient),
                    UPNP_BOOL(in_NewEnabled),
                    UPNP_STR(in_NewPortMappingDescription),
                    UPNP_UI4(in_NewLeaseDuration),
                    SYS_wan_if) != 0)
    {
        return SOAP_INVALID_ARGS;
    }
    
    sprintf(buf, "%u", portmap_num());
    gena_update_event_var(service, "PortMappingNumberOfEntries", buf);

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_DeletePortMapping()
static int action_DeletePortMapping
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost"); )
	UPNP_HINT( UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort"); )
	UPNP_HINT( UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol"); )
	
	UPNP_HINT( UPNP_STR(in_NewRemoteHost) )
	UPNP_HINT( UPNP_UI2(in_NewExternalPort) )
	UPNP_HINT( UPNP_STR(in_NewProtocol) )
	
//<< USER CODE BEGIN
	UPNP_INPUT *in_NewRemoteHost = UPNP_IN_ARG("NewRemoteHost");
	UPNP_INPUT *in_NewExternalPort = UPNP_IN_ARG("NewExternalPort");
	UPNP_INPUT *in_NewProtocol = UPNP_IN_ARG("NewProtocol");

    char buf[sizeof("65535")];
    if (del_portmap(UPNP_STR(in_NewRemoteHost),
                    UPNP_UI2(in_NewExternalPort),
                    UPNP_STR(in_NewProtocol),
                    SYS_wan_if) != 0)
    {
        return SOAP_INVALID_ARGS;
    }
	
    sprintf(buf, "%u", portmap_num());
    gena_update_event_var(service, "PortMappingNumberOfEntries", buf);

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetExternalIPAddress()
static int action_GetExternalIPAddress
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewExternalIPAddress = UPNP_OUT_ARG("NewExternalIPAddress"); )
	
	UPNP_HINT( UPNP_STR(out_NewExternalIPAddress) )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewExternalIPAddress = UPNP_OUT_ARG("NewExternalIPAddress");
	
    return statevar_ExternalIPAddress(service, out_NewExternalIPAddress, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< TABLE BEGIN
/***************************************************
  WARNNING: DON'T MODIFY THE FOLLOWING TABLES
  AND DON'T REMOVE TAG :
           "<< TABLE BEGIN"
           ">> TABLE END"
*****************************************************/

//Evented Variables
static UPNP_EVENTED evented_ConnectionStatus;
static UPNP_EVENTED evented_ExternalIPAddress;
static UPNP_EVENTED evented_PortMappingNumberOfEntries;
static UPNP_EVENTED evented_PossibleConnectionTypes;

//State Variable Table
UPNP_STATE_VAR statevar_x_wanipconnection[] =
{
	{"ConnectionStatus",                   UPNP_TYPE_STR,       &statevar_ConnectionStatus,                   &evented_ConnectionStatus},
	{"ConnectionType",                     UPNP_TYPE_STR,       &statevar_ConnectionType,                     0},
	{"ExternalIPAddress",                  UPNP_TYPE_STR,       &statevar_ExternalIPAddress,                  &evented_ExternalIPAddress},
	{"ExternalPort",                       UPNP_TYPE_UI2,       0,                                            0},
	{"InternalClient",                     UPNP_TYPE_STR,       0,                                            0},
	{"InternalPort",                       UPNP_TYPE_UI2,       0,                                            0},
	{"LastConnectionError",                UPNP_TYPE_STR,       &statevar_LastConnectionError,                0},
	{"NATEnabled",                         UPNP_TYPE_BOOL,      &statevar_NATEnabled,                         0},
	{"PortMappingDescription",             UPNP_TYPE_STR,       0,                                            0},
	{"PortMappingEnabled",                 UPNP_TYPE_BOOL,      0,                                            0},
	{"PortMappingLeaseDuration",           UPNP_TYPE_UI4,       0,                                            0},
	{"PortMappingNumberOfEntries",         UPNP_TYPE_UI2,       &statevar_PortMappingNumberOfEntries,         &evented_PortMappingNumberOfEntries},
	{"PortMappingProtocol",                UPNP_TYPE_STR,       0,                                            0},
	{"PossibleConnectionTypes",            UPNP_TYPE_STR,       &statevar_PossibleConnectionTypes,            &evented_PossibleConnectionTypes},
	{"RSIPAvailable",                      UPNP_TYPE_BOOL,      &statevar_RSIPAvailable,                      0},
	{"RemoteHost",                         UPNP_TYPE_STR,       0,                                            0},
	{"Uptime",                             UPNP_TYPE_UI4,       &statevar_Uptime,                             0},
	{0,                                    0,                   0,                                            0}
};

//Action Table
static UPNP_ARGUMENT arg_in_SetConnectionType [] =
{
	{"NewConnectionType",             UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetConnectionTypeInfo [] =
{
	{"NewConnectionType",             UPNP_TYPE_STR},
	{"NewPossibleConnectionTypes",    UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetStatusInfo [] =
{
	{"NewConnectionStatus",           UPNP_TYPE_STR},
	{"NewLastConnectionError",        UPNP_TYPE_STR},
	{"NewUptime",                     UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_out_GetNATRSIPStatus [] =
{
	{"NewRSIPAvailable",              UPNP_TYPE_BOOL},
	{"NewNATEnabled",                 UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_in_GetGenericPortMappingEntry [] =
{
	{"NewPortMappingIndex",           UPNP_TYPE_UI2}
};

static UPNP_ARGUMENT arg_out_GetGenericPortMappingEntry [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR},
	{"NewExternalPort",               UPNP_TYPE_UI2},
	{"NewProtocol",                   UPNP_TYPE_STR},
	{"NewInternalPort",               UPNP_TYPE_UI2},
	{"NewInternalClient",             UPNP_TYPE_STR},
	{"NewEnabled",                    UPNP_TYPE_BOOL},
	{"NewPortMappingDescription",     UPNP_TYPE_STR},
	{"NewLeaseDuration",              UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_in_GetSpecificPortMappingEntry [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR},
	{"NewExternalPort",               UPNP_TYPE_UI2},
	{"NewProtocol",                   UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetSpecificPortMappingEntry [] =
{
	{"NewInternalPort",               UPNP_TYPE_UI2},
	{"NewInternalClient",             UPNP_TYPE_STR},
	{"NewEnabled",                    UPNP_TYPE_BOOL},
	{"NewPortMappingDescription",     UPNP_TYPE_STR},
	{"NewLeaseDuration",              UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_in_AddPortMapping [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR},
	{"NewExternalPort",               UPNP_TYPE_UI2},
	{"NewProtocol",                   UPNP_TYPE_STR},
	{"NewInternalPort",               UPNP_TYPE_UI2},
	{"NewInternalClient",             UPNP_TYPE_STR},
	{"NewEnabled",                    UPNP_TYPE_BOOL},
	{"NewPortMappingDescription",     UPNP_TYPE_STR},
	{"NewLeaseDuration",              UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_in_DeletePortMapping [] =
{
	{"NewRemoteHost",                 UPNP_TYPE_STR},
	{"NewExternalPort",               UPNP_TYPE_UI2},
	{"NewProtocol",                   UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetExternalIPAddress [] =
{
	{"NewExternalIPAddress",          UPNP_TYPE_STR}
};

UPNP_ACTION action_x_wanipconnection[] =
{
	{"AddPortMapping",                          8,       arg_in_AddPortMapping,                        0,       0,                                            &action_AddPortMapping},
	{"DeletePortMapping",                       3,       arg_in_DeletePortMapping,                     0,       0,                                            &action_DeletePortMapping},
	{"ForceTermination",                        0,       0,                                            0,       0,                                            &action_ForceTermination},
	{"GetConnectionTypeInfo",                   0,       0,                                            2,       arg_out_GetConnectionTypeInfo,                &action_GetConnectionTypeInfo},
	{"GetExternalIPAddress",                    0,       0,                                            1,       arg_out_GetExternalIPAddress,                 &action_GetExternalIPAddress},
	{"GetGenericPortMappingEntry",              1,       arg_in_GetGenericPortMappingEntry,            8,       arg_out_GetGenericPortMappingEntry,           &action_GetGenericPortMappingEntry},
	{"GetNATRSIPStatus",                        0,       0,                                            2,       arg_out_GetNATRSIPStatus,                     &action_GetNATRSIPStatus},
	{"GetSpecificPortMappingEntry",             3,       arg_in_GetSpecificPortMappingEntry,           5,       arg_out_GetSpecificPortMappingEntry,          &action_GetSpecificPortMappingEntry},
	{"GetStatusInfo",                           0,       0,                                            3,       arg_out_GetStatusInfo,                        &action_GetStatusInfo},
	{"RequestConnection",                       0,       0,                                            0,       0,                                            &action_RequestConnection},
	{"SetConnectionType",                       1,       arg_in_SetConnectionType,                     0,       0,                                            &action_SetConnectionType},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END
