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


#ifndef __UPNP_TYPE_H__
#define __UPNP_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <porting.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <device_config.h>


/*
 * Definitions
 */
#if !defined(OK) || !defined(ERROR)
#define OK     0
#define ERROR -1
#endif

#if !defined(TRUE) || !defined(FALSE)
#define TRUE   1
#define FALSE  0
#endif

enum UPNP_DATA_TYPE_E
{ 
	UPNP_TYPE_STR = 3,
	UPNP_TYPE_BOOL,
	UPNP_TYPE_UI1,
	UPNP_TYPE_UI2,
	UPNP_TYPE_UI4,
	UPNP_TYPE_I1,
	UPNP_TYPE_I2,
#ifdef CONFIG_WPS
	UPNP_TYPE_I4,
	UPNP_TYPE_BIN_BASE64
#else
	UPNP_TYPE_I4
#endif /* CONFIG_WPS */
};

//
// Input argument defintions
//
typedef union
{
	char            i1;
	short           i2;
	long            i4;
	unsigned char   ui1;
	unsigned short  ui2;
	unsigned long   ui4;
	unsigned int	bool;
	char            *str;
#ifdef CONFIG_WPS
	unsigned char   *bin_base64;
#endif /* CONFIG_WPS */	
}
UPNP_INPUT;

typedef struct in_argument
{
	struct in_argument *next;
	
	char		*name;
	int			type;
	UPNP_INPUT	value;
}
IN_ARGUMENT;

//
// Output argument defintions
//
typedef union
{
    char			i1;
    short			i2;
    long			i4;
    unsigned char	ui1;
    unsigned short	ui2;
    unsigned long	ui4;
	unsigned int	bool;
    char			str[2048/* 512 */];
}
UPNP_VALUE;

typedef struct out_argument
{
	struct out_argument *next;
	
	char		*name;
	int			type;
	UPNP_VALUE	value;
}
OUT_ARGUMENT;

/*
 * UPnP interface definitions
 */
#define IFF_IPCHANGED		0x01    /* interface IP address changed */
#define IFF_MJOINED			0x02    /* SSDP multicast group joined */

typedef struct upnp_interface
{
	char				ifname[IFNAMSIZ];   /* interface name */
	int					flag;               /* ip changed, multicast joined? */
	int					http_sock;          /* upnp_http socket */
	unsigned long		ipaddr;				/* Host mode ip */
	unsigned long		netmask;			/* Host mode mask */
	unsigned char		hwaddr[6];
	struct upnp_interface	*next;			/* pointer to next if */
}
UPNP_INTERFACE;

/*
 * UPnP context definitions
 */
#define MAX_HEADERS			32				/* default message header num */
#define MAX_HEADER_LEN		4096
#define MAX_BUF_LEN			2048

#ifndef	UPNP_IF_NUM
#define	UPNP_IF_NUM			1
#endif

typedef struct
{
	/* Configuration */
	unsigned int    http_port;			/* upnp_http port number */
	unsigned int    adv_time;			/* ssdp advertising time interval */
	unsigned int    sub_time;			/* gena subscription time interval */
	unsigned int    last_check_adv_time;		/* Record the last check time */
	unsigned int    last_check_gena_time;		/* Record the last check time */
	
	UPNP_INTERFACE	upnp_if[UPNP_IF_NUM];
	
	/* Runtime */
	UPNP_INTERFACE	*iflist;
	int	http_sock;
	int	ssdp_sock;
	int	udp_sock;
                
	/* Status */
        int method;                         /* M_GET, M_POST, ... */
        int method_id;                      /* method index */
        int fd;                             /* client socket descriptor */
        int status;                         /* R_OK, R_ERROR, R_BAD_REQUEST... */
	
        char buf[MAX_HEADER_LEN];           /* upnp_http input buffer */
        char *msghdrs[MAX_HEADERS];			/* delimited headers */
        int header_num;                     /* num of headers */
    
        char head_buffer[MAX_HEADER_LEN];	/* output header buffer */
        char body_buffer[MAX_BUF_LEN];      /* output body buffer */
	
        int index;                          /* index to next unread char */
        int end;                            /* index to one char past last */
	
        char *url;
        char *content;
        int  content_len;
    
	char *CONTENT_LENGTH;
	char *SOAPACTION;
	char *SID;
	char *CALLBACK;
	char *TIMEOUT;
	char *NT;
	char *HOST;
	char *MAN;
	char *ST;
	
	IN_ARGUMENT	*in_arguments;
	OUT_ARGUMENT	*out_arguments;
	
	IN_ARGUMENT	in_args[UPNP_MAX_IN_ARG];
	OUT_ARGUMENT	out_args[UPNP_MAX_OUT_ARG];
	
        UPNP_INTERFACE *focus_ifp;			/* for SSDP MSEARCH */
	struct sockaddr_in *dst;			/* client address, for SSDP MSEARCH */
	
	struct sockaddr_in dst_addr;
	
	/* description variables */
	int	xml_stage;
}
UPNP_CONTEXT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UPNP_TYPE_H__ */


