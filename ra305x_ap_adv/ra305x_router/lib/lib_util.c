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

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <time.h>
#include <network.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <cfg_net.h>
#include <cfg_def.h>
#include <netinet/in_var.h>
#include <cfg_net.h>
#include <sys_status.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define __NEW_UTS_LEN 64

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================
struct new_utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
static struct new_utsname  system_utsname;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
void lib_mac_to_str(char * buf, char * mac)
{
    int i;
    for (i=0; i<5; i++)
        sprintf(buf+i*2, "%02X:", mac[i]&0xff );
    sprintf(buf+10, "%02X", mac[5]&0xff );
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int gethostname(char *name, int len)
{
	int i;
	char sysName[256];
	
#if 0	
	i = 1 + strlen(system_utsname.nodename);
	if (i > len)
		i = len;
		
	memcpy(name, system_utsname.nodename, i);
#else
	CFG_get_str(CFG_SYS_NAME,sysName);
	i = 1 + strlen(sysName);
	if (i > len)
		i = len;
	memcpy(name, sysName, i);	
#endif	
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int sethostname(const char *name, int len)
{
	if (len < 0 || len > __NEW_UTS_LEN)
		return -1;
		
	memcpy(system_utsname.nodename, name, len);
	system_utsname.nodename[len] = 0;
	
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int getosname(char *name)
{
	strcpy(name, system_utsname.sysname);
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int getosrelease(char *rel)
{
	strcpy(rel, system_utsname.release);
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int getmachine(char *mach)
{
	strcpy(mach, system_utsname.machine);
	return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int getosversion(char *ver)
{
	strcpy(ver, system_utsname.version);
	return 0;
}


#ifndef HAVE_STRNSTR
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
char *strnstr(const char *haystack, const char *needle, int len)
{
	int needle_len;
	
	if (*haystack == 0) {
		if (*needle != 0)
			return NULL;
		return (char *)haystack;
	}
	if (*needle == 0)
		return (char *)haystack;

	needle_len = strlen(needle);
	if (len <needle_len)
		return NULL;

	while (*haystack!=0 && len >= needle_len) {
		int i;
		for (i=0; i<=needle_len; i++) {
			if (i==needle_len)
				return (char *)haystack;
			if (haystack[i] != needle[i])
				break;
		}
		haystack++;
		len --;
	}
	
	return NULL;
}
#endif



//------------------------------------------------------------------------------
// FUNCTION
//		int str2arglist(char *buf, int *list, char c, int max)
//
// DESCRIPTION
//		convert string to argument array, ',' character was regarded as delimiter
//	and '\n' character regarded as end of string. 
//  
// PARAMETERS
//		buf: input string
//		list: output array pointer
//		c: delimiter
//		max: maximum number of sub-string
//  
// RETURN
//		number of arguments
//  
//------------------------------------------------------------------------------
int str2arglist(char *buf, int *list, char c, int max)
{
	char *idx = buf;
	int j=0;
	
	list[j++] = buf;
	while(*idx && j<max) {
		if(*idx == c || *idx == '\n') {
			*idx = 0;
			list[j++] = idx+1;
		}
		idx++;	
	}
	if(j==1 && !(*buf)) // No args
		j = 0;
		
	return j;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
char * time2string(int t)
{
    time_t now=(time_t)t;
static char time_info[32];
    ctime_r(&now, time_info);
    time_info[strlen(time_info)-1] = '\0';  // Strip \n
    return time_info;
}

/*
char * ver2string(int ver)
{
static char str[20];
    sprintf(str, "%d.%d.%d.%d", (ver>>24), (ver>>16)&0xff, (ver>>8)&0xff, ver&0xff );
    return str;
}
*/
/**************************************************************************************************
**                                                                                               **
**  FUNCTION                                                                                     **
**                                                                                               **
**      Check_IP                                                                                 **
**                                                                                               **
**************************************************************************************************/
int Check_IP(char* str)
{
	int   i;
	int   ip[4]={0};
	char  tmp[16];

	if (!str)
		return -1;

	if (sscanf(str, "%d.%d.%d.%d%16s", &ip[0], &ip[1], &ip[2], &ip[3], tmp) != 4)
		return -1;

	for (i = 0; i <= 3; i++)
	{
		if (ip[i] < 0 || ip[i] > 255)
			return -1;
	}

	//if (ip[3] <=0 || ip[3] >=255) return -1;
	
	return 1;
}
/**************************************************************************************************
**                                                                                               **
**  FUNCTION                                                                                     **
**                                                                                               **
**      Check_MASK                                                                                **
**                                                                                               **
**************************************************************************************************/
int Check_MASK(char* str)
{
	int   i,j;
	int   ip[4] = {-1};
	char  tmp[16];
	int   flag = 0;

	if (!str)
		return 0;

	if (sscanf(str, "%d.%d.%d.%d%15s", &ip[0], &ip[1], &ip[2], &ip[3], tmp) != 4)
		return 0;

	for (i = 0; i <4; i++)
	{
		if (ip[3-i] < 0 || ip[3-i] > 255)
			return -1;
		for(j= 0;j<4;j++)
		{
			if((ip[3-i]>>j)&0x01)
			{
			     flag++;
			}
			else
			{
			  if(flag)
			    return -1;    
			}     
		}		
	}

	return 1;
}
//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
#if 0
int fast_deleteTcb(unsigned short port)
{
	extern struct       inpcbtable tcbtable;
    int    netLock;         /* Use to secure the Network Code Access */
   	struct inpcb *inp;		/* Pointer to BSD PCB list */
	struct tcpcb *tp;

	// Let it has time to close
	cyg_thread_delay(1);
	//
	// Kill TCP control block
	//
   	netLock = splnet ();        /* Get exclusive access to Network Code */

	inp = tcbtable.inpt_queue.cqh_first;
    if (inp)
    {
        while (inp != (struct inpcb *)&tcbtable.inpt_queue)
        {
            if (inp->inp_laddr.s_addr !=  0 &&
                inp->inp_lport == htons(port))
            {
                tp = intotcpcb(inp);
                // Must higher than LAST_WAIT
                if (TCPS_HAVERCVDFIN(tp->t_state) == 1)
                {
                    struct inpcb *temp = inp->inp_queue.cqe_prev;
                    tcp_close(tp);

                    inp = temp;
                    continue;
                }
            }

            inp = inp->inp_queue.cqe_next;
        }
    }
	splx (netLock); 	/* Give up exclusive access to Network Code */
    return 0;
}
#endif

#if defined(CONFIG_NAT) || defined(CONFIG_FW)
//------------------------------------------------------------------------------
// FUNCTION
//		check_localport
//
// DESCRIPTION
//		search inet PCB to check whether port has been opened
//  
// PARAMETERS
//		dst : destination IP
//		port : port need to check
//  	prot : protocol(IPPROTO_TCP/IPPROTO_UDP)
//
// RETURN
//		1 : port has been opened
//		0 : otherwise
//  
//------------------------------------------------------------------------------
int check_localport(unsigned int dst, unsigned short port, unsigned char prot)
{


#if 1 //porting by ziqiang   2013-04-08
//#if 0 //Eddy6 todo
	extern struct inpcbhead tcb;
	extern struct inpcbhead udb;
   	struct inpcb *inp, *head;	/* Pointer to BSD PCB list */

	
	inp =0;
	head = 0;
	switch(prot)
	{
		case IPPROTO_TCP:
			inp = tcb.lh_first;
			head = (struct inpcb *)&(tcb.lh_first);
			break;
		case IPPROTO_UDP:
			inp = udb.lh_first;
			head = (struct inpcb *)&(udb.lh_first);
			break;
		default:
			return 0;
	}

    if (inp)
    {
        while (inp != head)
        {
            if ((inp->inp_lport == port)&&
            	(inp->inp_laddr.s_addr == 0 || inp->inp_laddr.s_addr == dst))
            {
                return 1;
            }
	inp = inp->inp_list.le_next;
	if (inp == NULL )
		break;

        }
    }
#endif
    return 0;
	
}

int check_localport2(unsigned int src, unsigned int sport, unsigned int dst, unsigned short port, unsigned char prot)
{


#if 1 //porting by ziqiang   2013-04-08
//#if 0 //Eddy6 todo

	extern struct inpcbhead tcb;
	extern struct inpcbhead udb;
   	struct inpcb *inp, *head;	/* Pointer to BSD PCB list */
	
	inp = 0;
	head = 0;
	switch(prot)
	{
		case IPPROTO_TCP:
			inp = tcb.lh_first;
			head = (struct inpcb *)&(tcb.lh_first);
			break;
		case IPPROTO_UDP:
			inp = udb.lh_first;
			head = (struct inpcb *)&(udb.lh_first);
			break;
		default:
			return 0;
	}

	if (inp)
	{
		while (inp != head)
		{
			
			if ((inp->inp_lport == port) &&
			    (inp->inp_laddr.s_addr == 0 || inp->inp_laddr.s_addr == dst))
			{
				if (prot == IPPROTO_TCP)
				{
					if ((inp->inp_fport == sport) &&
					    (inp->inp_faddr.s_addr == 0 || inp->inp_faddr.s_addr == src))
					{
						return 1;
					}
				}
				else					
					return 1;
			}
			inp = inp->inp_list.le_next;
			if (inp == NULL )
				break;
		}
	}
#endif
return 0;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int check_localaddr(struct ifnet *ifp, unsigned int addr)
{
	register struct ifaddr *ifa;
	
	ifa = TAILQ_FIRST(&ifp->if_addrlist);
    while (ifa) {
    	if (ifa->ifa_addr->sa_family == AF_INET)
    	{
    		struct in_ifaddr *ia;
    		
    		ia = (struct in_ifaddr *)ifa;
		if(ia->ia_addr.sin_addr.s_addr == addr)
       			return 1;
       	}
   		ifa = ifa->ifa_list.tqe_next;
    }
    
	return 0;
}
#endif /* CONFIG_NAT || CONFIG_FW */

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
inline time_t GMTtime(int flag)
{
	return ntp_getlocaltime(0);
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
inline time_t GMTtime_offset(int flag)
{
	return ntp_getlocaltime_offset();
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int check_dst_zone(int daylight, char *dst_time, struct tm *tb)
{
    int smon, sday, emon, eday;
    int dst;

    // If no daylight saving, exit
    if (daylight == 0)
        return false;

    // Initialize boundary
    smon = dst_time[0];
    sday = dst_time[1];
    emon = dst_time[2];
    eday = dst_time[3];


    // default to no daylight saving
    dst = false;    
    if (smon < emon)
    {
        // Northern Part of the planet
        if ((tb->tm_mon >  smon-1 && tb->tm_mon  <  emon-1) ||
            (tb->tm_mon == smon-1 && tb->tm_mday >= sday) ||
            (tb->tm_mon == emon-1 && tb->tm_mday <  eday))
        {
            dst = true;
        }
    }
    else if(smon > emon)
    {
        // Southern Part of the planet
        if ((tb->tm_mon >  smon-1 || tb->tm_mon < emon-1) ||
            (tb->tm_mon == smon-1 && tb->tm_mday >= sday) ||
            (tb->tm_mon == emon-1 && tb->tm_mday <  eday))
        {
            dst = true;
        }
    }
    else
    {
    	if(sday > eday)
    	{
    		if(tb->tm_mday < sday && tb->tm_mday  > eday)
    			dst = false;
    		else
    			dst = true;
    	}	
        if (tb->tm_mon == smon-1 && tb->tm_mon == emon-1)
        {
        	if(tb->tm_mday >= sday && tb->tm_mday <= eday)
        		dst = true;
        }		
    }	

    return dst;
}

//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
#define SECONDSPERMINUTE (cyg_uint32)60
#define SECONDSPERHOUR   (cyg_uint32)(SECONDSPERMINUTE * 60)
#define SECONDSPERDAY    (cyg_uint32)(SECONDSPERHOUR * 24)
#define SECONDSPERYEAR   (cyg_uint32)(SECONDSPERDAY * 365)
static char *timezone[]={
	"-12","-11","-10","-9","-8","-7","-7","-6","-6","-6","-5","-5","-5",
	"-4","-4","-4","-3","-3","-3","-2","-1","0","0","0","+1","+1","+1","+1","+1","+1","+2",
	"+2","+2","+2","+2","+2","+3","+3","+3","+4","+4","+5","+6","+7","+8","+8","+9","+10",
	"+10","+10","+10","+11","+12","+12","+13","-3.5","+3.5","+4.5","+5.5","+5.75","+9.5","+10.5"};

extern int config_ntp_save_time;

time_t
gmt_translate2localtime(time_t gmt_time)
{
  struct tm tm={ 0,0,0,0,0,0,0,0,0 };
  int time_zone;
  int t_zone , daylight, dst, tmp_info;
  char dst_time[4];
  int ntp_type;
  
  
   //t_zone = 45; // Taiwan +8
   
   CFG_get( CFG_SYS_TZONE, &t_zone);
   CFG_get( CFG_SYS_DAYLITE, &daylight);
   
   CFG_get( CFG_SYS_DAYLITE_SM, &tmp_info);
   dst_time[0] = (char)(tmp_info);
   CFG_get( CFG_SYS_DAYLITE_SD, &tmp_info);
   dst_time[1] = (char)(tmp_info);
   CFG_get( CFG_SYS_DAYLITE_EM, &tmp_info);
   dst_time[2] = (char)(tmp_info);
   CFG_get( CFG_SYS_DAYLITE_ED, &tmp_info);
   dst_time[3] = (char)(tmp_info);
   
   ntp_type = 0;
   if(CFG_get(CFG_SYS_NTPTYPE, &ntp_type)!= -1)
   {
       if(!Is_ntp_ever_sync() || get_non_ntp_flag()) 
    		t_zone =  22; //if not ntp mode, need no correct time
   }
   
   if(config_ntp_save_time == 0 && gmt_time < 20*365*24*60*60)
   {
       t_zone = 22;
       daylight = 0;
   }
			
   if(timezone[t_zone][0]=='+')
   {
       time_zone = atoi(&timezone[t_zone][1]);
       gmt_time+= time_zone*3600;    
   }
   else if(timezone[t_zone][0]=='-')
   {
       time_zone = atoi(&timezone[t_zone][1]);
       gmt_time-= time_zone*3600;
   }	    
	
   localtime_r(&gmt_time,&tm);
   dst = check_dst_zone(daylight, dst_time, &tm);
   gmt_time = gmt_time + 60 * (dst? 1:0)*60;
       
   return gmt_time;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int read_interface(char *interface, u_int32_t *addr, u_int32_t *mask, unsigned char *arp)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, interface);

		if (addr) 
		{ 
			if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) 
			{
				sin = (struct sockaddr_in *) &ifr.ifr_addr;
				*addr = sin->sin_addr.s_addr;
			} else 
				goto err_out;
		}
		
		if (mask) 
		{ 
			if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) 
			{
				sin = (struct sockaddr_in *) &ifr.ifr_addr;
				*mask = sin->sin_addr.s_addr;
			} else 
				goto err_out;
		}
		
		if(arp) 
		{
			if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) 
				memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
			else 
				goto err_out;
		}
	} else {
		diag_printf("socket failed!");
		return -1;
	}
	close(fd);
	return 0;

err_out:
	if (fd >= 0)
		close(fd);
	return -1;
}


//------------------------------------------------------------------------------
// FUNCTION
//
//
// DESCRIPTION
//
//  
// PARAMETERS
//
//  
// RETURN
//
//  
//------------------------------------------------------------------------------
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    cyg_tick_count_t ctick;
    
    ctick = cyg_current_time();
    tv->tv_usec = (ctick%100)*10000;
    tv->tv_sec = time(NULL);
    return(0);
}


unsigned int nstr2ip(char *hn)
{
	struct hostent *hostinfo;
	
	if(Check_IP(hn)<0)  /* not a IP */
    {
    	hostinfo = gethostbyname(hn);
    	if(hostinfo&&(hostinfo->h_length == 4))
    	{
    		return *(unsigned int *)(hostinfo->h_addr);
    	}		
    	else
    	{
    		return 0;
    	}	
	}
	else
		return (unsigned int)(inet_addr(hn));
}


int net_ping_test(unsigned int dst, int seq, int ping_time)
{
	unsigned int time;

	if((dst == htonl(0x7f000001)) || (dst == SYS_lan_ip)||((SYS_wan_up == CONNECTED)&&(dst == SYS_wan_ip)))
		return 0;

	time=cyg_current_time();
	
	if (icmp_ping( htonl(dst), seq, ping_time)==0)
	{
		return (cyg_current_time()- time);
	}	
	else
	{
		return -1;
	}	
}

char *strdup(const char *src)
{
    int len;
    char *dst;

    len = strlen(src) + 1;
    if ((dst = (char *)malloc(len)) == NULL)
	return(NULL);
    strcpy(dst, src);
    return(dst);
}


