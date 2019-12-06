#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <pkgconf/system.h>
#include <network.h>
#include <arpa/inet.h>
#include <config.h>
#include <eventlog.h>
#include <smtp.h>
#include <cfg_def.h>
#include <time.h>

#define EVENT_ENTRY_PER_BLK         64
#define TOTAL_EVENT_ENTRY           128

#define EVENT_LOG_REPORTING_LOCAL   0x80
#define EVENT_LOG_REPORTING_SYSLOG  0x40
#define EVENT_LOG_REPORTING_TRAP    0x20

#define EVENT_LOG_TASK_STACK_SIZE   10000
#define EVENT_LOG_TASK_PRIORITY     180
#define EVENT_LOG_TASK_SLICE        0

#define SYSLOGD_CMD_SEND            0x00000001
#define SYSLOGD_CMD_RESET           0x00000002


extern char log_buf_area[];
extern int log_buf_len;
extern int log_pool_num;
extern int log_count_max;
extern int log_mail_thresh;
extern int log_mail_addr_num;

struct log_cb
{
	char *buf;
	char *start;
	char *end;
	char *bottom;
	int num;
	
	char *mail_start;
	int num_to_mail;
};

struct log_cb event_cb[NUM_LOG_TYPE];


CODE facility_names[] =
{
	{ "kern",	LOG_KERN },
	{ "user",	LOG_USER },
	{ "auth",	LOG_AUTH },
	{ "syslog",	LOG_SYSLOG },
	{ "",		-1 }
};

CODE log_module_name[] =
{
	{ STR_SYS,	LOGM_SYSTEM },
	{ STR_SIP,	LOGM_SATICIP },
	{ STR_DHCPC,LOGM_DHCPC },
	{ STR_POE,	LOGM_PPPOE },
	{ STR_PTP,	LOGM_PPTP },
	{ STR_L2TP,	LOGM_L2TP },
	{ STR_PPP,	LOGM_PPPD },		/* INTERNAL */
	{ STR_DHCPD,LOGM_DHCPD },
	{ STR_RIP,	LOGM_RIP },
	{ STR_HTP,	LOGM_HTTPD },
	{ STR_TCP,	LOGM_TCPIP },
	{ STR_FIRE,	LOGM_FIREWALL },
	{ STR_NAT,	LOGM_NAT },
	{ STR_NTP,	LOGM_NTP },
	{ STR_DMQ,	LOGM_DNSMASQ },
	{ STR_DDNS,	LOGM_DDNS },
	{ STR_UPNP,	LOGM_UPNP },
	{ STR_LOG,	LOGM_LOGD}, 
	{ STR_CFG,	LOGM_CFG}, 
	{ STR_OS,	LOGM_OS},	
	{ STR_MON,	LOGM_MON},
	{ STR_IGMP, LOGM_IGMP},
	{ STR_WLAN, LOGM_WLAN},
	{ "",		-1 }
};

CODE prioritynames[] =
{
    { "alert",	SYS_LOG_ALERT },
    { "crit",	SYS_LOG_CRIT },
    { "debug",	SYS_LOG_DEBUG },
    { "emerg",	SYS_LOG_EMERG },
    { "err",	SYS_LOG_ERR },
    { "error",	SYS_LOG_ERR },		/* DEPRECATED */
    { "info",	SYS_LOG_INFO },
    { "none",	INTERNAL_NOPRI },		/* INTERNAL */
    { "notice",	SYS_LOG_NOTICE },
    { "panic",	SYS_LOG_EMERG },		/* DEPRECATED */
    { "warn",	SYS_LOG_WARNING },		/* DEPRECATED */
    { "warning",SYS_LOG_WARNING },
    { "",		-1 }
};

#define _string(s) #s
#define	STR(s)	_string(s)
  		   
static int syslogd_initialized = 0;
static struct sockaddr_in remoteaddr;

char *EventModuleDesc (UINT16 module);
char *EventModule (EventLogEntry *ev);


//*************************************
// Porting declaration
//*************************************
struct eventlog_cfg	*evcfg;
static int evcfg_len;

extern void EventLog_CfgRead(struct eventlog_cfg *cfg);
void EventLog_CfgUpdate(void);


/* udp socket for logging to remote host */
static int remotefd = -1;
static int RemotePort = 514;

int  init_RemoteLog (void);


/* Lock for log message access */
static cyg_mutex_t event_mutex;
static cyg_mbox syslogd_tsk_mbox;
static cyg_handle_t syslogd_tsk_mbox_h;

void syslog_mutex_lock(void);
void syslog_mutex_unlock(void);


//*************************************
// Function start
//*************************************
static void ResetEvent(int type)
{
	struct log_cb *lc;
	
	if (type >= log_pool_num)
		return;
	
	lc = &event_cb[type];
	
	lc->buf = log_buf_area + log_buf_len * type;
	
	lc->start = lc->buf;
	lc->end = lc->buf;
	lc->bottom = lc->buf + log_buf_len;
	lc->num = 0;
	
	lc->mail_start = lc->buf;
	lc->num_to_mail = 0;
	
	// Initialize log buffer
	memset(lc->buf, 0xff, log_buf_len);
	return;
}

void ResetMailEvent(int type)
{
	struct log_cb *lc;
	
	if (type >= log_pool_num)
		return;
	
	lc = &event_cb[type];
	lc->mail_start = lc->end;
	lc->num_to_mail = 0;
}

int LogEventNow (UINT16 class, UINT16 level, char *fmtStr, ...)
{
	va_list args;
	
	MessageLogSend *log_msg;
	static int keep_lasttime = 0;
	static int logcnt = 0;
	
	
	if (syslogd_initialized == 0)
   	   return -1;
   
	log_msg = (MessageLogSend *)malloc(sizeof(MessageLogSend));
	if(!log_msg)
		return -1;
	
	va_start (args, fmtStr);
	
	log_msg->printed_len = vsnprintf(log_msg->printk_buf,
									sizeof(log_msg->printk_buf),
									fmtStr,
									args);
	if (log_msg->printed_len <=0) 
		{
		free(log_msg);//free malloc memory before return;
		return -1;	//error check	
		}
	// Simon added to remove the '\n'
	while ( log_msg->printed_len &&
			log_msg->printk_buf[log_msg->printed_len-1] == '\n')
	{
		log_msg->printed_len--;
		log_msg->printk_buf[log_msg->printed_len] = 0;
	}
									
	va_end (args);
	
	if (level == SYS_LOG_DEBUG) //only debug print
	{
		diag_printf("[%08d]%s %s\n", 
					(UINT32)cyg_current_time(),
					EventModuleDesc((UINT16)(class) & LOG_MODULE_MASK),
					log_msg->printk_buf);
					
		free(log_msg);
		return 0;
	}
	
	if (level == SYS_LOG_INFO) //Debug print and save to LOG file
	{
		diag_printf("[%08d]%s %s\n",
					(UINT32)cyg_current_time(),
					EventModuleDesc((UINT16)(class) & LOG_MODULE_MASK),
					log_msg->printk_buf);
	}
	
	//logmode=>bit0:system,bit1:debug,bit2:Hicker,bit3:TX packet,bit4:anounce.
	if ((class & LOG_CLASS_MASK) == LOG_USER)
	{
		if ((evcfg->logmode & 0x01) == 0)
		{
			free(log_msg);
			return -1;
		}
	}
	
	if ((class & LOG_CLASS_MASK) == LOG_AUTH)
	{
		if ((evcfg->logmode & 0x04) == 0)
		{
			free(log_msg);
			return -1;
		}
	}
	
	if ((class & LOG_CLASS_MASK) == LOG_DROPPK)
	{	
		if ((evcfg->logmode & 0x08) == 0)
		{
			free(log_msg);
			return -1;
		}
	}

	if (class == LOGM_DHCPD)
	{
		if ((evcfg->logmode & 0x16) == 0)
		{
			free(log_msg);
			return -1;
		}
	}
	if (time(0) <= keep_lasttime)
	{
		if(logcnt >= 10)
		{
			free(log_msg);
			return 0;
		}	
	}
	else
	{
		logcnt = 0;
		keep_lasttime = time(0)+1;
	}	
	logcnt++;
	
	if (log_pool_num == 1)
	{
		class = (class & ~LOG_CLASS_MASK) | LOG_USER;
	}
	else
	{
		if ((class & LOG_CLASS_MASK) != LOG_USER && 
			(class & LOG_CLASS_MASK) != LOG_AUTH)
		{
			class = (class & ~LOG_CLASS_MASK) | LOG_USER;
		}
	}
	
	log_msg->class = class;
	log_msg->level = level;
	log_msg->cmd = 0;
	if (!cyg_mbox_tryput(syslogd_tsk_mbox_h, log_msg))
	{
		free(log_msg);
		return -1;
	}
	
	return 0;
}

char *EventClassDesc(UINT16 class)
{
	CODE *c_fac;
	
	class = (class & LOG_CLASS_MASK);
	for (c_fac = facility_names;
		c_fac->c_name[0] != 0 && c_fac->c_val != class;
		c_fac++)
	{
		;
	}
	
	return c_fac->c_name;
}

char *EventModuleDesc(UINT16 module)
{
	CODE *c_modu;
	
	module = (module & LOG_MODULE_MASK);
	for (c_modu = log_module_name;
		c_modu->c_name[0] != 0 && c_modu->c_val != module;
		c_modu++)
	{
		;
	}
	
	return c_modu->c_name;
}

char *EventLevelDesc(UINT16 level)
{
	CODE *c_pri;
	
	level = (level & LOG_LEVEL_MASK);
	for (c_pri = prioritynames;
		c_pri->c_name[0] != 0 && c_pri->c_val != level;
		c_pri++)
	{
		;
	}
	
	return c_pri->c_name;
}

static struct log_cb *GetEventCB(int type)
{
	if (syslogd_initialized == 0)
   	   return 0;
	
	if (type >= log_pool_num)
		return 0;
	
	return &event_cb[type];
}

char *FindMailEvent(int type)
{
	struct log_cb *lc = GetEventCB(type);
	
	if (lc == 0 || lc->num_to_mail == 0)
		return 0;
	
	return lc->mail_start;
}

char *FindEvent(int type)
{    
	struct log_cb *lc = GetEventCB(type);
	
	if (lc == 0 || lc->num == 0)
		return 0;
	
	return lc->start;
}

char *NextEvent(char *currEvent, int type)
{
	struct log_cb *lc = GetEventCB(type);
	EventLogEntry *ev;
	char *nextEvent;
	
	if (lc == 0)
		return 0;
	
	ev = (EventLogEntry *)currEvent;
	
	nextEvent = currEvent + ev->len;
	if (nextEvent == lc->bottom)
		nextEvent = lc->buf;
	
	if (nextEvent == lc->end)
		return 0;
	
	return nextEvent;
}

char *EndEvent(int type)
{    
	struct log_cb *lc = GetEventCB(type);
	
	if (lc == 0 || lc->num == 0)
		return 0;
	
	if (lc->end == lc->buf)
		return lc->bottom;
	
	return lc->end;
}

char *PrevEvent(char *currEvent, int type)
{
	struct log_cb *lc = GetEventCB(type);
	
	if (lc == 0)
		return 0;
	
	if (currEvent == lc->start)
		return 0;
	
	if (currEvent == lc->buf)
		currEvent = lc->bottom;
	
	return (currEvent - *(int *)(currEvent - 4));
}

char *EventClass(EventLogEntry *ev)
{
	return EventClassDesc(ev->class & LOG_CLASS_MASK);
}

char *EventModule(EventLogEntry *ev)
{
	return EventModuleDesc(ev->class & LOG_MODULE_MASK);
}

char *EventLevel(EventLogEntry *ev)
{
	return EventLevelDesc(ev->level);
}

static void syslogd_fn(cyg_addrword_t data)
{
	MessageLogSend *msq_ptr;
	
	struct log_cb *lc;
	EventLogEntry *ev;
	
	int type;
	UINT16 class;
	UINT16 level;
	UINT32 len;
	
	while(1)
	{
		msq_ptr = (MessageLogSend *)cyg_mbox_get(syslogd_tsk_mbox_h);
		if (msq_ptr == NULL)
			continue;
		
		syslog_mutex_lock();
		
		switch (msq_ptr->cmd)
		{	
		case 0:
			// Setup type
			class = msq_ptr->class;
			level = msq_ptr->level;
			len = (msq_ptr->printed_len + 1) + sizeof(*ev);
			len = ((len + 3) & ~3) + 4;		// Simon 2006/04/12, hid length at end.
			
			type = SYSTEM_LOG;
			if (level == SYS_LOG_NOTICE || level == SYS_LOG_INFO)
			{
				if ((class & LOG_CLASS_MASK) == LOG_USER)
					 type = SYSTEM_LOG;
				
				if ((class & LOG_CLASS_MASK) == LOG_AUTH)
				{
					if (log_pool_num == 1)
						type = SYSTEM_LOG;
					else
						type = SECURE_LOG;
				}
			}
			
			lc = &event_cb[type];
			
			//
			// Take care end of buffer problem.
			//
			if ((lc->end + len) > (lc->buf + log_buf_len))
			{
				// Do log pointer
				if (lc->end <= lc->start && lc->num != 0)
				{
					while (lc->start < lc->bottom)
					{
						lc->start += ((EventLogEntry *)lc->start)->len;
						lc->num--;
					}
					
					lc->start = lc->buf;
				}
				
				// Do email log pointer
				if (lc->end <= lc->mail_start && lc->num_to_mail != 0)
				{
					if (evcfg->doRemoteMail)
					{
						SysLog(LOG_USER|SYS_LOG_INFO|LOGM_LOGD, "Log Buffer Full, send mail.");
						
						EventSendMail(type);
						ResetMailEvent(type);
					}
					else
					{
						while (lc->mail_start < lc->bottom)
						{
							lc->mail_start += ((EventLogEntry *)lc->mail_start)->len;
							lc->num_to_mail--;
						}
						
						lc->mail_start = lc->buf;
					}
				}
				
				// Move log_end to being
				lc->bottom = lc->end;
				lc->end = lc->buf;
			}
			
			// Take care of end is head of start problem.
			if (lc->end <= lc->start && 
				(lc->end + len) > lc->start &&
				lc->num != 0)
			{
				while ((lc->end + len) > lc->start)
				{
					// Give up some old entries to have space to put new
					lc->start += ((EventLogEntry *)lc->start)->len;
					lc->num--;
					
					if (lc->start == lc->bottom)
					{
						lc->start = lc->buf;
						break;
					}
				}
			}
			
			// Send log if exceeded
			if (lc->end <= lc->mail_start && 
				(lc->end + len) > lc->mail_start && 
				lc->num_to_mail != 0)
			{
				if (evcfg->doRemoteMail)
				{
					SysLog(LOG_USER|SYS_LOG_INFO|LOGM_LOGD, "Log Buffer Full, send mail.");
					
					EventSendMail(type);
					ResetMailEvent(type);
				}
				else
				{
					while ((lc->end + len) > lc->mail_start)
					{
						lc->mail_start += ((EventLogEntry *)lc->mail_start)->len;
						lc->num_to_mail--;
						
						if (lc->mail_start == lc->bottom)
						{
							lc->mail_start = lc->buf;
							break;
						}
					}
				}
			}
			
			// Fill the new entry
			ev = (EventLogEntry *)lc->end;
			ev->len = len;
			ev->class = class;
			ev->level = level;
			ev->unix_dateTime = time(0);
			strcpy(ev->text, msq_ptr->printk_buf);
			
			// Advance to next
			lc->end += len;
			*(int *)(lc->end - 4) = len;	// Simon, 2006/04/12, put length info
			
			if (lc->end > lc->bottom)
				lc->bottom = lc->end;
			
			lc->num++;
			lc->num_to_mail++;
			
			// Check if log max reached
			if (lc->num > log_count_max)
			{
				lc->start += ((EventLogEntry *)lc->start)->len;
				if (lc->start == lc->bottom)
					lc->start = lc->buf;
				
				lc->num--;
			}
			
			//
			// Do remote log
			//
			if (evcfg->doRemoteLog)  //Unix event log
			{
				if (remotefd >= 0 || init_RemoteLog() > 0)
				{
					UINT32 ev_time;
					int diff;
					char log_pkt[1024];
					int pri;
					char timeinfo[32];
					
					pri = (ev->class & LOG_CLASS_MASK) | (ev->level);
					log_pkt[0] = 0;
					diff = ntp_time() - time(0);
					ev_time = gmt_translate2localtime(diff + ev->unix_dateTime);
					ctime_r(&ev_time, timeinfo);
					timeinfo[19] = 0;
					sprintf(log_pkt, "<%d>%s %s %s: %s", pri, &timeinfo[4], evcfg->sysID, EventModule(ev), ev->text);
					if (write(remotefd, &log_pkt, strlen(log_pkt)) < 0)                   
					{
						diag_printf("syslogd_fn:write error\n");
					}
				}
			}
			
			//
			// Do mail log
			//
			if (evcfg->doRemoteMail) //send by mail
			{
				if (lc->num_to_mail >= log_mail_thresh)
				{
					SysLog(LOG_USER|SYS_LOG_INFO|LOGM_LOGD, "Log mail threshold reached, send mail.");
					
					EventSendMail(type);
					ResetMailEvent(type);
				}
			}
			break;
			
		case 1:
			EventSendMail(-1);
			break;
			
		case 2:
			EventLog_CfgUpdate();
			break;
			
		case 3:
			ResetEvent(SYSTEM_LOG);
			break;
		
		case 4:
			ResetEvent(SECURE_LOG);
			break;
			
		default:
			break;
		}
		
		syslog_mutex_unlock();
		free(msq_ptr);
	}
}


#define	CYGNUM_SYSLOGD_STACK_SIZE	4096
static char syslogd_stack[CYGNUM_SYSLOGD_STACK_SIZE];
static cyg_thread syslogd_thread_data;
static cyg_handle_t syslogd_handle;

/* Start the SYSLOG Daemon */
void eventlog_start(void)
{
	/* Only initialize things once */
	if (syslogd_initialized)
	   return;
	
	syslogd_initialized = 1;
	
	cyg_mbox_create( &syslogd_tsk_mbox_h, &syslogd_tsk_mbox );
	cyg_thread_create(CYGPKG_NET_THREAD_PRIORITY+1, 
						syslogd_fn,               // entry
						0,                     // entry parameter
						"SYSLOG Daemon",         // Name
						&syslogd_stack,           // stack
						sizeof(syslogd_stack),    // stack size
						&syslogd_handle,          // Handle
						&syslogd_thread_data);    // Thread data structure
						
	cyg_thread_resume(syslogd_handle);
}

//
// Function to be changed
//
int init_RemoteLog (void)
{
	int len = sizeof(remoteaddr);
	struct hostent *hostinfo;
	static struct in_addr logserv_addr = {0};
	
	if (Check_IP(evcfg->remote_ip) < 0)  /* not a IP */
	{
		hostinfo = gethostbyname(evcfg->remote_ip);
		if (hostinfo && (hostinfo->h_length == 4))
		{
			sprintf(evcfg->remote_ip, inet_ntoa(*(struct in_addr *)hostinfo->h_addr));
		}
		else
		{
			return -1;
		}
	}
	
	if (inet_addr(evcfg->remote_ip) == 0) /* IP equal zero */
		return -1;	
	
	remotefd = socket(AF_INET, SOCK_DGRAM, 0);
	if (remotefd < 0)
	{
		diag_printf("syslogd: cannot create socket\n");
		return -1;
	}
	
	memset(&remoteaddr, 0, len);
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(evcfg->remote_ip);
	remoteaddr.sin_len = sizeof(struct sockaddr);
	remoteaddr.sin_port = htons(RemotePort);
	
	/* 
	 Since we are using UDP sockets, connect just sets the default host and port 
	 for future operations
	*/
	if (0 != (connect(remotefd, (struct sockaddr *) &remoteaddr, len)))
	{
		diag_printf("syslogd: cannot connect to remote host\n");
		
		close(remotefd);
		remotefd = -1;
	}
	
	return remotefd;
}

void EventLog_CfgUpdate(void)
{
	memset(evcfg, 0, evcfg_len);
	EventLog_CfgRead(evcfg);
	
	// Close remote socket anyway
	if (remotefd >= 0)
	{
		close(remotefd);
		remotefd = -1;
	}
	
	if (evcfg->doRemoteLog)
	{
		init_RemoteLog();
	}
	return ;
}

int InitEvent (void)
{
    int i;
	
	evcfg_len = sizeof(*evcfg) + sizeof(evcfg->MailTo[0])*log_mail_addr_num;
	
	evcfg = (struct eventlog_cfg *)malloc(evcfg_len);
	if (evcfg == NULL)
		return -1;
	
	/* init mutext */
	cyg_mutex_init(&event_mutex);
	
	for(i=0; i<NUM_LOG_TYPE; i++)
		ResetEvent(i);
	
	EventLog_CfgUpdate();
	
	eventlog_start();
	return 0;
}

//######################################
// Functions for external calling in
//######################################
void syslog_mutex_lock(void)
{
	cyg_mutex_lock(&event_mutex);
}

void syslog_mutex_unlock(void)
{
	cyg_mutex_unlock(&event_mutex);
}

void eventlog_cmd(int cmd)
{
	MessageLogSend *log_msg;
	
	if (syslogd_initialized == 0)
	   return;
	
	log_msg = (MessageLogSend *)malloc(sizeof(MessageLogSend));
	if (!log_msg)
		return;
	
	log_msg->cmd = cmd;
	if (!cyg_mbox_tryput(syslogd_tsk_mbox_h, log_msg))
	{
		free(log_msg);
	}
	
	return;
}

void event_send_mail(void)
{
	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_LOGD," Send one mail.");
	eventlog_cmd(1);
}
void eventlog_update(void)
{
	eventlog_cmd(2);
}
void clear_system_log(void)
{
	SysLog(LOG_USER|SYS_LOG_NOTICE|LOGM_LOGD,"Clear the syslog");
	eventlog_cmd(3);
}
void clear_seurity_log(void)
{
	SysLog(LOG_AUTH|SYS_LOG_NOTICE|LOGM_LOGD,"Clear the secure log");
	eventlog_cmd(4);
}


