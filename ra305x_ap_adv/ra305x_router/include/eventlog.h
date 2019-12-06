/*******************************************************************************
* FILE PURPOSE: including file for event logging
*
********************************************************************************
* FILE NAME: eventLog.h
*******************************************************************************/

#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__
#include <cyg/kernel/kapi.h>
#include <str_def.h>

/* event class */
#define EV_CLS_SYS	    0x0001
#define EV_CLS_TCPIP    0x0002
#define EV_CLS_PPPOE    0x0004
#define EV_CLS_FIREWALL	0x0008
#define EV_CLS_OTHER	0x0080

#define EV_CLS_RIP	    0x0100
#define EV_CLS_NTP	    0x0200
#define EV_CLS_DHCP	    0x0400
#define EV_CLS_UPNP		0x0800

#define EV_CLS_RESERVED 0x8000 /* reserved for identifying an empty event entry */

/* event level */
#define	SYS_LOG_EMERG	0	/* system is unusable */
#define	SYS_LOG_ALERT	1	/* action must be taken immediately */
#define	SYS_LOG_CRIT	2	/* critical conditions */
#define	SYS_LOG_ERR	3	/* error conditions */
#define	SYS_LOG_WARNING	4	/* warning conditions */
#define	SYS_LOG_NOTICE	5	/* normal but significant condition */
#define	SYS_LOG_INFO	6	/* informational */
#define	SYS_LOG_DEBUG	7	/* debug-level messages */

#define	INTERNAL_NOPRI	0x10	/* the "no priority" priority */
/* facility codes */
#define	LOG_KERN		(0<<3)		/* kernel messages */
#define	LOG_USER		(1<<3)		/* random user-level messages */
#define	LOG_AUTH		(2<<3)		/* security/authorization messages */
#define	LOG_SYSLOG		(3<<3)		/* messages generated internally by syslogd */
#define	LOG_DROPPK		(4<<3)		/* messages for drop packet */
#define	LOG_ANNOUNCE	(5<<3)		/* messages for announce */
//#define	LOG_DEBUG		(6<<3)		/* messages for debug */
#define LOGM_NULL		(0<<6)
#define LOGM_SYSTEM     (1<<6)
#define	LOGM_SATICIP	(2<<6)
#define	LOGM_DHCPC		(3<<6)
#define	LOGM_PPPOE		(4<<6)
#define	LOGM_PPTP		(5<<6)
#define	LOGM_L2TP		(6<<6)
#define	LOGM_PPPD		(7<<6)
#define	LOGM_DHCPD		(8<<6)
#define	LOGM_RIP		(9<<6)
#define	LOGM_HTTPD		(10<<6)
#define	LOGM_TCPIP		(11<<6)
#define	LOGM_FIREWALL 	(12<<6)
#define	LOGM_NAT		(13<<6)
#define LOGM_NTP		(14<<6)
#define LOGM_DNSMASQ	(15<<6)
#define LOGM_DDNS       (16<<6)
#define LOGM_UPNP       (17<<6)
#define LOGM_LOGD		(19<<6)

#define LOGM_CFG     	(21<<6)
#define LOGM_OS			(22<<6)
#define LOGM_MON		(23<<6)

#define LOGM_IGMP	(25<<6)
#define LOGM_WLAN	(26<<6)


#define	LOG_MAKEPRI(fac, pri)	(((fac) << 3) | (pri))
#define	LOG_NFACILITIES	24	/* current number of facilities */
#define	LOG_FACMASK	0x03f8	/* mask to extract facility part */

#define TIME_BASEDIFF_10        ((((cyg_uint32)10*365) * 24*3600))

				/* facility of pri */
#define	LOG_FAC(p)	(((p) & LOG_FACMASK) >> 3)
#define	INTERNAL_MARK	LOG_MAKEPRI(LOG_NFACILITIES, 0)
typedef struct _code {
	char	*c_name;
	int	c_val;
} CODE;


#ifndef WINDOWS_TYPE
#ifndef __RTMP_ECOS_TYPE_H_
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
#endif
#endif

typedef struct _EventLogEntry {
	UINT32  len;
	UINT16	class;
	UINT16	level;
	UINT32	unix_dateTime;
	char	text[0];
} EventLogEntry;

typedef struct _MessageLogSend {
	UINT16 class;
	UINT16 level;
	UINT16 cmd;
	char printk_buf[512];
	int printed_len;
} MessageLogSend;  

extern int  LogEventNow (UINT16 class, UINT16 level, char *fmtStr, ...);

char *EventClass (EventLogEntry *ev);
char *EventModule(EventLogEntry *ev);
char *EventLevel (EventLogEntry *ev);

#define NUM_LOG_TYPE	2 // system, security
#define SYSTEM_LOG      0
#define SECURE_LOG      1

//void ResetEvent (int type);
char *FindEvent (int type);					/* find the latest event so far   */
char *EndEvent (int type);
char *NextEvent (char *currEvent, int type); /* get next event to be displayed */
char *PrevEvent (char *currEvent, int type);
char *FindMailEvent (int type);					/* find the latest event so far   */

#define LOG_CLASS_MASK ((UINT16)(0x07<<3))
#define LOG_MODULE_MASK (~0x03f)
#define LOG_LEVEL_MASK  0x07

//-------------------------------------------------------------------------------------
// Syslog Ussage:
// Syslog(flag, meassge) 
// flag:  3bit:syslog/securitylog,3bit:debug-levle,10bit:module
// example->LOG_USER|LOG_INFO|LOGM_PPPOE
// syslog/securitylog-> LOG_USER:syslog,LOG_AUTH:securitylog
// debug-levle-> LOG_DEBUG:console,LOG_NOTICE:log pool,LOG_INFO:both console & log pool
// module-> refer to LOGM
// message: string
//----------------------------------------------------------------------------------------

#define SysLog(a,c...) LogEventNow((((UINT16)a)&(~LOG_LEVEL_MASK)),(((UINT16)a)&((UINT16)0x07)), ##c)

void event_send_mail(void);
void eventlog_update(void);
void clear_system_log(void);
void clear_seurity_log(void);

struct eventlog_cfg
{
	char sysName[256];
	char sysDomain[256];
	char sysID[16];
	int logmode;
	int doRemoteLog;
	int doRemoteMail;
	char remote_ip[256];
	char Local_Server_IP[256];
	char MailTo[0][256];
};

#endif /* __EVENTLOG_H__ */


