/*
	header file for wsc_ioctl

*/
#ifndef __WSC_IOCTL_H__
#define __WSC_IOCTL_H__

#include <sys/ioctl.h>

// Define Linux ioctl relative structure, keep only necessary things
struct iw_point
{
	void *pointer;
	unsigned short length;
	unsigned short flags;
};
	
union iwreq_data
{
	struct iw_point data;
};

struct iwreq {
 
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;

	/* Data part (defined just above) */
	union   iwreq_data      u;

};


#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE                          0x8BE0
#endif
//#define SIOCIWFIRSTPRIV                         0x00
#define SIOCIWFIRSTPRIV							0x8BE0


#define RT_PRIV_IOCTL                           (SIOCIWFIRSTPRIV + 0x01)

#define OID_GET_SET_TOGGLE                      0x8000

#define RT_OID_WSC_UUID                         0x0753
#define RT_OID_WSC_SET_SELECTED_REGISTRAR       0x0754
#define RT_OID_WSC_EAPMSG                       0x0755

#endif

