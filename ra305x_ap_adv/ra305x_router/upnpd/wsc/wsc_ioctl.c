/*
	function handler for ioctl handler
*/
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include "wsc_common.h"
#include "wsc_ioctl.h"

extern int rt28xx_ap_ioctl(
        PNET_DEV pEndDev, 
        int cmd, 
	caddr_t	pData);

/******************************************************************************
 * wsc_set_oid
 *
 * Description: 
 *       Send Wsc Message to kernel space by ioctl function call.
 *
 * Parameters:
 *    uint16 oid - The ioctl flag type for the ioctl function call.
 *    char *data - The data message want to send by ioctl
 *    int len    - The length of data 
 *  
 * Return Value:
 *    Indicate if the ioctl success or failure.
 *    	-1 = failure.
 *    	0  = If success. 
 *****************************************************************************/
int wsc_set_oid(uint16 oid, char *data, int len)
{
	int status;
	char *buf;
	struct iwreq iwr;
        char name[IFNAMSIZ];
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;
	
	if((buf = malloc(len)) == NULL)
		return WSC_SYS_ERROR;

	memset(buf, 0, len);
	memset(&iwr, 0, sizeof(iwr));
	strncpy(name, WSC_IOCTL_IF, IFNAMSIZ);
	iwr.u.data.flags = oid;
	iwr.u.data.flags |= OID_GET_SET_TOGGLE;

	if (data)
		memcpy(buf, data, len);

	iwr.u.data.pointer = (caddr_t)buf;
	iwr.u.data.length = len;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0)
		{
			found = 1;
			break;
	    }
	}

	if (found == 1)
#ifdef CONFIG_AP_SUPPORT		
		status = rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT		
		status = rt28xx_sta_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif
	else
		status = 1;

	if (status != 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "%s: oid=0x%x len (%d) failed", __FUNCTION__, oid, len);
		free(buf);
		return WSC_SYS_ERROR;
	}
	free(buf);
	return WSC_SYS_SUCCESS;
}


/******************************************************************************
 * wsc_get_oid
 *
 * Description: 
 *       Get Wsc Message from kernel space by ioctl function call.
 *
 * Parameters:
 *    uint16 oid - The ioctl flag type for the ioctl function call.
 *    char *data - The data pointer want to receive msg from ioctl.
 *    int len    - The length of returned data.
 *  
 * Return Value:
 *    Indicate if the ioctl success or failure.
 *    	-1 = failure.
 *    	0  = If success. 
 *****************************************************************************/
int wsc_get_oid(uint16 oid, char *data, int *len)
{
	int status;
	struct iwreq iwr;
        char name[IFNAMSIZ];
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;

	if (data == NULL)
		return WSC_SYS_ERROR;	

	memset(&iwr, 0, sizeof(iwr));
	strncpy(name, WSC_IOCTL_IF, IFNAMSIZ);
	iwr.u.data.flags = oid;
	iwr.u.data.pointer = (caddr_t)data;

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0)
		{
			found = 1;
			break;
	    }
	}

	if (found == 1)
	{
#ifdef CONFIG_AP_SUPPORT	
		status = rt28xx_ap_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif
#ifdef CONFIG_STA_SUPPORT	
		status = rt28xx_sta_ioctl(sc, RT_PRIV_IOCTL, (caddr_t)&iwr);
#endif

		*len = iwr.u.data.length;
	}
	else
		status = 1;

	if (status != 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "%s: oid=0x%x len (%d) failed", __FUNCTION__, oid, *len);
		return WSC_SYS_ERROR;
	}

	return WSC_SYS_SUCCESS;
}


/******************************************************************************
 * wscU2KModuleInit
 *
 * Description: 
 *       Initialize the U2K Message subsystem(i.e. it's ioctl sub-system now).
 *
 * Parameters:
 *    void
 *  
 * Return Value:
 *    The created ioctl socket id or -1 to indicate if the initialize success or failure.
 *    	-1 		  		 = failure.
 *    	ioctl socket id  = If success. 
 *****************************************************************************/
int wscU2KModuleInit(void)
{
	int s;
	struct ifreq ifr;
	
	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "ioctl socket create failed! errno=%d!\n", errno);
		return -1;
	}
#if 0
	/* do it */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, WSC_IOCTL_IF, IFNAMSIZ);//sht:set default interface is ra0 after init

	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "ioctl to get the interface(%s) index failed! errno=%d!\n" , ifr.ifr_name, errno);
		return -1;
	}
	//diag_printf();
#ifdef SECOND_WIFI //0 //mytest SECOND_WIFI

	/* do it */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "rai0");//for the second wifi interface:rai0:5G

	if (ioctl(s, SIOCGIFINDEX, &ifr) < 0)
	{
		DBGPRINTF(RT_DBG_ERROR, "ioctl to get the interface(%s) index failed! errno=%d!\n" , ifr.ifr_name, errno);
		return -1;
	}
	
#endif
#endif
	return s;

}


