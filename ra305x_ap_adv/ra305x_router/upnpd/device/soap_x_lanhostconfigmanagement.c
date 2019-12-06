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


/***********************************************************
  WARNNING: PLEASE LEAVE YOUR CODES BETWEEN 
           "<< USER CODE BEGIN" AND ">> USER CODE END"
  AND DON'T REMOVE TAG :
           "<< AUTO GENERATED FUNCTION: "
           ">> AUTO GENERATED FUNCTION"
           "<< USER CODE BEGIN"
           ">> USER CODE END"
************************************************************/

//<< AUTO GENERATED FUNCTION: statevar_DHCPServerConfigurable()
static int statevar_DHCPServerConfigurable
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BOOL(value) )

//<< USER CODE BEGIN

    /* return TRUE if DHCP server is enabled */
	
#ifdef __ECOS

	CFG_get(CFG_LAN_DHCPD_EN, &(UPNP_BOOL(value)));
	
#else
	
    char *dhcpd_enable = cf_GetValue("/etc/usrcfg/rc.conf", "dhcpd_enable");
    
    if (!dhcpd_enable)
		UPNP_BOOL(value) = 0;
	else
	{
		if (strcasecmp(dhcpd_enable, "yes"))
			UPNP_BOOL(value) = 0;
		else
			UPNP_BOOL(value) = 1;
#endif

    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_DHCPRelay()
static int statevar_DHCPRelay
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_BOOL(value) )

//<< USER CODE BEGIN
	
	// We current don't support DHCP relay
	UPNP_BOOL(value) = 0;
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_SubnetMask()
static int statevar_SubnetMask
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	strcpy(UPNP_STR(value), "255.255.255.0");
	
#else
	
	char *lan_mask = cf_GetValue("/etc/usrcfg/rc.conf", "lan_mask");
	
	if (!lan_mask)
		strcpy(UPNP_STR(value), "255.255.255.0");
	else
		strcpy(UPNP_STR(value), lan_mask);
	
#endif

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_IPRouters()
static int statevar_IPRouters
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

    CFG_get_str(CFG_LAN_IP, UPNP_STR(value));
	
#else

	char *gateway = cf_GetValue("/etc/usrcfg/rc.conf", "lan_ip");
	
	if (!gateway)
		strcpy(UPNP_STR(value), "0.0.0.0");
	else
		strcpy(UPNP_STR(value), gateway);
	
#endif
	
    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_DNSServers()
static int statevar_DNSServers
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
#ifdef __ECOS

	strcpy(UPNP_STR(value), "168.95.1.1");
	
#else

    char *dns= cf_GetValue("/etc/usrcfg/rc.conf", "lan_ip");
	
	if (!dns)
		strcpy(UPNP_STR(value), "168.95.1.1");
	else
		strcpy(UPNP_STR(value), dns);
	
#endif

    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_DomainName()
static int statevar_DomainName
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value, 
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )
	
//<< USER CODE BEGIN
#ifdef __ECOS
	
    CFG_get_str(CFG_SYS_DOMAIN, UPNP_STR(value));
	
#else
	
	char *domain = cf_GetValue("/etc/usrcfg/rc.conf", "domain");
	
	if (!domain)
		strcpy(UPNP_STR(value), "localdomain");
	else
		strcpy(UPNP_STR(value), domain);
	
#endif
	
    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_MinAddress()
static int statevar_MinAddress
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN
	
#ifdef __ECOS

	CFG_get_str(CFG_LAN_DHCPD_START, UPNP_STR(value));
	
#else

	char *ptr;
	char lan_ip[16];
	int startip;
	int ip1, ip2, ip3;
	int pos;
	
	strcpy(UPNP_STR(value), "0.0.0.0");
	
	ptr = cf_GetValue("/etc/usrcfg/rc.conf", "lan_ip");
	if (!ptr)
		return SOAP_DEVICE_INTERNAL_ERROR;
	
	strcpy(lan_ip, ptr);
	
	ptr = cf_GetValue("/etc/usrcfg/rc.conf", "dhcpd_startip");
	if (!ptr)
		return SOAP_DEVICE_INTERNAL_ERROR;
	
	startip = atoi(ptr);
	
	ptr = lan_ip;
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip1 = atoi(ptr);
	
	ptr += (pos + 1);
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip2 = atoi(ptr);
	
	ptr += (pos + 1);
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip3 = atoi(ptr);
	
	sprintf(UPNP_STR(value), "%d.%d.%d.%d", ip1, ip2, ip3, startip);
	
#endif
	
    return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_MaxAddress()
static int statevar_MaxAddress
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN

#ifdef __ECOS
	
	CFG_get_str(CFG_LAN_DHCPD_END,  UPNP_STR(value));
	
#else
	
	char *ptr;
	char lan_ip[16];
	int endip;
	int ip1, ip2, ip3;
	int pos;
	
	strcpy(UPNP_STR(value), "0.0.0.0");
	
	ptr = cf_GetValue("/etc/usrcfg/rc.conf", "lan_ip");
	if (!ptr)
		return SOAP_DEVICE_INTERNAL_ERROR;
	
	strcpy(lan_ip, ptr);
	
	ptr = cf_GetValue("/etc/usrcfg/rc.conf", "dhcpd_endip");
	if (!ptr)
		return SOAP_DEVICE_INTERNAL_ERROR;
	
	endip = atoi(ptr);
	
	ptr = lan_ip;
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip1 = atoi(ptr);
	
	ptr += (pos + 1);
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip2 = atoi(ptr);
	
	ptr += (pos + 1);
	pos = strcspn(ptr, ".");
	ptr[pos] = 0;
	ip3 = atoi(ptr);
	
	sprintf(UPNP_STR(value), "%d.%d.%d.%d", ip1, ip2, ip3, endip);
	
#endif
	
	return OK;
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: statevar_ReservedAddresses()
static int statevar_ReservedAddresses
(
	UPNP_SERVICE * service,
	UPNP_VALUE * value,
	char * error_string
)
{
	UPNP_HINT( UPNP_STR(value) )

//<< USER CODE BEGIN

	/* XXX -- Should be static leases? */
	strcpy(UPNP_STR(value), "");

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetDHCPServerConfigurable()
static int action_SetDHCPServerConfigurable
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDHCPServerConfigurable = UPNP_IN_ARG("NewDHCPServerConfigurable"); )
	
	UPNP_HINT( UPNP_BOOL(in_NewDHCPServerConfigurable) )
	
//<< USER CODE BEGIN
	
	//if (IN_UPNP_BOOL(in_NewDHCPServerConfigurable) == 1)
	//{
	//	/* XXX -- enable dhcp server */
	//}
	//else
	//{
	//	/* *in_NewDHCPServerConfigurable == 0 */
	//	/* XXX -- disable dhcp server */
	//	
	//}
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDHCPServerConfigurable()
static int action_GetDHCPServerConfigurable
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDHCPServerConfigurable = UPNP_OUT_ARG("NewDHCPServerConfigurable"); )
	
	UPNP_HINT( UPNP_BOOL(out_NewDHCPServerConfigurable) )
	
//<< USER CODE BEGIN
	
	UPNP_VALUE *out_NewDHCPServerConfigurable = UPNP_OUT_ARG("NewDHCPServerConfigurable");
	
    return statevar_DHCPServerConfigurable(service, out_NewDHCPServerConfigurable, error_string);
    
//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetDHCPRelay()
static int action_SetDHCPRelay
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDHCPRelay = UPNP_IN_ARG("NewDHCPRelay"); )
	
	UPNP_HINT( UPNP_BOOL(in_NewDHCPRelay) )
	
//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDHCPRelay()
static int action_GetDHCPRelay
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDHCPRelay = UPNP_OUT_ARG("NewDHCPRelay"); )
	
	UPNP_HINT( UPNP_BOOL(out_NewDHCPRelay) )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewDHCPRelay = UPNP_OUT_ARG("NewDHCPRelay");
	
    return statevar_DHCPRelay(service, out_NewDHCPRelay, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetSubnetMask()
static int action_SetSubnetMask
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewSubnetMask = UPNP_IN_ARG("NewSubnetMask"); )
	
	UPNP_HINT( UPNP_STR(in_NewSubnetMask) )
	
//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetSubnetMask()
static int action_GetSubnetMask
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewSubnetMask = UPNP_OUT_ARG("NewSubnetMask"); )

	UPNP_HINT( UPNP_STR(out_NewSubnetMask); )
	
//<< USER CODE BEGIN
	
	UPNP_VALUE *out_NewSubnetMask = UPNP_OUT_ARG("NewSubnetMask");
	
    return statevar_SubnetMask(service, out_NewSubnetMask, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetIPRouter()
static int action_SetIPRouter
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewIPRouters = UPNP_IN_ARG("NewIPRouters"); )
	
	UPNP_HINT( UPNP_STR(in_NewIPRouters) )

//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_DeleteIPRouter()
static int action_DeleteIPRouter
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewIPRouters = UPNP_IN_ARG("NewIPRouters"); )
	
	UPNP_HINT( UPNP_STR(in_NewIPRouters) )

//<< USER CODE BEGIN
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetIPRoutersList()
static int action_GetIPRoutersList
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewIPRouters = UPNP_OUT_ARG("NewIPRouters"); )
	
	UPNP_HINT( UPNP_STR(out_NewIPRouters); )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewIPRouters = UPNP_OUT_ARG("NewIPRouters");
	
    return statevar_IPRouters(service, out_NewIPRouters, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetDomainName()
static int action_SetDomainName
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDomainName = UPNP_IN_ARG("in_NewDomainName"); )
	
	UPNP_HINT( UPNP_STR(in_NewDomainName) )
	
//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDomainName()
static int action_GetDomainName
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDomainName = UPNP_OUT_ARG("NewDomainName"); )
	
	UPNP_HINT( UPNP_STR(out_NewDomainName); )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewDomainName = UPNP_OUT_ARG("NewDomainName");
	
    return statevar_DomainName(service, out_NewDomainName, error_string);

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetAddressRange()
static int action_SetAddressRange
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewMinAddress = UPNP_IN_ARG("NewMinAddress"); )
	UPNP_HINT( UPNP_INPUT *in_NewMaxAddress = UPNP_IN_ARG("NewMaxAddress"); )
	
	UPNP_HINT( UPNP_STR(in_NewMinAddress) )
	UPNP_HINT( UPNP_STR(in_NewMaxAddress) )
	
//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetAddressRange()
static int action_GetAddressRange
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewMinAddress = UPNP_OUT_ARG("NewMinAddress"); )
	UPNP_HINT( UPNP_VALUE *out_NewMaxAddress = UPNP_OUT_ARG("NewMaxAddress"); )
	
	UPNP_HINT( UPNP_STR(out_NewMinAddress); )
	UPNP_HINT( UPNP_STR(out_NewMaxAddress); )
	
//<< USER CODE BEGIN

	UPNP_VALUE *out_NewMinAddress = UPNP_OUT_ARG("NewMinAddress");
	UPNP_VALUE *out_NewMaxAddress = UPNP_OUT_ARG("NewMaxAddress");

    statevar_MinAddress(service, out_NewMinAddress, error_string);
    statevar_MaxAddress(service, out_NewMaxAddress, error_string);

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetReservedAddress()
static int action_SetReservedAddress
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewReservedAddresses = UPNP_IN_ARG("NewReservedAddresses"); )
	
	UPNP_HINT( UPNP_STR(in_NewReservedAddresses) )

//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_DeleteReservedAddress()
static int action_DeleteReservedAddress
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewReservedAddresses = UPNP_IN_ARG("NewReservedAddresses"); )
	
	UPNP_HINT( UPNP_STR(in_NewReservedAddresses) )

//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetReservedAddresses()
static int action_GetReservedAddresses
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewReservedAddresses = UPNP_OUT_ARG("out_NewReservedAddresses"); )
	
	UPNP_HINT( UPNP_STR(out_NewReservedAddresses); )

//<< USER CODE BEGIN

	UPNP_VALUE *out_NewReservedAddresses = UPNP_OUT_ARG("out_NewReservedAddresses");
	if(out_NewReservedAddresses)
		return statevar_ReservedAddresses(service, out_NewReservedAddresses, error_string);
	else 
		return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_SetDNSServer()
static int action_SetDNSServer
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDNSServers = UPNP_IN_ARG("NewDNSServers"); )
	
	UPNP_HINT( UPNP_STR(in_NewDNSServers) )
	
//<< USER CODE BEGIN
	
	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_DeleteDNSServer()
static int action_DeleteDNSServer
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_INPUT *in_NewDNSServers = UPNP_IN_ARG("NewDNSServers"); )
	
	UPNP_HINT( UPNP_STR(in_NewDNSServers) )
	
//<< USER CODE BEGIN

	return OK;

//>> USER CODE END
}
//>> AUTO GENERATED FUNCTION

//<< AUTO GENERATED FUNCTION: action_GetDNSServers()
static int action_GetDNSServers
(
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument,
	char *error_string
)
{
	UPNP_HINT( UPNP_VALUE *out_NewDNSServers = UPNP_OUT_ARG("NewDNSServers"); )
	
	UPNP_HINT( UPNP_STR(out_NewDNSServers); )


//<< USER CODE BEGIN

	UPNP_VALUE *out_NewDNSServers = UPNP_OUT_ARG("NewDNSServers");
	
    return statevar_DNSServers(service, out_NewDNSServers, error_string);

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

//State Variable Table
UPNP_STATE_VAR statevar_x_lanhostconfigmanagement[] =
{
	{"DHCPRelay",                          UPNP_TYPE_BOOL,      &statevar_DHCPRelay,                          0},
	{"DHCPServerConfigurable",             UPNP_TYPE_BOOL,      &statevar_DHCPServerConfigurable,             0},
	{"DNSServers",                         UPNP_TYPE_STR,       &statevar_DNSServers,                         0},
	{"DomainName",                         UPNP_TYPE_STR,       &statevar_DomainName,                         0},
	{"IPRouters",                          UPNP_TYPE_STR,       &statevar_IPRouters,                          0},
	{"MaxAddress",                         UPNP_TYPE_STR,       &statevar_MaxAddress,                         0},
	{"MinAddress",                         UPNP_TYPE_STR,       &statevar_MinAddress,                         0},
	{"ReservedAddresses",                  UPNP_TYPE_STR,       &statevar_ReservedAddresses,                  0},
	{"SubnetMask",                         UPNP_TYPE_STR,       &statevar_SubnetMask,                         0},
	{0,                                    0,                   0,                                            0}
};

//Action Table
static UPNP_ARGUMENT arg_in_SetDHCPServerConfigurable [] =
{
	{"NewDHCPServerConfigurable",     UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_out_GetDHCPServerConfigurable [] =
{
	{"NewDHCPServerConfigurable",     UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_in_SetDHCPRelay [] =
{
	{"NewDHCPRelay",                  UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_out_GetDHCPRelay [] =
{
	{"NewDHCPRelay",                  UPNP_TYPE_BOOL}
};

static UPNP_ARGUMENT arg_in_SetSubnetMask [] =
{
	{"NewSubnetMask",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetSubnetMask [] =
{
	{"NewSubnetMask",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_SetIPRouter [] =
{
	{"NewIPRouters",                  UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_DeleteIPRouter [] =
{
	{"NewIPRouters",                  UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetIPRoutersList [] =
{
	{"NewIPRouters",                  UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_SetDomainName [] =
{
	{"NewDomainName",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetDomainName [] =
{
	{"NewDomainName",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_SetAddressRange [] =
{
	{"NewMinAddress",                 UPNP_TYPE_STR},
	{"NewMaxAddress",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetAddressRange [] =
{
	{"NewMinAddress",                 UPNP_TYPE_STR},
	{"NewMaxAddress",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_SetReservedAddress [] =
{
	{"NewReservedAddresses",          UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_DeleteReservedAddress [] =
{
	{"NewReservedAddresses",          UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetReservedAddresses [] =
{
	{"NewReservedAddresses",          UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_SetDNSServer [] =
{
	{"NewDNSServers",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_in_DeleteDNSServer [] =
{
	{"NewDNSServers",                 UPNP_TYPE_STR}
};

static UPNP_ARGUMENT arg_out_GetDNSServers [] =
{
	{"NewDNSServers",                 UPNP_TYPE_STR}
};

UPNP_ACTION action_x_lanhostconfigmanagement[] =
{
	{"DeleteDNSServer",                         1,       arg_in_DeleteDNSServer,                       0,       0,                                            &action_DeleteDNSServer},
	{"DeleteIPRouter",                          1,       arg_in_DeleteIPRouter,                        0,       0,                                            &action_DeleteIPRouter},
	{"DeleteReservedAddress",                   1,       arg_in_DeleteReservedAddress,                 0,       0,                                            &action_DeleteReservedAddress},
	{"GetAddressRange",                         0,       0,                                            2,       arg_out_GetAddressRange,                      &action_GetAddressRange},
	{"GetDHCPRelay",                            0,       0,                                            1,       arg_out_GetDHCPRelay,                         &action_GetDHCPRelay},
	{"GetDHCPServerConfigurable",               0,       0,                                            1,       arg_out_GetDHCPServerConfigurable,            &action_GetDHCPServerConfigurable},
	{"GetDNSServers",                           0,       0,                                            1,       arg_out_GetDNSServers,                        &action_GetDNSServers},
	{"GetDomainName",                           0,       0,                                            1,       arg_out_GetDomainName,                        &action_GetDomainName},
	{"GetIPRoutersList",                        0,       0,                                            1,       arg_out_GetIPRoutersList,                     &action_GetIPRoutersList},
	{"GetReservedAddresses",                    0,       0,                                            1,       arg_out_GetReservedAddresses,                 &action_GetReservedAddresses},
	{"GetSubnetMask",                           0,       0,                                            1,       arg_out_GetSubnetMask,                        &action_GetSubnetMask},
	{"SetAddressRange",                         2,       arg_in_SetAddressRange,                       0,       0,                                            &action_SetAddressRange},
	{"SetDHCPRelay",                            1,       arg_in_SetDHCPRelay,                          0,       0,                                            &action_SetDHCPRelay},
	{"SetDHCPServerConfigurable",               1,       arg_in_SetDHCPServerConfigurable,             0,       0,                                            &action_SetDHCPServerConfigurable},
	{"SetDNSServer",                            1,       arg_in_SetDNSServer,                          0,       0,                                            &action_SetDNSServer},
	{"SetDomainName",                           1,       arg_in_SetDomainName,                         0,       0,                                            &action_SetDomainName},
	{"SetIPRouter",                             1,       arg_in_SetIPRouter,                           0,       0,                                            &action_SetIPRouter},
	{"SetReservedAddress",                      1,       arg_in_SetReservedAddress,                    0,       0,                                            &action_SetReservedAddress},
	{"SetSubnetMask",                           1,       arg_in_SetSubnetMask,                         0,       0,                                            &action_SetSubnetMask},
	{0,                                         0,       0,                                            0,       0,                                            0}
};
//>> TABLE END

