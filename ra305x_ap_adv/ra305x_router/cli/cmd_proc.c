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
    cmd_proc.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <pkgconf/io.h>
#include <pkgconf/io_serial.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <cyg/io/devtab.h>
#include <cyg/io/io.h>
#include <cli_cmd.h>
#include <stdio.h>
#include <cyg/io/ttyio.h>
#include <sys/select.h>
#include <sys/sockio.h>
#include <fcntl.h>
#include <network.h>
#include <arpa/inet.h>
#include <cfg_def.h>
#ifdef CONFIG_USBHOST
#include "../../drivers/usbhost/include/mu_hfi_if.h"
#include "../../drivers/usbhost/include/musb_if.h"
#endif

static char cur_cmd_buf[CLI_MAX_CMD_LEN];

#define CMD_HISTORY // enable command recording function

#ifdef CMD_HISTORY
#define MAX_CMD_HISTORY		10
static char cmd_history[MAX_CMD_HISTORY][CLI_MAX_CMD_LEN];
static int cmd_index=0;
static struct timeval select_timeout;
#define DEFAULT_TIMEOUT_VALUE   3600

#define CMD_DEV			"/dev/ser0"
static int  ser0;
fd_set      fd_rd;

#ifdef MODULE_TELNET
#define TELNET_IAC          255
#define TELNET_DONT         254
#define TELNET_DO           253
#define TELNET_WONT         252
#define TELNET_WILL         251
#define TELNET_SB           250
#define TELNET_SE           240
#define TELNET_ECHO         1
#define TELNET_SUPGOAHEAD   3
        
static int  fd_telnet;
static int  fd_client;
struct sockaddr_in sa_telnet;
struct sockaddr_in sa_client;
socklen_t sa_client_len = sizeof(sa_client);

#define TELNET_WITHOUT_AUTH     0
#define TELNET_IN_AUTH			1
#define TELNET_PASS_AUTH		2
#define CFG_SYS_USERS			0x010B0200
#define CFG_SYS_ADMPASS         0x01010200

#define FALSE 0
#define TRUE 1

static int haveAuth = TELNET_WITHOUT_AUTH;
static int isEACOMode = FALSE;
static char admps[256];
static char telnet_CR_LF[2]={'\r', '\n'};
static char *userName = "Login as:";
static char *userNameError = "Invalid User Name\r\n";
static char *password = "Password:";
static char *passwordError = "Access denied\r\n";
#endif // MODULE_TELNET //
#endif //CMD_HISTORY

#define CLI_MAX_ARGV	 25

static char* argv_array[CLI_MAX_ARGV];

char* get_sys_adm_name(void) 
{	
	memset(admps, 0, 256);
	
	CFG_get(CFG_SYS_USERS, admps);

	return admps;
}

char* get_sys_adm_pass(void) 
{	
	memset(admps, 0, 256);
	
	CFG_get(CFG_SYS_ADMPASS, admps);

	return admps;
}

#ifdef MODULE_TELNET
void telnet_printf(char *message, int messagelen)
{
    if ((haveAuth == TELNET_PASS_AUTH) 
        && (message != NULL)
        && (messagelen > 0))
    {
	    int err = 0;
		err = write(fd_client, message, messagelen);
        if (err < 0)
        {
            haveAuth = TELNET_WITHOUT_AUTH;
            select_timeout.tv_sec = DEFAULT_TIMEOUT_VALUE;
            FD_CLR(fd_client, &fd_rd);
            FD_SET(fd_telnet, &fd_rd);
        }        
    }    
}
#else
void telnet_printf(char *message, int messagelen) {}
#endif // MODULE_TELNET //

int get_argc(const char* string)
{
	int		argc;
	char*	p;

	argc=0;
	p=(char*)string;

	while(*p)
	{
		if( *p != ' '  &&  *p )
		{
			argc++ ;
			while( *p != ' '  &&  *p ) p++;
			continue ;
		}
		p++ ;
	}
	if (argc >= CLI_MAX_ARGV) argc = CLI_MAX_ARGV-1;
	return argc;
}

char** get_argv(const char* string)
{
	char*	p;
	int		n;

	n=0;
	memset(argv_array, 0, CLI_MAX_ARGV*sizeof(char *));
	p = (char* )string;
	while(*p)
	{
		argv_array[n] = p ;
		while( *p != ' '  &&  *p ) p++ ;
		*p++ = '\0';
		while( *p == ' '  &&  *p ) p++ ;
		n++ ;
		if (n == CLI_MAX_ARGV) break;
	}
	return (char** )&argv_array ;
}

void my_memset(void * dst, int v, int len)
{
	char *p=(char *)dst;
	int i;
	for (i=0;i<len;i++)
		*p++ = v&0xff;
}

#ifdef CMD_HISTORY
static void get_cmd(char *buf, char *promt)
{
	unsigned char c;
	int clen, size=0;
	int esc_f1=0, esc_f2=0;
	int idx = cmd_index;
	char erase_line[]={0x1b, '[', '2', 'K', '\0'};
    int err;
    int readlen;

	while(1)
    {
        /* prevent input buffer overflows */
        if (size >= (CLI_MAX_CMD_LEN - 1))
        {
            diag_printf("\nWARNING : The maximum input size is %d bits long\n", CLI_MAX_CMD_LEN);
            buf[0] = '\0';
            return;
        }

        fd_set rd_res = fd_rd;
        c = 0;
        err = select(8, &rd_res, NULL, NULL, &select_timeout);
        
        if (err == 0) // select timeout
            continue;
        
        if (err < 0)
        {
            diag_printf("select returned error: %s\n", strerror(err));
            break;
        }
        
        if( FD_ISSET( ser0, &rd_res ) )
        {
            readlen = read( ser0, &c, 1);
            if (readlen < 1)
                continue;
        }

#ifdef MODULE_TELNET
        if(FD_ISSET(fd_telnet, &rd_res ))
        {       
            fd_client = accept( fd_telnet, (struct sockaddr *)&sa_client, &sa_client_len);
            if (fd_client < 0 )
            {
                diag_printf("socket() returned error: %s\n", strerror(fd_client));
                continue;
            }

            write(fd_client, userName, strlen(userName));
            select_timeout.tv_sec = 2;
            haveAuth = TELNET_WITHOUT_AUTH;
            isEACOMode = FALSE;
            FD_CLR(fd_telnet, &fd_rd);
            FD_SET(fd_client, &fd_rd);
            continue;
        }
        
        if(FD_ISSET(fd_client, &rd_res ))
        {
            readlen = read(fd_client, &c, 1);
            if (readlen < 1)
            {
                close(fd_client);
                select_timeout.tv_sec = DEFAULT_TIMEOUT_VALUE;
                haveAuth = TELNET_WITHOUT_AUTH;
                FD_CLR(fd_client, &fd_rd);
                FD_SET(fd_telnet, &fd_rd);
                continue;
            }

            if ((isEACOMode == TRUE) 
                && (c != TELNET_IAC) 
                && (c != 0xd))
            {
                if (c == 0xa)
                    write(fd_client, telnet_CR_LF, 2);
                else if (haveAuth == TELNET_WITHOUT_AUTH)
                    write(fd_client, &c, 1);
                else if (haveAuth == TELNET_IN_AUTH)
                    write(fd_client, "*", 1);
                else {
                    write(fd_client, &c, 1);
                    write(ser0, &c, 1);
                }
            }

            if (c == TELNET_IAC)
            {   
                unsigned char ResponseOption[3];
                unsigned char c2;
                unsigned char c3;
                unsigned char c4;
                
                read(fd_client, &c2, 1);
                read(fd_client, &c3, 1);
                ResponseOption[0] = TELNET_IAC;
                switch (c2)
                {
                    case TELNET_SB:
                        while (c3 != TELNET_IAC)
                        {
                            read(fd_client, &c3, 1);
                        }
                        read(fd_client, &c3, 1);
                        goto goto_continue;
                        break;
                    case TELNET_DONT:
                        ResponseOption[1] = TELNET_WONT;
                        break;
                    case TELNET_DO:
                        ResponseOption[1] = TELNET_WILL;
                        break;
                    case TELNET_WONT:
                        ResponseOption[1] = TELNET_DONT;
                        break;
                    case TELNET_WILL:
                        ResponseOption[1] = TELNET_DO;
                        break;
                    default:
                        goto goto_continue;
                }
                if (c3 == TELNET_ECHO)
                {                
                    if ((c2 == TELNET_WILL) && (ResponseOption[1] == TELNET_DO) 
                        || (c2 == TELNET_DO) && (ResponseOption[1] == TELNET_WILL))
                        isEACOMode = TRUE;                    
                }
                ResponseOption[2] = c3;
                write(fd_client, ResponseOption, 3);
goto_continue:
                continue;
            } 
            else if (c == 0xd)
            {
                continue;
            }
            else if (c == 0xa)
            {
                buf[size] = '\0';            
                switch(haveAuth)
                {
                    case TELNET_WITHOUT_AUTH:
                        /* check the input username is correct or not*/
    					if (!strcmp(buf, get_sys_adm_name()))  
    					{
    						write(fd_client, password, strlen(password));
    						haveAuth = TELNET_IN_AUTH;
    					}
    					else
    					{
    						write(fd_client, userNameError, strlen(userNameError));
    						write(fd_client, userName, strlen(userName));
    					}     
                        size = 0;
                        continue; 
                        break;
                    case TELNET_IN_AUTH:
                        /* check the input password is correct or not*/
    					if (!strcmp(buf, get_sys_adm_pass())) 
    					{
    						haveAuth = TELNET_PASS_AUTH;                            
    						buf[0] = '\0';
    						return;
    					}
    					else
    					{
    						write(fd_client, passwordError, strlen(passwordError));
    						write(fd_client, password, strlen(password));
    						size = 0;
    						continue;
    					}                    
                        break;
                    case TELNET_PASS_AUTH:
                        /* close the telnet connection if typing the command "exit" or "logout" from telnet clients */
    					if ((!strcmp(buf, "logout")) || (!strcmp(buf, "exit"))) 
    					{
    						close(fd_client);
                            select_timeout.tv_sec = DEFAULT_TIMEOUT_VALUE;
                            haveAuth = TELNET_WITHOUT_AUTH;
    						FD_CLR(fd_client, &fd_rd);
                            FD_SET(fd_telnet, &fd_rd);
                            size = 0;
                            continue; 
    					}	
                        break;
                }
                c = 0xd;
            } 
            else if (c == 0x3) // Ctrl + c
            {
                close(fd_client);
                select_timeout.tv_sec = DEFAULT_TIMEOUT_VALUE;
                haveAuth = TELNET_WITHOUT_AUTH;
                FD_CLR(fd_client, &fd_rd);
                FD_SET(fd_telnet, &fd_rd);
                size = 0;
                continue;
            }
        }
#endif // MODULE_TELNET //

       	switch (c) {
        	case 0x8:    /* drop through */
            case 0x7f:
                size -= 1;  // erase one character + 'backspace' char
                if (size < 0) {
                    size = 0;
                } else {
                    clen = 3;
                    diag_printf("\b \b");
                }
                continue;
#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
            case 0x7:   /* control-c , enter the debug mode */
                diag_printf("CTRL-G");
                breakpoint();
                continue;
                break;
#endif
			case 0x3b:   /* case of ; */
			case 0xd:
                if( FD_ISSET( ser0, &rd_res ))
                {
                    clen = 1;
                    diag_printf("\n");
                } else
                {
                    clen = 1;
                    write(ser0, "\r\n", 2);
                }
                buf[size] = '\0';
                return;
            case 0xa:
                if( FD_ISSET( ser0, &rd_res ))
                {
                    clen = 1;
                    diag_printf("\r");
                }
                continue;
            case 0x1b:
				esc_f1 = 1;
				continue;
			case 0x5b:
				if(esc_f1){
					esc_f2 = 1;
					continue;
				}
				esc_f1 = 0;
				break;
			case 0x41: // up
				if(esc_f1 && esc_f2 ){
					clen = 4;
                    diag_printf("%s", erase_line);
					diag_printf("\r%s>", promt);
					if(--idx < 0)
						idx = MAX_CMD_HISTORY-1;
					memcpy(buf, cmd_history[idx], CLI_MAX_CMD_LEN);
					clen = strlen(buf);
					size = clen;
                    buf[clen] = '\0';
                    diag_printf("%s", buf);                    
					continue;
				}
				break;
			case 0x42: //down
				if(esc_f1 && esc_f2 ){
					clen = 4;
                    diag_printf("%s", erase_line);
					diag_printf("\r%s>", promt);
					if(++idx > MAX_CMD_HISTORY-1)
						idx = 0;
					memcpy(buf, cmd_history[idx], CLI_MAX_CMD_LEN);
					clen = strlen(buf);
					size = clen;
                    buf[clen] = '\0';
                    diag_printf("%s", buf);                    
					continue;
				}
				break;
			case 0x43: //right
			// fix me
			case 0x44: //left
			// fix me
            default:
                break;
		}

        if( FD_ISSET( ser0, &rd_res ))
        {
            clen = 1;
            diag_printf("%c", c);
        }
        buf[size++] = c;
	}
}

void show_history(void)
{
	int i = cmd_index;
	
	do{
		if(--i < 0)
			i = MAX_CMD_HISTORY;
		diag_printf("%d: %s\n", i, cmd_history[i]);
	}while(i == cmd_index);
}

inline void save_in_history(char *buf)
{
	if(MAX_CMD_HISTORY-1 < cmd_index)
		cmd_index = 0;
	memcpy(cmd_history[cmd_index++], buf, CLI_MAX_CMD_LEN);
}
#endif //CMD_HISTORY

#ifdef CONFIG_USBHOST
//static MBlockFunc_T MUSBBlkFuncTbltest;
static cyg_handle_t usb_test_thread_h;
static cyg_thread usb_test_thread;
static char usb_test_stack[2048];
static int usb_flag = 0;

void usb_start()
{
	MBlockFunc_T MUSBBlkFuncTbltest;
	MBlockFunc_T *pMUSBBlkFuncTbl = &MUSBBlkFuncTbltest;
    MUSB_GetBlkFuncTbl(pMUSBBlkFuncTbl);
    int len = 4;
    int i = 0;
	cyg_uint8 *buf = (cyg_uint8 *)malloc(len*4096);
    memset(buf,0xa5,len*4096);
    int block_num = 0;
    time_t tm1,tm2;
    time(&tm1);
    int ret = 0;
	while(1)
	{
		if(usb_flag == 0)
		{
			cyg_thread_exit();
		}
    	time(&tm1);
		i = 0;
		block_num = 100000;
	    while((i < 10000) && usb_flag)
	    {
	        ret = pMUSBBlkFuncTbl->pfnRead(block_num,(cyg_uint32)buf,len*4096);
        	if(0 > ret){diag_printf("read fail\n");break;} 
			
	        ret = pMUSBBlkFuncTbl->pfnWrite(block_num+400000,(cyg_uint32)buf,len*4096);
	        if(0 > ret){diag_printf("write fail\n");break;} 
	        i++;
	        block_num += 32;   
	    }
		time(&tm2);
		if(((tm2-tm1) > 0) && (i > 0))
            diag_printf("usb read/write performace: %6dKB/s\n",(2*(i+1)*len*4096)/(tm2-tm1)/1024);
	}
    //free(buf);
}
int usb_test_init( void )
{
	//usb_flag= 0;
	cyg_thread_create(6, (cyg_thread_entry_t *)usb_start, (cyg_addrword_t)0, "USB_Test",
                          (void *)&usb_test_stack[0], sizeof(usb_test_stack), &usb_test_thread_h, &usb_test_thread);
	cyg_thread_resume(usb_test_thread_h);
}
#endif
int CLI_cmd_init()
{
	unsigned int opt = 1;  
    int err; 

#ifdef 	CMD_HISTORY
    select_timeout.tv_sec = DEFAULT_TIMEOUT_VALUE;
    select_timeout.tv_usec= 0;

    FD_ZERO( &fd_rd );
    ser0 = open("/dev/ser0", O_RDWR);
    if ( ser0 < 0 )
    {
        diag_printf("open(/dev/ser0) returned error: %s\n", strerror(ser0));
        return -1;
    }
    FD_SET( ser0, &fd_rd );
    
#ifdef MODULE_TELNET
    fd_telnet = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_telnet < 0 )
    {
        diag_printf("socket() returned error: %s\n", strerror(fd_telnet));
        return -1;
    }
	/* complete the socket structure */
	memset(&sa_telnet, 0, sizeof(sa_telnet));
	sa_telnet.sin_len = sizeof(sa_telnet);
	sa_telnet.sin_family = AF_INET;
	sa_telnet.sin_addr.s_addr = INADDR_ANY;
	sa_telnet.sin_port = htons(23);

    err = setsockopt(fd_telnet, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if ( err < 0 )
	{
        diag_printf("setsockopt() returned error: %s\n", strerror(err));
        return -1;
	}

    err = bind(fd_telnet, (struct sockaddr *) &sa_telnet, sizeof(sa_telnet));
    if ( err < 0 )
	{
        diag_printf("bind() returned error: %s\n", strerror(err));
        return -1;
	}

    err = listen(fd_telnet, 3);
    if ( err < 0 )
	{
        diag_printf("listen() returned error: %s\n", strerror(err));
        return -1;
	}

    FD_SET(fd_telnet, &fd_rd);
#endif // MODULE_TELNET //
#endif // CMD_HISTORY //
    return 0;
}

void CLI_cmd_proc(stCLI_cmd * cmds)
{
	int		argc;
	char**		argv;
	int             count, retval;
	stCLI_cmd       *cp;
	int 		found;
	static          int enter_level=0;
	int             help_mode;

	if (enter_level++ > 10)
	{
		diag_printf("enter level > 10\n\r");
		return ;
	}

	if (cmds->func && !(cmds->mode & CLI_CMD_FLAG_INITED))
	{
		cmds->func(0,0);
		cmds->mode |= CLI_CMD_FLAG_INITED ;
	}

	while(1)
	{

		diag_printf("%s>", cmds->name );	// show root prompt

		//memset(last_cmd_buf,0,CLI_MAX_CMD_LEN);

		my_memset(cur_cmd_buf, 0 , CLI_MAX_CMD_LEN);
#ifndef	CMD_HISTORY 
		gets(cur_cmd_buf);
#else
		get_cmd(cur_cmd_buf, cmds->name);
		if(cur_cmd_buf[0])		// record into history
			save_in_history(cur_cmd_buf);
#endif //CMD_HISTORY

		argc = get_argc((const char *)cur_cmd_buf);
		argv = (char**)get_argv((const char *)cur_cmd_buf);

		if(argc < 1) continue;

		if (!strcmp(argv[0], "reboot" ))
		{	
			mon_snd_cmd(MON_CMD_REBOOT);
			return;
		}

		help_mode=0;
		if ( (! strcmp( argv[0], "help" ) ||  (*(argv[0])=='?') ) )
		{
			help_mode=1;
			if (argc > 1)
				help_mode++; // 2 "help cmds"
		}
		else if ( (! strcmp( argv[0], "ls" )) )
		{
			help_mode=1;
		}
		else if (! strcmp( argv[0], "cd" ))
		{
			if ((argc > 1) && (! strcmp( argv[1], ".." )))
			{
				argv[0] = "up";

			}
			else if (argc > 1)
				strcpy(argv[0], argv[1]);
			else
				help_mode=1;                
		}
#ifdef CONFIG_WIRELESS
		if (!strcmp(argv[0], "iwpriv" ))
		{	
			CmdIwpriv(argc - 1, argv + 1);
			found = 1;
			goto exit;
		}
#endif // CONFIG_WIRELESS //
#ifdef CONFIG_CLI_OS_CMD
		if ((!strcmp(argv[0], "mbuf")) || (!strcmp(argv[0], "mem")))
		{	
			os_show_mem(argc - 1, argv + 1);
			found = 1;
			goto exit;
		}
		if (!strcmp(argv[0], "reg" ))
		{	
			os_register_cmd(argc - 1, argv + 1);
			found = 1;
			goto exit;
		}
#endif
#ifdef CONFIG_USBHOST
		if (!strcmp(argv[0], "usb_start" ))
		{	
			usb_flag = 1;
			//cyg_thread_resume(usb_test_thread_h);
			usb_test_init();
			found = 1;
			goto exit;
		}
		if (!strcmp(argv[0], "usb_stop" ))
		{	
			usb_flag = 0;
			//cyg_thread_suspend(usb_test_thread_h);
			found = 1;
			goto exit;
		}
#endif		
		found=0;
		for(cp=cmds->next,count=0; cp ; cp=cp->next )
		{
			switch (help_mode)
			{
				case 1: // single help
					if (! (cp->mode & CLI_CMD_FLAG_HIDE )) {
						diag_printf("%-12s", cp->name);
						if ( count++ > 5 )
						{
							printf("\n\r");
							count=0;
						}
					}
					break;

				case 2:
					if( ! strcmp( argv[1], cp->name ) )
					{
						diag_printf("%s\n\r", cp->doc);
						found++;
					}
					break;

				default:	// found
					if (! strcmp( argv[0], "up" ))
					{

						if (enter_level > 1)
						{
							if (cmds->prev)
								cmds = cmds->prev; // set to the parent cmd table
							enter_level--;
							return ;
						}
						found++;
					} 
					else if( ! strcmp( argv[0], cp->name ) )
					{
						if (cp->mode & CLI_CMD_FLAG_SUBCMD )
						{
							CLI_cmd_proc((stCLI_cmd * )(cp->func));
						}
						else
						{
							retval = cp->func(argc-1, argv+1);
						}
						found++;
					}
					break;
			} //switch 

			if (found)
				break;
		} // for (cp=cmds

exit:
		if ((!found)&&(help_mode!=1))
			diag_printf("Unknown Command !\r\n");
		else
			diag_printf("\n\r");

	}
}


/*
	to add a new command (news) entry following to the current 
	command (cur).
*/
int CLI_add_cmd(stCLI_cmd * cur,  stCLI_cmd * newc)
{
	if (cur->next)
	{
		cur->next->prev=newc;
		newc->next=cur->next;
	}
	else // last one
	{
		newc->next=0;
	}
	
	newc->prev = cur;
	cur->next = newc;

	return 0;
}

/*
	To find, and delete a command entry from the current command list
	according to the name.
*/
int CLI_del_cmd(stCLI_cmd * cur,  char * name)
{
	stCLI_cmd  * cp;

	for (cp=cur; cp->next; cp=cp->next)
	{	
		if (strcmp(cp->name, name))
			continue;
		// yes, delete it
		if (cp->next)
		{
			cp->next->prev=cp->prev;
			cp->prev->next=cp->next;
		}
		else // last one
		{
			cp->next=0;
		}
		return 0;
	}	
	return -1;
}

/*
	init the commands from an array to a link list
*/
int CLI_init_cmd(stCLI_cmd * cmds,  int size)
{
	int i;
	
	for (i=1; i < size  ; i++)
	{
		CLI_add_cmd(&cmds[i-1], &cmds[i]);
	}

	return 0;
}


