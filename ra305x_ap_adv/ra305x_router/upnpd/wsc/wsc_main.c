/***************************************
	wsc main function
****************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <upnp_type.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <sys/ioctl.h> 
#include <upnp.h>	

#include "wsc_common.h"
#include "wsc_msg.h"
#include "wsc_upnp.h"
#include "wsc_netlink.h"

int ioctl_sock = -1;
unsigned char UUID[16]= {0};

static int wscUPnPDevInit = FALSE;
static int wscK2UMInit = FALSE;
static int wscU2KMInit = FALSE;
static int wscMsgQInit = FALSE;

#ifdef SECOND_WIFI
	#define WSC_DEVICE_NUM 2
#else
	#define WSC_DEVICE_NUM 2
#endif


int WscUPnPOpMode;
unsigned char HostMacAddr[WSC_DEVICE_NUM][MAC_ADDR_LEN]={0};			// Used to save the MAC address of local host(ra0 or rai0)..
char WSC_IOCTL_IF[IFNAMSIZ];
extern cyg_handle_t	wsc_msg_monitor_tid;
extern cyg_handle_t	wscNLEventHandle_tid;

int wscSystemInit(void)
{
	return wscMsgSubSystemInit();
}


int wscSystemStop(void)
{
	/* Stop the K2U and U2K msg subsystem */
	if (wscU2KMInit && ioctl_sock >= 0) 
	{
		// Close the ioctl socket.
		close(ioctl_sock);
		ioctl_sock = -1;
	}

	stopNLThread = 1;
	/* delete the message queue */
	wps_cmdq_delete(wps_msqid);
	wps_msqid = -1;
	
	if (wscK2UMInit)
		wscK2UModuleExit();

	if(wscUPnPDevInit)
		WscUPnPDevStop();

	if(wscMsgQInit)
		wscMsgSubSystemStop();
	cyg_thread_kill(wsc_msg_monitor_tid);
	cyg_thread_kill(wscNLEventHandle_tid);
	return 1;
}


/******************************************************************************
 * main of WPS module
 *
 * Description: 
 *       Main entry point for WSC device application.
 *       Initializes and registers with the sdk.
 *       Initializes the state stables of the service.
 *       Starts the command loop.
 *
 * Parameters:
 *    int argc  - count of arguments
 *    char ** argv -arguments. The application 
 *                  accepts the following optional arguments:
 *
 *          -ip 	ipAddress
 *          -port 	port
 *		    -desc 	descDoc
 *	        -webdir webRootDir"
 *		    -help 
 *          -i      ioctl binding interface.
 *
 *****************************************************************************/
int WPSInit(void)
{
	char *infName = NULL;
	int retVal;

	/* default op mode is device */
	WscUPnPOpMode = WSC_UPNP_OPMODE_DEV;

	/* default op mode is device */
	memset(&WSC_IOCTL_IF[0], 0, IFNAMSIZ);
	strcpy(&WSC_IOCTL_IF[0], "ra0");//sht:???,default interface is ra0(2.4G)
	
	/* System paramters initialization */
	if (wscSystemInit() == WSC_SYS_ERROR)
	{
	        DBGPRINTF(RT_DBG_OFF, "wsc MsgQ System Initialization failed\n");
		goto STOP;
	}
	wscMsgQInit = TRUE;	
	
	/* Initialize the ioctl interface for data path to kernel space */
	ioctl_sock = wscU2KModuleInit();
	if(ioctl_sock == -1)
	{
	        DBGPRINTF(RT_DBG_OFF, "creat ioctl socket failed!\n");
		goto STOP;
	}
	else
	{
		DBGPRINTF(RT_DBG_INFO, "Create ioctl socket(%d) success!\n", ioctl_sock);
		wscU2KMInit = TRUE;
	}

	/* Initialize the upnp related data structure and start upnp service */
	if(WscUPnPOpMode)
	{
		struct ifreq  ifr;

		// Get the Mac Address of wireless interface ra0:2.4G
		memset(&ifr, 0, sizeof(struct ifreq));
		strcpy(ifr.ifr_name, WSC_IOCTL_IF); //sht:set default interface is ra0 after init
		if (ioctl(ioctl_sock, SIOCGIFHWADDR, &ifr) > 0) 
			goto STOP;

		memcpy(HostMacAddr[0], ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);

		diag_printf("%s:HW-Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", ifr.ifr_name,HostMacAddr[0][0], HostMacAddr[0][1], 
		HostMacAddr[0][2], HostMacAddr[0][3], HostMacAddr[0][4], HostMacAddr[0][5]);

		DBGPRINTF(RT_DBG_INFO, "%s:HW-Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", ifr.ifr_name,HostMacAddr[0][0], HostMacAddr[0][1], 
				HostMacAddr[0][2], HostMacAddr[0][3], HostMacAddr[0][4], HostMacAddr[0][5]);

#ifdef SECOND_WIFI// 0//mytest SECOND_WIFI
		// Get the Mac Address of wireless interface rai0:5G
		memset(&ifr, 0, sizeof(struct ifreq));
		strcpy(ifr.ifr_name, "rai0"); //rai0:5G
		if (ioctl(ioctl_sock, SIOCGIFHWADDR, &ifr) > 0) 
			goto STOP;

		memcpy(HostMacAddr[1], ifr.ifr_hwaddr.sa_data, MAC_ADDR_LEN);
		
		diag_printf("%s:HW-Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", ifr.ifr_name,HostMacAddr[1][0], HostMacAddr[1][1], 
				HostMacAddr[1][2], HostMacAddr[1][3], HostMacAddr[1][4], HostMacAddr[1][5]);

		DBGPRINTF(RT_DBG_INFO, "%s:HW-Addr: %02x:%02x:%02x:%02x:%02x:%02x\n", ifr.ifr_name,HostMacAddr[1][0], HostMacAddr[1][1], 
				HostMacAddr[1][2], HostMacAddr[1][3], HostMacAddr[1][4], HostMacAddr[1][5]);

#endif
		// Start UPnP Device Service.
		if (WscUPnPOpMode & WSC_UPNP_OPMODE_DEV)
		{	  
            retVal = WscUPnPDevStart();

			if (retVal != WSC_SYS_SUCCESS)
				goto STOP;

			wscUPnPDevInit = TRUE;
		}
	}
	
	/* Initialize the netlink interface from kernel space *///sht:kernel space means wifi driver  
	if(wscK2UModuleInit() != WSC_SYS_SUCCESS)
	{	
	        DBGPRINTF(RT_DBG_OFF, "creat netlink socket failed\n");
		goto STOP;
	}
	else 
	{
		DBGPRINTF(RT_DBG_INFO, "Create netlink socket success!\n");
		wscK2UMInit = TRUE;
	}
	
	return 1;	

STOP:
	return 0;
}

