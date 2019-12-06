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
//#include <net/if.h>


#ifdef __ECOS
extern unsigned int netif_get_cnt(const char * ifn, int id);
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

//<< AUTO GENERATED FUNCTION: statevar_WANAccessType()
static int statevar_WANAccessType
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
	UPNP_HINT(char *pDSL = "DSL";)
	UPNP_HINT(char *pPOTS = "POTS";)
	UPNP_HINT(char *pCable = "Cable";)
	UPNP_HINT(char *pEthernet = "Ethernet";)
	UPNP_HINT(char *pOther = "Other";)

//<< USER CODE BEGIN
    
    strcpy(UPNP_STR(value), "Ethernet");

    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_Layer1UpstreamMaxBitRate()
static int statevar_Layer1UpstreamMaxBitRate
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN

	UPNP_UI4(value) = 100000000;    /* 100 Mbps */

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_Layer1DownstreamMaxBitRate()
static int statevar_Layer1DownstreamMaxBitRate
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN

	UPNP_UI4(value) = 100000000;    /* 100 Mbps */

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_PhysicalLinkStatus()
static int statevar_PhysicalLinkStatus
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
	UPNP_HINT(char *pUp = "Up";)
	UPNP_HINT(char *pDown = "Down";)
	UPNP_HINT(char *pInitializing = "Initializing";)
	UPNP_HINT(char *pUnavailable = "Unavailable";)

//<< USER CODE BEGIN

	strcpy(UPNP_STR(value), "Up");
	
    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_EnabledForInternet()
static int statevar_EnabledForInternet
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BOOL(value) )

//<< USER CODE BEGIN

	UPNP_BOOL(value) = 1;    /* TRUE */

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_MaximumActiveConnections()
static int statevar_MaximumActiveConnections
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI2(value) )

//<< USER CODE BEGIN

	UPNP_UI2(value) = 1;

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_NumberOfActiveConnections()
static int statevar_NumberOfActiveConnections
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI2(value) )

//<< USER CODE BEGIN

	UPNP_UI2(value) = 1;

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_TotalBytesSent()
static int statevar_TotalBytesSent
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	UPNP_UI4(value) = netif_get_cnt(SYS_wan_if, 5);
	
#else

    int found = 0;
    char line[255];
    char ifname[IFNAMSIZ];
    int pos;
    FILE *fp;
    u_long rx_bytes, rx_packets, rx_errors, rx_drop, rx_fifo, rx_frame, rx_compress, rx_mcast, tx_bytes, tx_packets, tx_errors, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    char *wan_opt;
    char wanif[IFNAMSIZ];
	char *ptr;

    UPNP_UI4(value) = 0;
    
    wan_opt = cf_GetValue("/etc/usrcfg/interface.conf", "wan_opt");
    if (!wan_opt)
        return SOAP_DEVICE_INTERNAL_ERROR;

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
			fp = fopen("/tmp/default_ppp", "r");
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
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    /* skip heading line */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strlen(line)-1] = 0;
        ptr = line;
        pos = strcspn(ptr, ":");
        ptr[pos] = 0;
        sscanf(ptr, "%s", ifname);
        ptr += pos+1;
        sscanf(ptr, "%8lu %7lu %4lu %4lu %4lu %5lu %10lu %9lu %8lu %7lu %4lu %4lu %4lu %5lu %7lu %10lu",
        &rx_bytes, &rx_packets, &rx_errors, &rx_drop, &rx_fifo, &rx_frame, &rx_compress, &rx_mcast, &tx_bytes, &tx_packets, &tx_errors, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);

        if (strcmp(wanif, ifname) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    UPNP_UI4(value) = tx_bytes;
	
#endif

    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_TotalBytesReceived()
static int statevar_TotalBytesReceived
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	UPNP_UI4(value) = netif_get_cnt(SYS_wan_if, 6);
	
#else

    int found = 0;
    char line[255];
    char ifname[IFNAMSIZ];
    int pos;
    FILE *fp;
    u_long rx_bytes, rx_packets, rx_errors, rx_drop, rx_fifo, rx_frame, rx_compress, rx_mcast, tx_bytes, tx_packets, tx_errors, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    char *wan_opt;
    char wanif[IFNAMSIZ];
	char *ptr;

    UPNP_UI4(value) = 0;
    
    wan_opt = cf_GetValue("/etc/usrcfg/interface.conf", "wan_opt");
    if (!wan_opt)
        return SOAP_DEVICE_INTERNAL_ERROR;

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
			fp = fopen("/tmp/default_ppp", "r");
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
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    /* skip heading line */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strlen(line)-1] = 0;
        ptr = line;
        pos = strcspn(ptr, ":");
        ptr[pos] = 0;
        sscanf(ptr, "%s", ifname);
        ptr += pos+1;
        sscanf(ptr, "%8lu %7lu %4lu %4lu %4lu %5lu %10lu %9lu %8lu %7lu %4lu %4lu %4lu %5lu %7lu %10lu",
        &rx_bytes, &rx_packets, &rx_errors, &rx_drop, &rx_fifo, &rx_frame, &rx_compress, &rx_mcast, &tx_bytes, &tx_packets, &tx_errors, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);

        if (strcmp(wanif, ifname) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    UPNP_UI4(value) = rx_bytes;
	
#endif
	
    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_TotalPacketsSent()
static int statevar_TotalPacketsSent
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	UPNP_UI4(value) = netif_get_cnt(SYS_wan_if, 1);
	
#else
    int found = 0;
    char line[255];
    char ifname[IFNAMSIZ];
    int pos;
    FILE *fp;
    u_long rx_bytes, rx_packets, rx_errors, rx_drop, rx_fifo, rx_frame, rx_compress, rx_mcast, tx_bytes, tx_packets, tx_errors, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    char *wan_opt;
    char wanif[IFNAMSIZ];
	char *ptr;

    UPNP_UI4(value) = 0;
    
    wan_opt = cf_GetValue("/etc/usrcfg/interface.conf", "wan_opt");
    if (!wan_opt)
        return SOAP_DEVICE_INTERNAL_ERROR;

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
			fp = fopen("/tmp/default_ppp", "r");
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
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    /* skip heading line */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strlen(line)-1] = 0;
        ptr = line;
        pos = strcspn(ptr, ":");
        ptr[pos] = 0;
        sscanf(ptr, "%s", ifname);
        ptr += pos+1;
        sscanf(ptr, "%8lu %7lu %4lu %4lu %4lu %5lu %10lu %9lu %8lu %7lu %4lu %4lu %4lu %5lu %7lu %10lu",
        &rx_bytes, &rx_packets, &rx_errors, &rx_drop, &rx_fifo, &rx_frame, &rx_compress, &rx_mcast, &tx_bytes, &tx_packets, &tx_errors, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);

        if (strcmp(wanif, ifname) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    UPNP_UI4(value) = tx_packets;
#endif
	
    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_TotalPacketsReceived()
static int statevar_TotalPacketsReceived
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_UI4(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	UPNP_UI4(value) = netif_get_cnt(SYS_wan_if, 3);
	
#else

    int found = 0;
    char line[255];
    char ifname[IFNAMSIZ];
    int pos;
    FILE *fp;
    u_long rx_bytes, rx_packets, rx_errors, rx_drop, rx_fifo, rx_frame, rx_compress, rx_mcast, tx_bytes, tx_packets, tx_errors, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
    char *wan_opt;
    char wanif[IFNAMSIZ];
	char *ptr;

    UPNP_UI4(value) = 0;
    
    wan_opt = cf_GetValue("/etc/usrcfg/interface.conf", "wan_opt");
    if (!wan_opt)
        return SOAP_DEVICE_INTERNAL_ERROR;

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
			fp = fopen("/tmp/default_ppp", "r");
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
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    /* skip heading line */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        line[strlen(line)-1] = 0;
        ptr = line;
        pos = strcspn(ptr, ":");
        ptr[pos] = 0;
        sscanf(ptr, "%s", ifname);
        ptr += pos+1;
        sscanf(ptr, "%8lu %7lu %4lu %4lu %4lu %5lu %10lu %9lu %8lu %7lu %4lu %4lu %4lu %5lu %7lu %10lu",
        &rx_bytes, &rx_packets, &rx_errors, &rx_drop, &rx_fifo, &rx_frame, &rx_compress, &rx_mcast, &tx_bytes, &tx_packets, &tx_errors, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);

        if (strcmp(wanif, ifname) == 0)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    if (!found)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    UPNP_UI4(value) = rx_packets;
	
#endif

    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_ActiveConnectionDeviceContainer()
static int statevar_ActiveConnectionDeviceContainer
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
    UPNP_SERVICE *wan_service = 0;
	
	wan_service = get_service("/control?WANIPConnection");
    if (!wan_service)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    sprintf(UPNP_STR(value), "uuid:%s:WANConnectionDevice:1", wan_service->uuid);

    return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_ActiveConnectionServiceID()
static int statevar_ActiveConnectionServiceID
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
	UPNP_SERVICE *wan_service = 0;
	
	wan_service = get_service("/control?WANIPConnection");
	if (!wan_service)
		return SOAP_DEVICE_INTERNAL_ERROR;
	
	sprintf(UPNP_STR(value), "urn:upnp-org:serviceId:%s", wan_service->service_id);
	
	return OK;
	
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetEnabledForInternet()
static int action_SetEnabledForInternet
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewEnabledForInternet = UPNP_IN_ARG("NewEnabledForInternet"); )
	
	UPNP_HINT( UPNP_BOOL(in_NewEnabledForInternet) )

//<< USER CODE BEGIN

	/* XXX -- should take action */
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetEnabledForInternet()
static int action_GetEnabledForInternet
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewEnabledForInternet = UPNP_OUT_ARG("NewEnabledForInternet"); )
	
	UPNP_HINT( UPNP_BOOL(out_NewEnabledForInternet) )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewEnabledForInternet = UPNP_OUT_ARG("NewEnabledForInternet");
	
	return statevar_EnabledForInternet(service,
                                       out_NewEnabledForInternet,
                                       error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetCommonLinkProperties()
static int action_GetCommonLinkProperties
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewWANAccessType = UPNP_OUT_ARG("NewWANAccessType"); )
	UPNP_HINT( UPNP_VALUE *out_NewLayer1UpstreamMaxBitRate = UPNP_OUT_ARG("NewLayer1UpstreamMaxBitRate"); )
	UPNP_HINT( UPNP_VALUE *out_NewLayer1DownstreamMaxBitRate = UPNP_OUT_ARG("NewLayer1DownstreamMaxBitRate"); )
	UPNP_HINT( UPNP_VALUE *out_NewPhysicalLinkStatus = UPNP_OUT_ARG("NewPhysicalLinkStatus"); )

	UPNP_HINT( UPNP_STR(out_NewWANAccessType) )
	UPNP_HINT( UPNP_UI4(out_NewLayer1UpstreamMaxBitRate) )
	UPNP_HINT( UPNP_UI4(out_NewLayer1DownstreamMaxBitRate) )
	UPNP_HINT( UPNP_STR(out_NewPhysicalLinkStatus) )
	
//<< USER CODE BEGIN
	UPNP_VALUE *out_NewWANAccessType = UPNP_OUT_ARG("NewWANAccessType");
	UPNP_VALUE *out_NewLayer1UpstreamMaxBitRate = UPNP_OUT_ARG("NewLayer1UpstreamMaxBitRate");
	UPNP_VALUE *out_NewLayer1DownstreamMaxBitRate = UPNP_OUT_ARG("NewLayer1DownstreamMaxBitRate");
	UPNP_VALUE *out_NewPhysicalLinkStatus = UPNP_OUT_ARG("NewPhysicalLinkStatus");

    strcpy(UPNP_STR(out_NewWANAccessType), "Ethernet");
    UPNP_UI4(out_NewLayer1UpstreamMaxBitRate) = 100000000;   /* 100 Mbps */
    UPNP_UI4(out_NewLayer1DownstreamMaxBitRate) = 100000000; /* 100 Mbps */
    strcpy(UPNP_STR(out_NewPhysicalLinkStatus), "Up");
    
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetMaximumActiveConnections()
static int action_GetMaximumActiveConnections
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewMaximumActiveConnections = UPNP_OUT_ARG("NewMaximumActiveConnections"); )
	
	UPNP_HINT( UPNP_UI2(out_NewMaximumActiveConnections) )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewMaximumActiveConnections = UPNP_OUT_ARG("NewMaximumActiveConnections");
	
	return statevar_MaximumActiveConnections(service,
                                             out_NewMaximumActiveConnections,
                                             error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetTotalBytesSent()
static int action_GetTotalBytesSent
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewTotalBytesSent = UPNP_OUT_ARG("NewTotalBytesSent"); )

	UPNP_HINT( UPNP_UI4(out_NewTotalBytesSent) )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewTotalBytesSent = UPNP_OUT_ARG("NewTotalBytesSent");
	
	return statevar_TotalBytesSent(service,
                                   out_NewTotalBytesSent,
                                   error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetTotalPacketsSent()
static int action_GetTotalPacketsSent
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewTotalPacketsSent = UPNP_OUT_ARG("NewTotalPacketsSent"); )
	
	UPNP_HINT( UPNP_UI4(out_NewTotalPacketsSent) )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewTotalPacketsSent = UPNP_OUT_ARG("NewTotalPacketsSent");
	
	return statevar_TotalPacketsSent(service,
                                     out_NewTotalPacketsSent,
                                     error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetTotalBytesReceived()
static int action_GetTotalBytesReceived
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewTotalBytesReceived = UPNP_OUT_ARG("NewTotalBytesReceived"); )
	
	UPNP_HINT( UPNP_UI4(out_NewTotalBytesReceived) )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewTotalBytesReceived = UPNP_OUT_ARG("NewTotalBytesReceived");
	
	return statevar_TotalBytesReceived(service,
                                       out_NewTotalBytesReceived,
                                       error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetTotalPacketsReceived()
static int action_GetTotalPacketsReceived
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewTotalPacketsReceived = UPNP_OUT_ARG("NewTotalPacketsReceived"); )
	
	UPNP_HINT( UPNP_UI4(out_NewTotalPacketsReceived) )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewTotalPacketsReceived = UPNP_OUT_ARG("NewTotalPacketsReceived");
	
	return statevar_TotalPacketsReceived(service,
                                        out_NewTotalPacketsReceived,
                                        error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetActiveConnections()
static int action_GetActiveConnections
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewActiveConnectionIndex = UPNP_IN_ARG("NewActiveConnectionIndex"); )
	UPNP_HINT( UPNP_VALUE *out_NewActiveConnDeviceContainer = UPNP_OUT_ARG("NewActiveConnDeviceContainer"); )
	UPNP_HINT( UPNP_VALUE *out_NewActiveConnectionServiceID = UPNP_OUT_ARG("NewActiveConnectionServiceID"); )
	
	UPNP_HINT( UPNP_UI2(in_NewActiveConnectionIndex) )
	UPNP_HINT( UPNP_STR(out_NewActiveConnDeviceContainer) )
	UPNP_HINT( UPNP_STR(out_NewActiveConnectionServiceID) )
	
//<< USER CODE BEGIN
	UPNP_VALUE *out_NewActiveConnDeviceContainer = UPNP_OUT_ARG("NewActiveConnDeviceContainer");
	UPNP_VALUE *out_NewActiveConnectionServiceID = UPNP_OUT_ARG("NewActiveConnectionServiceID");
	
    UPNP_SERVICE *wan_service = 0;
	
	wan_service = get_service("/control?WANIPConnection");
    if (!wan_service)
        return SOAP_DEVICE_INTERNAL_ERROR;
    
    sprintf(UPNP_STR(out_NewActiveConnDeviceContainer), 
            "uuid:%s:WANConnectionDevice:1",
            wan_service->uuid);

    sprintf(UPNP_STR(out_NewActiveConnectionServiceID),
            "urn:upnp-org:serviceId:%s",
            wan_service->service_id);

    return OK;

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
static UPNP_EVENTED evented_EnabledForInternet;
static UPNP_EVENTED evented_PhysicalLinkStatus;

//State Variable Table
UPNP_STATE_VAR statevar_x_wancommoninterfaceconfig[] =
//the following state variable are all "out" type!
//no one calls statevar_ActiveConnectionDeviceContainer() 
{
	{"ActiveConnectionDeviceContainer",    UPNP_TYPE_STR,       &statevar_ActiveConnectionDeviceContainer,    0},
	{"ActiveConnectionServiceID",          UPNP_TYPE_STR,       &statevar_ActiveConnectionServiceID,          0},
	{"EnabledForInternet",                 UPNP_TYPE_BOOL,      &statevar_EnabledForInternet,                 &evented_EnabledForInternet},
	{"Layer1DownstreamMaxBitRate",         UPNP_TYPE_UI4,       &statevar_Layer1DownstreamMaxBitRate,         0},
	{"Layer1UpstreamMaxBitRate",           UPNP_TYPE_UI4,       &statevar_Layer1UpstreamMaxBitRate,           0},
	{"MaximumActiveConnections",           UPNP_TYPE_UI2,       &statevar_MaximumActiveConnections,           0},
	{"NumberOfActiveConnections",          UPNP_TYPE_UI2,       &statevar_NumberOfActiveConnections,          0},
	{"PhysicalLinkStatus",                 UPNP_TYPE_STR,       &statevar_PhysicalLinkStatus,                 &evented_PhysicalLinkStatus},
	{"TotalBytesReceived",                 UPNP_TYPE_UI4,       &statevar_TotalBytesReceived,                 0},
	{"TotalBytesSent",                     UPNP_TYPE_UI4,       &statevar_TotalBytesSent,                     0},
	{"TotalPacketsReceived",               UPNP_TYPE_UI4,       &statevar_TotalPacketsReceived,               0},
	{"TotalPacketsSent",                   UPNP_TYPE_UI4,       &statevar_TotalPacketsSent,                   0},
	{"WANAccessType",                      UPNP_TYPE_STR,       &statevar_WANAccessType,                      0},
	{0,                                    0,                   0,                                            0}
};

//Action Table
static UPNP_ARGUMENT arg_in_SetEnabledForInternet [] =
{
	{"NewEnabledForInternet",         UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_out_GetEnabledForInternet [] =
{
	{"NewEnabledForInternet",         UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_out_GetCommonLinkProperties [] =
{
	{"NewWANAccessType",              UPNP_TYPE_STR},
	{"NewLayer1UpstreamMaxBitRate",   UPNP_TYPE_UI4},
	{"NewLayer1DownstreamMaxBitRate", UPNP_TYPE_UI4},
	{"NewPhysicalLinkStatus",         UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetMaximumActiveConnections [] =
{
	{"NewMaximumActiveConnections",   UPNP_TYPE_UI2}
};

static UPNP_ARGUMENT arg_out_GetTotalBytesSent [] =
{
	{"NewTotalBytesSent",             UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_out_GetTotalPacketsSent [] =
{
	{"NewTotalPacketsSent",           UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_out_GetTotalBytesReceived [] =
{
	{"NewTotalBytesReceived",         UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_out_GetTotalPacketsReceived [] =
{
	{"NewTotalPacketsReceived",       UPNP_TYPE_UI4}
};

static UPNP_ARGUMENT arg_in_GetActiveConnections [] =
{
	{"NewActiveConnectionIndex",      UPNP_TYPE_UI2}
};

static UPNP_ARGUMENT arg_out_GetActiveConnections [] =
{
	{"NewActiveConnDeviceContainer",  UPNP_TYPE_STR},
	{"NewActiveConnectionServiceID",  UPNP_TYPE_STR}
};

UPNP_ACTION action_x_wancommoninterfaceconfig[] =
{
	{"GetActiveConnections",                    1,       arg_in_GetActiveConnections,                  2,       arg_out_GetActiveConnections,                 &action_GetActiveConnections},
	{"GetCommonLinkProperties",                 0,       0,                                            4,       arg_out_GetCommonLinkProperties,              &action_GetCommonLinkProperties},
	{"GetEnabledForInternet",                   0,       0,                                            1,       arg_out_GetEnabledForInternet,                &action_GetEnabledForInternet},
	{"GetMaximumActiveConnections",             0,       0,                                            1,       arg_out_GetMaximumActiveConnections,          &action_GetMaximumActiveConnections},
	{"GetTotalBytesReceived",                   0,       0,                                            1,       arg_out_GetTotalBytesReceived,                &action_GetTotalBytesReceived},
	{"GetTotalBytesSent",                       0,       0,                                            1,       arg_out_GetTotalBytesSent,                    &action_GetTotalBytesSent},
	{"GetTotalPacketsReceived",                 0,       0,                                            1,       arg_out_GetTotalPacketsReceived,              &action_GetTotalPacketsReceived},
	{"GetTotalPacketsSent",                     0,       0,                                            1,       arg_out_GetTotalPacketsSent,                  &action_GetTotalPacketsSent},
	{"SetEnabledForInternet",                   1,       arg_in_SetEnabledForInternet,                 0,       0,                                            &action_SetEnabledForInternet},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END
