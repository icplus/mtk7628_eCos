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
 ***************************************************************************

    Module Name:
    ddns_main.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <cfg_def.h>
#include <cfg_id.h>
#include <network.h>
#include <ddns.h>
#include <sys_status.h>
#include <stdlib.h>
#include <stdio.h>

//==============================================================================
//                                    MACROS
//==============================================================================
#define DDNS_PRIORITY  15
#define DDNS_STACKSIZE 8192 //CYGNUM_HAL_STACK_SIZE_TYPICAL

#define	MINUTE_TICKS			(30*100)
#define DDNS_MIN_UPDATE_TIME	(cyg_current_time()+MINUTE_TICKS)

#define DDNS_EVENT_CONFIG		0x01
#define DDNS_EVENT_DOWN			0x02
#define	DDNS_EVENT_UPDATE		0x04
#define DDNS_EVENT_CORRTIME		0x08

#define DDNS_EVENT_ALL      (DDNS_EVENT_CONFIG | DDNS_EVENT_DOWN | DDNS_EVENT_UPDATE | DDNS_EVENT_CORRTIME)

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================
cyg_handle_t ddns_handle ;
cyg_thread ddns_thread ;
unsigned char ddns_stack[DDNS_STACKSIZE];
cyg_flag_t ddns_event_id;

struct user_info *ddns_user_list = NULL;
static int DDNS_running = 0;
//static int DDNS_sleep = 1;

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================
void DDNS_update(void);
void DDNS_init(void);

//==============================================================================
//                              EXTERNAL FUNCTIONS
//==============================================================================
extern struct service_t *find_service(char *);
//extern void oray_set_online_status(int);
extern int str2arglist(char *, char **, char, int);

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
void DDNS_down(void) 
{
	//oray_set_online_status(0);
	if (DDNS_running)
		cyg_flag_setbits(&ddns_event_id, DDNS_EVENT_DOWN); //send msg to DDNS_main
	//oray_set_online_status(0);
}

void DDNS_config(void)
{
	DDNS_init();
	if (DDNS_running)
		cyg_flag_setbits(&ddns_event_id, DDNS_EVENT_CONFIG);
}

void DDNS_update(void)
{
	DDNS_init();
	if (DDNS_running)
		cyg_flag_setbits(&ddns_event_id, DDNS_EVENT_UPDATE);
}

void DDNS_corrtime(void)
{
	if (DDNS_running)
		cyg_flag_setbits(&ddns_event_id, DDNS_EVENT_CORRTIME);
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
void clean_account(struct user_info **info)
{
	struct user_info *acc, *next_acc;
	
	acc = *info;
	while (acc)
	{
		next_acc = acc->next;
		free(acc);
		acc = next_acc;
	}
	
	*info = NULL;
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
void DDNS_add_account(struct user_info *account)
{
	struct user_info *acc = ddns_user_list;
	struct user_info *pre_acc = NULL;
	
	if (!account)
		return;
	//search if account exist in list
	while(acc)
	{
		if (acc->service == account->service && strcmp(acc->host, account->host) == 0)
		{
			if (pre_acc)
				pre_acc->next = acc->next;
			else
				ddns_user_list = acc->next;
			
			free(acc); //remove the old one
			break;
		}
		pre_acc = acc;
		acc = acc->next;
	}
	//Insert or put in top
	account->next = ddns_user_list;
	ddns_user_list = account;
}

//------------------------------------------------------------------------------
// FUNCTION
//		load_config
//
// DESCRIPTION
//	format: [enable];[type];[host_name];[username];[password];[mail_exchanger];[updated_ip];[updated_time];[retry_time]
//  
// PARAMETERS
//
//  
// RETURN
//		0	: success
//		-1:	: fail
//
//------------------------------------------------------------------------------
int load_config(void)
{
	int i;
	struct user_info *account;
	char line[255];
	char *arglists[9];
//	struct user_info *info;
	struct service_t *service;
	
	DDNS_DBG("\n[DDNS]%s",__FUNCTION__);
	i = 1;
	if (CFG_get(CFG_DDNS_SET+i, line) != -1)
	{

	
	DDNS_DBG("\n[DDNS]%s",line);
		if ((str2arglist(line, arglists, ';', 9) == 9) && (atoi(arglists[0]) == 1))
		{
			service = find_service(arglists[1]); //this value may return NULL
			if (service == NULL)
				return -1;//continue;

			account = (struct user_info * )malloc(sizeof(struct user_info));
			if (account == NULL)	
			{
				return -1;	
			}
			bzero(account, sizeof(struct user_info));
			
			account->service = service;
			strcpy(account->host, arglists[2]);
			strcpy(account->usrname, arglists[3]);
			strcpy(account->usrpwd, arglists[4]);
			strcpy(account->mx, arglists[5]);
			account->updated_time = atoi(arglists[7]);
			account->trytime = atoi(arglists[8]);
			account->ddns_ticks = account->trytime * 100;
			account->ip = atoi(arglists[6]);
			switch (account->ip)
			{
			case 0:
				account->status = UPDATERES_ERROR; //need to do update
				break;
				
			//case -1:
			//	account->status = UPDATERES_SHUTDOWN; //should not do update
			//	break;
			
			default:
				account->status = UPDATERES_OK;
				break;
			}
			
			DDNS_add_account(account);
		}
		i++;
	}
	
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
void update_ddns_config(struct user_info *info)
{
	int i;
	char *arglists[9];
	char line[255];
	char newline[255];
	char buf[255];
	
	i = 1;
	while (CFG_get(CFG_DDNS_SET+i, line) != -1)
	{
		strcpy(buf,line);
		if ((str2arglist(line, arglists, ';', 9) == 9) &&
			info->service == find_service(arglists[1]))
		{
			//Write update information to cfg
			sprintf(newline, "%s;%s;%s;%s;%s;%s;%d;%d;%d",
					arglists[0], arglists[1], arglists[2], arglists[3], arglists[4], arglists[5],
					info->ip, info->updated_time, info->trytime);
					
			if (strcmp(buf,newline))		
			{
				CFG_set(CFG_DDNS_SET+i, newline);
				CFG_commit(CFG_NO_EVENT);
			}	
			break;
		}
		i++;
	}
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
void do_update(struct user_info *info)
{

	DDNS_DBG("\n[%s]===>",__FUNCTION__);
	//Call the ddns update process
	info->status = info->service->update_entry(info);	
	switch (info->status)
	{
	case UPDATERES_OK:
    	info->ip = MY_IP;
		
		// Set re-update time
		if (info->service->manual_timeout == 0)
		{
			DDNS_DBG("update %s with IP %s successfully\n", info->service->default_server, MY_IP_STR);
 			info->updated_time = GMTtime(0);
			info->ddns_ticks = info->trytime * 100;
		}
		break;
		
	case UPDATERES_ERROR:
    	DDNS_DBG("failed to update %s with IP %s\n", info->service->default_server, MY_IP_STR);
    	info->ip = 0;
    	info->updated_time = 0;
		info->ddns_ticks = MINUTE_TICKS;		// Try it one minute later
		break;

	case UPDATERES_SHUTDOWN:
	default:
    	DDNS_DBG("failed to update %s with IP %s\n", info->service->default_server, MY_IP_STR);
    	info->ip = -1;
    	info->updated_time = 0;
		info->ddns_ticks = 0;				// Never retry
		break;
    }
	
	update_ddns_config(info);
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
void check_update(void)
{
	struct user_info *info;

	if (MY_IP == 0)
		return;
		
	for (info = ddns_user_list; info; info=info->next)
	{	
		/* UPDATERES_SHUTDOWN mean server has something wrong, 
		   so we should not keep to update */
		if (info->status == UPDATERES_SHUTDOWN)
			continue;
				
		//if (info->ip != MY_IP)
		{
			if (info->service->init_entry)
				(*info->service->init_entry)(info);
			
			do_update(info);
		}
	}
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
void do_corrtime(void)
{
	struct user_info *info;

	if (MY_IP == 0)
		return;

	for (info = ddns_user_list; info; info=info->next)
	{
		if (info->status == UPDATERES_SHUTDOWN)
			continue;

		// Check if time corrected,
		if (info->updated_time != 0 && info->updated_time < 631123200)
		{
			//????????????????????????????????????
			// This is not the time we update it
			// It's the time NTP set correct time
			// TBC
			info->updated_time = GMTtime(0);

			update_ddns_config(info);
		}
	}
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
void do_retrytime(void)
{
	struct user_info *info;


	if (MY_IP == 0)
		return;

	for (info = ddns_user_list; info; info=info->next)
	{
		if (info->ddns_ticks != 0)
		{
			info->ddns_ticks -= MINUTE_TICKS;
			if (info->ddns_ticks <= 0)
			{
				do_update(info);
			}
		}
	}
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
void DDNS_main(cyg_addrword_t data) 
{
	unsigned long event;

	load_config();

	while(1)
	{
		if ((event = cyg_flag_timed_wait(&ddns_event_id,
						DDNS_EVENT_ALL,
						(CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR),
						DDNS_MIN_UPDATE_TIME)) != 0)
		{
			if (event & DDNS_EVENT_CONFIG)
			{
				diag_printf("DDNS_config\n");
				check_update();
			}

			if (event & DDNS_EVENT_UPDATE)
			{
				diag_printf("DDNS_update\n");
				check_update();
			}

			if (event & DDNS_EVENT_DOWN)
			{
				diag_printf("DDNS_down\n");
				break;
			}

			if (event & DDNS_EVENT_CORRTIME)
			{
				diag_printf("DDNS_corrtime\n");
				do_corrtime();
			}
		}
		else
		{
			// Come here every minute
			do_retrytime();
		}	
	}

	clean_account(&ddns_user_list);
	DDNS_running = 0;
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
void DDNS_init(void) 
{
	int en;

	if((CFG_get(CFG_DDNS_EN, &en) != -1)&&(en ==1))
	{
		if(DDNS_running == 0) //initial value is 0
		{
			// Load configuration for the first time
			//load_config();
			
			cyg_flag_init(&ddns_event_id);
			cyg_thread_create(	DDNS_PRIORITY, 
								&DDNS_main, 
								0, 
								"DDNS_daemon",
                     			ddns_stack, 
                     			DDNS_STACKSIZE,
                     			&ddns_handle, 
                     			&ddns_thread);
                     		
			cyg_thread_resume(ddns_handle);	
			DDNS_running = 1;
		}
	}
}

