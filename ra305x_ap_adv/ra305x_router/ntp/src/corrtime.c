#include <config.h>
#include <pkgconf/system.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "eventlog.h"
#include "tz_info.h"
#include "time_servers.h"
#include <cfg_id.h>
#include <cfg_def.h>
#include <network.h>
#include <cfg_net.h>
#include <sys_status.h>

#ifdef NTP


#define RETRY_SHORT     60          /* one-minute */
//#define RETRY_LONG      60*60       /* one-hour */
#define RETRY_LONG      60*60*24*7  /* one-week */
#define NTP_THREAD_PRIORITY 15


static int tz_index, dls, smon, sday, emon, eday;
static int sync_ok=0;
static time_t NextTimeUpdate = 0;
static int ntp_initialized = 0;
int ntp_GMTtime_offset = 0;
int ntp_zonetime_offset = 0;
int ntp_type = 0;  //0:NTP,otherwise not NTP
static int non_ntp_flag=0;
char ntp_server[256];
static int real_wan_up=0;
static int ntp_ever_sync =0;
static int tmp_zonetime=0;

extern int config_ntp_save_time;

extern int ntpc_synchonize(char *server);
extern int gettimeofday(struct timeval *tv,  struct  timezone *tz);
extern time_t ntp_getlocaltime(int flag);
int ntp_remove_tzone(int val);
int Is_ntp_ever_sync(void);



int corrtime(void)
{
    struct timezone tz;
    struct timeval tv;
    int status;
    int server_num = sizeof(time_servers)/sizeof(char *);
    int server_index;
    int retry_count = 0;
    int tmp_update;

    server_index = ((unsigned int)arc4random())%server_num;
    
    while (retry_count < 3)
    {
        status = -1;
        if(ntp_server[0] == 0)  //yfchou added: specified the NTP server
        	status = ntpc_synchonize(time_servers[server_index]);
        else
        	status = ntpc_synchonize(ntp_server);

    	if (status != 0)
    	{
            //syslog(SYS_LOG_ERR, "Cannot syncronize with time server %s, status=%d\n", time_servers[server_index], status);

            server_index = (server_index+1)%server_num;
            retry_count++;
            continue;
    	}
    	else
        {
       	    if(ntp_server[0] == 0)  //yfchou added: specified the NTP server
        		SysLog(LOG_USER|SYS_LOG_INFO|LOGM_NTP, STR_Sync_with " %s", time_servers[server_index]);
        	else
       	    	SysLog(LOG_USER|SYS_LOG_INFO|LOGM_NTP, STR_Sync_with " %s", ntp_server);
            sync_ok = 1;			  //ntp sync	
            non_ntp_flag = 0;		  //Non ntp? 	
            ntp_ever_sync = 1;		  // the ntp have ever syn?	
            SYS_time_correct = 1;     //system watch
            if (config_ntp_save_time)
	    {
                tmp_update = ntp_getlocaltime(0)-time(0);
                CFG_set_no_event(CFG_SYS_TIME,&tmp_update);
                CFG_commit(0);
	    }
            mon_snd_cmd(MON_CMD_NTP_UPDATE);
            break;
        }
    }
    
    if (retry_count == 3)
    {
        sync_ok = 0;
        SYS_time_correct = 0;
        return -1;
    }
    
    /* Get time */
    gettimeofday(&tv, &tz);
    ntp_zonetime_offset = tv.tv_sec-time(NULL);
    
    return 0;
}


static void ntp_fn(cyg_addrword_t data)
{
    time_t current_time;

    srand(time(NULL));
    CFG_get(CFG_SYS_NTPTYPE, &ntp_type);
    if(ntp_type) // NON NTP
    	non_ntp_flag=1;
    if (config_ntp_save_time)
    {
        CFG_get(CFG_SYS_TIME, &ntp_GMTtime_offset);
        ntp_getlocaltime(0); //get the tmp_zonetime
        if(CFG_get(CFG_SYS_NTPTYPE, &ntp_type) == -1) // May save with TZONE
            ntp_GMTtime_offset = ntp_remove_tzone(ntp_GMTtime_offset);
        //eventlog_time_correct();
    }
    ntp_server[0] = 0;
    CFG_get(CFG_SYS_NTPSRV, ntp_server);

    /* run forever unless shutdown */
    while (1)
    {
		if((SYS_wan_up==DISCONNECTED)||!real_wan_up)
  		{	
  			if((SYS_wan_up==DISCONNECTED))
  				real_wan_up = 0;
  			NextTimeUpdate = 0;	
  			cyg_thread_delay(100);
  			continue;
		}
		CFG_get(CFG_SYS_NTPTYPE, &ntp_type);
		if(ntp_type) //NON NTP
		{
			NextTimeUpdate = 0;
			cyg_thread_delay(100);
  			continue;
		}
		else //NTP mode
		{	
        	if (NextTimeUpdate == 0)
        	{
            	if (corrtime() == 0)
            	{
                	NextTimeUpdate = RETRY_LONG + time(NULL);
            	}
            	else
            	{
                	NextTimeUpdate = RETRY_SHORT + time(NULL);
            	}
        	}
        	else
        	{
        		current_time = time(NULL);
       			if (current_time > NextTimeUpdate)
					NextTimeUpdate = 0;
    		}	
        	cyg_thread_delay(50);
    	}
    }	
}


static char ntp_stack[4096*2];
static cyg_thread ntp_thread_data;
static cyg_handle_t ntp_handle;


/* Start the NTP server */
void cyg_ntp_start(void) {
//  struct timeval tv;
      
  /* Only initialize things once */
  if (ntp_initialized)
      return;
  ntp_initialized = 1;
  
  cyg_thread_create(NTP_THREAD_PRIORITY, 
		    ntp_fn,               // entry
		    0,                     // entry parameter
		    "NTP Client",         // Name
		    &ntp_stack,           // stack
		    sizeof(ntp_stack),    // stack size
		    &ntp_handle,          // Handle
		    &ntp_thread_data);    // Thread data structure

  cyg_thread_resume(ntp_handle);
}


time_t ntp_time(void)
{
	return (ntp_GMTtime_offset + time(NULL));
}


time_t ntp_getlocaltime_offset(void)
{
	return 	ntp_GMTtime_offset;
}

	
time_t ntp_getlocaltime(int flag)
{
	time_t now;
	char time_info[32];
	struct timezone tz;
	struct timeval tv;
	struct tm local;
	int tz_offset;
	
	/* Get time */
	memset(time_info,0,sizeof(time_info));
    gettimeofday(&tv, &tz);
    
	CFG_get( CFG_SYS_TZONE, &tz_index);

	CFG_get( CFG_SYS_DAYLITE, &dls);    
	
	CFG_get( CFG_SYS_DAYLITE_SM, &smon);
	
	CFG_get( CFG_SYS_DAYLITE_SD, &sday);
	
	CFG_get( CFG_SYS_DAYLITE_EM, &emon);
	
	CFG_get( CFG_SYS_DAYLITE_ED, &eday);
	
    if (dls && (smon == 0 || sday == 0 || emon == 0 || eday == 0))
        dls = 0;
    
    //diag_printf("----------Is_ntp_ever_sync=%d,non_ntp_flag=%d\n",Is_ntp_ever_sync(),non_ntp_flag);
    if(CFG_get(CFG_SYS_NTPTYPE, &ntp_type)!=-1)
    {
    	if(Is_ntp_ever_sync() && !non_ntp_flag) //NTP mode
    		tz_offset = time_zones[tz_index].tz_offset;
    	else  //In non ntp mode, need no time zone and daylight saving
    	{
    		tz_offset=0;
    		dls = 0;	
    	}
    }
    else
    {
        if (config_ntp_save_time)
	{
    	    tz_offset = time_zones[tz_index].tz_offset;
	}
	else
        {
            if(Is_ntp_ever_sync()) //NTP mode
                tz_offset = time_zones[tz_index].tz_offset;
    	    else  //In non ntp mode, need no time zone and daylight saving
    	    {
    		tz_offset=0;
    		dls = 0;	
    	    }
	}
    }			
    tz.tz_minuteswest = tz_offset;

    if (dls)
    {
    	if(smon==16)
    	{
    		dls = 1;  //in dlink mode, don't judge the date interval..
    	}	
    	else
    	{
        	now = (time_t)ntp_time()+tz_offset;
        	//setenv("TZ", time_zones[tz_index].tz_str, 1);   /* XXX -- do we need to? */
        	localtime_r(&now, &local);
        	/* Disable daylight saving before daylight saving time is validated */
        	dls = 0;
        	if (smon < emon)
        	{
                /* Northern Sphere */
            	if ((local.tm_mon > smon-1 && local.tm_mon < emon-1) ||
                	(local.tm_mon == smon-1 && local.tm_mday >= sday) ||
                	(local.tm_mon == emon-1 && local.tm_mday <= eday))
            	{
                	dls = 1;
            	}
        	}
        	else if(smon > emon)
        	{
            	/* Southern Sphere */
            	if ((local.tm_mon > smon-1 || local.tm_mon < emon-1) ||
                	(local.tm_mon == smon-1 && local.tm_mday >= sday) ||
                	(local.tm_mon == emon-1 && local.tm_mday < eday))
            	{
                	dls = 1;
            	}
        	}
        	else /*smon = emon*/
        	{
        		if(sday > eday)
        		{
        			if(local.tm_mday > eday && local.tm_mday < sday)
        				dls = 0;
        			else
        				dls = 1;
        		}	
        		if (local.tm_mon == smon-1 && local.tm_mon == emon-1)
            	        {
        			if(local.tm_mday >= sday && local.tm_mday <= eday)
                	            dls = 1;
            	        }
        	}
        	//unsetenv("TZ");
        }	
    }
    tmp_zonetime = (tz_offset + 60*(dls?1:0))*60;
    now = ntp_time() + tmp_zonetime;
    ctime_r(&now, time_info);
	if (strlen(time_info)==0)return 0;
    time_info[strlen(time_info)-1] = '\0';  // Strip \n
    if(flag)  //printf the time
       diag_printf("%s:ntp_type=%d,tz_offset=%d,daylight=%d,SYS_time_correct=%d",time_info,ntp_type,tz_offset,dls,SYS_time_correct);
    return now;   
}


void ntp_update(int flag)
{
	static int non_ntp_cnt = 0;
	
	CFG_get(CFG_SYS_NTPTYPE, &ntp_type);
	
	if(!flag) //wan-up
	{
		real_wan_up = 1;
		if(ntp_type != 0 && !non_ntp_cnt) //System restart, do once if in non NTP mode.
		{
			non_ntp_cnt++;
			mon_snd_cmd(MON_CMD_NTP_UPDATE);
		}		
	}
	else
	{	
		diag_printf("ntp_update!\n");
		if(ntp_type == 0)
			NextTimeUpdate = 0;
		ntp_type = 0;
		sync_ok=0;
		if(ntp_type == 0)
			SYS_time_correct = 0;
		else
			SYS_time_correct = 1;	
		ntp_server[0]=0;
		CFG_get(CFG_SYS_NTPSRV, ntp_server);
	}	
}


void non_ntp_update(int computer_time)
{
	ntp_GMTtime_offset = computer_time-time(NULL);
	if (config_ntp_save_time)
	{
		CFG_set_no_event(CFG_SYS_TIME,&ntp_GMTtime_offset);
		CFG_commit(0);
	}
	non_ntp_flag = 1;
	mon_snd_cmd(MON_CMD_NTP_UPDATE);
}


int get_ntp_type(void)
{
	int ntptype = 0;
	
	CFG_get(CFG_SYS_NTPTYPE, &ntptype);
	return ntptype;
}


int get_ntp_sync(void)
{
	return sync_ok;
}


int get_non_ntp_flag(void)
{
	return non_ntp_flag;
}


int Is_ntp_ever_sync(void)
{
	return ntp_ever_sync;
}


int ntp_remove_tzone(int val)
{
	time_t now;
	
    if(val > tmp_zonetime)
  		now = val - tmp_zonetime;
  	else
  		now = 0;	
    return now;   
}		
#endif


