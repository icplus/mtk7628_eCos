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
    cmd_os.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <pkgconf/system.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <cli_cmd.h>

int os_show_mem(int argc , char* argv[]);
int os_register_cmd(int argc , char* argv[]);

static int os_cmd_init(int argc, char* argv[]);
static int os_show_threads(int argc , char* argv[]);
static int os_thread_cmd(int argc , char* argv[]);
static int os_spi_cmd(int argc , char* argv[]);
static int os_i2c_cmd(int argc , char* argv[]);
static void os_show_log(int argc , char* argv[]);

#ifdef CONFIG_CLI_NET_CMD
#define CLI_OSCMD_FLAG			0
#else
#define CLI_OSCMD_FLAG 		CLI_CMD_FLAG_HIDE
#endif

#ifdef CONFIG_CPULOAD
extern void cpuload_cmd(int argc,char *argv[]);
static int cpubusy_cmd(int argc , char* argv[]);
#endif

char help_thread [] = {"thread <show>\nthread id pri pri-val\nthread id state run/suspend/release"};

stCLI_cmd os_cmds[] =
{
	{ "OS" , &os_cmd_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "thread" , os_thread_cmd, &help_thread, CLI_OSCMD_FLAG, 0, 0 },
	{ "mem" , os_show_mem, "Show Memory", CLI_OSCMD_FLAG, 0, 0 },
	{ "spi" , os_spi_cmd, "SPI <rd/wr/er> <addr> [len]", CLI_OSCMD_FLAG, 0, 0 },	
	{ "i2ccmd" , os_i2c_cmd, "i2ccmd <set/read/write/dump> <addr> [len]", CLI_OSCMD_FLAG, 0, 0 },
	{ "reg" , os_register_cmd, "Read/Write Register", CLI_OSCMD_FLAG, 0, 0 },
#ifdef CONFIG_CPULOAD
	{"cpuload",cpuload_cmd,"cpuload start/stop",CLI_OSCMD_FLAG,0,0},
	{"cpubusy",cpubusy_cmd,"cpubusy time(unit second)",CLI_OSCMD_FLAG,0,0},
#endif
#ifdef	CONFIG_SYSLOG
	{ "event",os_show_log, "Show log", 0, 0, 0},
#endif
};

static int os_cmd_init(int argc, char* argv[])
{
	// default cmds
	CLI_init_cmd(&os_cmds[0], CLI_NUM_OF_CMDS(os_cmds) );	

	return 0;
}

static char* thread_state[]={\
	"RUN",//0
	"CNTSLEEP",///2
	"SUSP",	///4
	"CREATE",///8
	"EXIT",///16
};\
static int os_show_threads(int argc , char* argv[])
{
        cyg_handle_t thread = 0;
        cyg_uint16 id = 0;
        int more;

        diag_printf("id    state    Pri(Set) Name                      StackBase   Size   usage\n\r");
        diag_printf("---------------------------------------------------------------------------\n\r");

        /* Loop over the threads, and generate a table row for
         * each.
         */
        do 
        {
                cyg_thread_info info;
                char *state_string;

                more=cyg_thread_get_next( &thread, &id );
                if (thread==0)
                        break;

                cyg_thread_get_info( thread, id, &info );
                
                if( info.name == NULL )
                        info.name = "----------";

                /* Translate the state into a string.
                 */
                /*if( info.state == 0 )
                        state_string = "RUN";
                else if( info.state & 0x04 )
                        state_string = "SUSP";
                else switch( info.state & 0x1b )
                {
                        case 0x01: state_string = "SLEEP"; break;
                        case 0x02: state_string = "CNTSLEEP"; break;
                        case 0x08: state_string = "CREATE"; break;
                        case 0x10: state_string = "EXIT"; break;
                        default: state_string = "????"; break;
                }*/
				 /* Translate the state into a string.*/
				if( info.state == 1 )
					state_string = "SLEEP";
				else
				{
					if((info.state&0x1F)<8)
						state_string=thread_state[(info.state&0x1F)/2];
					else
						state_string=thread_state[(info.state&0x1F)/8+2];	
				}
                /* Now generate the row. */
                diag_printf("%04d  %-8s %-2d ( %-2d) ", id, state_string, info.cur_pri, info.set_pri);
                diag_printf("%-25s 0x%08x  %-6d %-6d", 	info.name, info.stack_base , info.stack_size, 
                                                cyg_thread_measure_stack_usage(thread));
                diag_printf("\r\n");
        } while (more==true) ;
    return 0;
}

static int os_thread_cmd(int argc , char* argv[])
{
	/* Get the thread id from the hidden control */
	cyg_handle_t thread = 0;
	cyg_uint16 id;


	if ((argc==0) || !strcmp(argv[0], "show"))
	{
		os_show_threads(0,0);
		return 0;
	}

	if (argc > 2)
	{
		if (sscanf( argv[0], "%04d", &id )==0 || 
				((thread = cyg_thread_find( id ))==0))
		{
			printf("thread id %s not exist\n", argv[0]);
			return -1;
		}

		if (!strcmp(argv[1], "pri"))
		{
			cyg_priority_t pri;

			sscanf( argv[2], "%d", &pri );
			cyg_thread_set_priority( thread, pri );
			return 0;
		}

		/* If there is a state field, change the thread state */
		if (!strcmp(argv[1], "state"))
		{
			if( strcmp( argv[2], "run" ) == 0 )
				cyg_thread_resume( thread );
			else if( strcmp( argv[2], "suspend" ) == 0 )
				cyg_thread_suspend( thread );
			else if( strcmp( argv[2], "release" ) == 0 )
				cyg_thread_release( thread );
			else if( strcmp( argv[2], "delete" ) == 0 )
				cyg_thread_delete( thread );
			else if( strcmp( argv[2], "kill" ) == 0 )
				cyg_thread_kill( thread );
			else
				goto help;

			cyg_thread_delay(10);
			os_show_threads(0,0);
			return 0;
		}
	}
help:
	printf("thread <show>\n");
	printf("thread id pri pri-val\n");
	printf("thread id state run/suspend/release/delete\n");
	return 0;
}

unsigned int os_mem_ceiling(void)
{
	struct mallinfo mem_info;
    
    mem_info = mallinfo();
	
	return mem_info.maxfree;
}

int os_show_mem(int argc , char* argv[])
{
        struct mallinfo mem_info;
       
        mem_info = mallinfo();

        printf("Memory system: \n"); 
        printf("   Total %dk, Free %dk, Largest free block %dk\n", 
                mem_info.arena/1024, mem_info.fordblks/1024, mem_info.maxfree/1024);

        cyg_kmem_print_stats();
	return 0;
}

int os_register_cmd(int argc , char* argv[])
{
	cyg_addrword_t addr;
	cyg_uint32 val;

        if ((argc==2) && (!strcmp(argv[0], "r") || !strcmp(argv[0], "read")))
        {
                if (sscanf(argv[1], "%x", &addr) != 1)
                        goto help;

                addr &= 0x0FFFFFFC;
                addr |= 0xb0000000;
		val = HAL_REG32(addr);

                printf("Address = 0x%8x, Value = 0x%8x\n", (addr & 0xFFFFFFFF), (val & 0xFFFFFFFF));
                return 0;
        }

        if ((argc==3) && (!strcmp(argv[0], "w") || !strcmp(argv[0], "write")))
        {
                if (sscanf(argv[1], "%x", &addr) != 1)
                        goto help;

                if (addr & 0x00000003)
                {
                        printf("Address = 0x%x is not correct\n", addr);
                        return 0;
                }
                addr &= 0x0FFFFFFC;
                addr |= 0xb0000000;                

                if (sscanf(argv[2], "%x", &val) != 1)
                        goto help;

		HAL_REG32(addr) = val;
		val = HAL_REG32(addr);
                printf("Address = 0x%8x, Value = 0x%8x\n", (addr & 0xFFFFFFFF), (val & 0xFFFFFFFF));
                return 0;
        }

help:
        printf("reg r/read address\n");
        printf("reg w/write address value\n");
        return 0;
}

static int os_spi_cmd(int argc , char* argv[])
{
	cyg_flashaddr_t addr, err_addr;
	cyg_uint8 *p;
	cyg_uint32 *buf;
	cyg_uint32 len, val, res;
	int i, j, status;

    if (argc == 0)
		goto err_out;
	
	if ((strcmp(argv[0], "rd") == 0) && (argc > 1 ) && (argc < 4))
	{
		if (sscanf(argv[1], "%x", &addr)!=1)
			goto err_out;
		
		if (argc > 2)
		{
			if (sscanf(argv[2], "%d", &len) != 1)
				goto err_out;
		}
		else
			len = 64;
		
		res = len % 4;
		switch (res)
		{
		case 1:
			len += 3;
			break;
		case 2:
			len += 2;
			break;
		case 3:
			len += 1;
			break;
		}
		if(len > 2*1024*1024)//eCos SDK max SPI flash size is 2M.
			len = 2*1024*1024;
		if (addr <0 || addr >= 2*1024*1024)
			addr=0;
		if(len + addr > 2*1024*1024)//eCos SDK max SPI flash size is 2M.
			len = 2*1024*1024 - addr;		
		p = (cyg_uint8 *)malloc(len);
		if(!p){
			diag_printf("os_spi_cmd malloc failed!\n");
			return -1;
			}
		status = cyg_flash_read(addr, p, len, &err_addr);
		if (status != CYG_FLASH_ERR_OK)
		{
			diag_printf("SPI: flash read err!!\n");
			free(p);
			goto out;
		}
		buf = (cyg_uint32 *)p;
		
		j = 0;
		for (i=0; i<len/4; i++)
		{
			diag_printf("%08x ", buf[i]);
			j++;
			if (j == 4)
			{
				j = 0;
				diag_printf("\n");
			}
		}
		diag_printf("\n");
		free(p);
	}
	else if ((strcmp(argv[0], "wr") == 0) && (argc > 2))
	{
		if (sscanf(argv[1], "%x", &addr)!=1)
			goto err_out;
		if (addr <0 ||addr >=2*1024*1024)
			{
			diag_printf("[%s:%d]address error %d\n",__FUNCTION__,__LINE__,addr);
			return -1;
			}
			len = argc-2;
			if (len <0 ||len >=2*1024*1024)
				{
				diag_printf("[%s:%d]len error %d\n",__FUNCTION__,__LINE__,len);
				return -1;
				}
			p = (cyg_uint8 *)malloc(len);
			if (p == NULL)
				{
				diag_printf("[%s:%d]malloc fail\n",__FUNCTION__,__LINE__);
				return -1;
				}
			for (i=2; i<argc; i++)
			{
				sscanf(argv[i], "%x", &val);
				p[i-2] = (cyg_uint8)val;
			}
			status = cyg_flash_program(addr, p, len, &err_addr);
			if (status != CYG_FLASH_ERR_OK)
				diag_printf("SPI: flash write err!!\n");
			free(p);
		}
	else if (strcmp(argv[0], "er") == 0)
	{
		if (sscanf(argv[1], "%x", &addr)!= 1)
			goto err_out;
		if (argc > 2)
		{
			if (sscanf(argv[2], "%d", &len)!= 1)
				goto err_out;
		}
		else
			len = 1;
		cyg_flash_erase(addr, len, &err_addr);
	}
	else
        printf("spi <rd/wr/er> <addr> [len]\n");        
	
out:
	return 0;
err_out:
	printf("spi <rd/wr/er> <addr> [len]\n");    
	return -1;
}

#include "./i2c_drv.c"
static int os_i2c_cmd(int argc , char* argv[])
{
#if 0
	cyg_flashaddr_t addr, err_addr;
	cyg_uint8 *p;
	cyg_uint32 *buf;
	cyg_uint32 len, val, res;
	int i, j, status;
#endif
	int chk_match, size;
	ulong addr, value;
	u16 address;

    if (argc == 0)
		goto err_out;

#ifndef BBU_I2C
	/* configure i2c to normal mode */
#if defined (MT7621_FPGA_BOARD) || defined (MT7621_ASIC_BOARD)
	RT2880_REG(RT2880_GPIOMODE_REG) &= ~(1 << 2);
#elif defined (MT7628_FPGA_BOARD) || defined (MT7628_ASIC_BOARD)
	RT2880_REG(RT2880_GPIOMODE_REG) &= ~(1 << 16);
#else
	RT2880_REG(RT2880_GPIOMODE_REG) &= ~1;
#endif
#endif

	switch (argc) {
		case RT2880_I2C_DUMP:
			chk_match = strcmp(argv[1], RT2880_I2C_DUMP_STR);

			if (chk_match != 0) {
				goto err_out;
				return 1;
			}

			i2c_eeprom_dump();
			break;

		case RT2880_I2C_READ:
			chk_match = strcmp(argv[1], RT2880_I2C_READ_STR);

			if (chk_match != 0) {
				goto err_out;
				return 1;
			}

			addr = simple_strtoul(argv[2], NULL, 16);
			address = addr;
			i2c_master_init();
			i2c_eeprom_read(addr, (u8 *)&value, 4);
			printf("0x%04x : 0x%04x\n", address, value);
			break;

		case RT2880_I2C_WRITE:
			chk_match = strcmp(argv[1], RT2880_I2C_WRITE_STR);

			if (chk_match != 0) {
				goto err_out;
				return 1;
			}

			size = simple_strtoul(argv[2], NULL, 16);
			addr = simple_strtoul(argv[3], NULL, 16);
			value = simple_strtoul(argv[4], NULL, 16);
			i2c_master_init();
			address = addr;
			i2c_eeprom_write(address, (u8 *)&value, size);
			printf("0x%08x: 0x%08x in %d bytes\n", address, value, size);
			break;

		case RT2880_I2C_DEVADDR:
			chk_match = strcmp(argv[2],  RT2880_I2C_DEVADDR_STR);

			if (chk_match != 0) {
				goto err_out;
				return 1;
			}

			i2c_devaddr = simple_strtoul(argv[3], NULL, 16);
			break;

		default:
				goto err_out;
	}
	return 0;

err_out:
	printf("i2ccmd	- read/write data to eeprom via I2C Interface\n");
	printf("i2ccmd read/write eeprom_address data(if write)\n");
	printf("i2ccmd format:\n");
	printf("  i2ccmd set devaddr [addr in hex]\n");
	printf("  i2ccmd read [offset in hex]\n");
	printf("  i2ccmd write [size] [offset] [value]\n");
	printf("  i2ccmd dump\n");
	return -1;
}


#ifdef CONFIG_CPULOAD
static cyg_thread	cpubusy_thread;
static cyg_handle_t	 cpubusy_thread_handle;
static char cpubusy_stack[1024];
static char cpubusy_running = 0;
void cpubusy(CYG_ADDRESS data)
{
	printf("\ncpubusy_thread start time=%d seconds\n", data);	

	int i = 0;
	unsigned long start_time, now_time;
	start_time = cyg_current_time();
	while (cpubusy_running)
	{
		i++;
		now_time = cyg_current_time();
		if ((now_time - start_time) > (data*100))
			break;
	}

	cpubusy_running = 0;
	printf("\ncpubusy_thread end\n");
}

static int cpubusy_cmd(int argc , char* argv[])
{
	int busy_time=10;

	if ((argc > 0) && (strcmp(argv[0], "stop") == 0))
	{
		cpubusy_running = 0;
	} else if (cpubusy_running == 0)
	{
		if (argc > 0)
			busy_time=atoi(argv[0]);

		cpubusy_running = 1;	
		cyg_thread_create(20,
				  (cyg_thread_entry_t *) cpubusy,
				  busy_time,
				  "cpubusy thread",
				 (void *)&cpubusy_stack[0] ,
				  sizeof(cpubusy_stack),
				  &cpubusy_thread_handle,
				  &cpubusy_thread);	
  		cyg_thread_resume(cpubusy_thread_handle);
	}
	return 0; 
}
#endif
#ifdef	CONFIG_SYSLOG
#include <eventlog.h>
extern int log_pool_num ;
static void os_show_log(int argc, char* argv[])
{
	int i, count;
	char time_info[32]={0};
	UINT32	ev_dateTime;
	int	diff = ntp_time() - time(0);
	
    if (argc == 0)
    {
	    goto help;
    }
	
	if (strcmp(argv[0],"print") == 0) 
	{
		EventLogEntry *ev;
		char *p;
		int j, index;
		
		index = (argc<2) ? 1 : atoi(argv[1]);
		count = (argc<3) ? 65535 : atoi(argv[2]);
		
		syslog_mutex_lock();
		
		for (j=0;j<log_pool_num;j++)
		{
			if(j == SYSTEM_LOG)
				diag_printf("/*----------System Log-----------------*/\n");
			if(j == SECURE_LOG)
				diag_printf("/*----------Security Log---------------*/\n");
			p = FindEvent(j);	
			for (i=1; (p != 0) && (i < index); i++, p = NextEvent(p,j));
			for (i=0; (p != 0) && (i < count); i++, p = NextEvent(p,j))
			{
				ev = (EventLogEntry *)p;
		    	ev_dateTime = gmt_translate2localtime(diff + ev->unix_dateTime);
				ctime_r(&ev_dateTime, time_info);
				if (strlen(time_info)==0)
					{
					diag_printf("[%s:%d]time_info==0\n",__FUNCTION__,__LINE__);
					return ;
					}
				time_info[strlen(time_info)-1] = 0;  // Strip \n
				diag_printf ("[%s]: <%s-%s> %s %s\n", 
							time_info, 
							EventClass(ev), 
							EventLevel(ev), 
							EventModule(ev), 
							ev->text);
			}
		}
		
		syslog_mutex_unlock();
	}
	else if ( strcmp(argv[0],"reset") == 0 ) 
	{
		clear_system_log();
		clear_seurity_log();
	}
	else if ( strcmp(argv[0], "gen") == 0)
	{
		static int gen_count = 0;
		extern int log_count_max;
		
		count = (argc<2) ?  log_count_max : atoi(argv[1]);
		if (count <0 || count >CONFIG_LOG_MAX_COUNT)
			count = CONFIG_LOG_MAX_COUNT;
		
		for (i=0; i<count && i < CONFIG_LOG_MAX_COUNT; i++)
		{
			SysLog(LOG_USER|SYS_LOG_INFO|LOGM_LOGD,"Evenglog Gen %d", gen_count);
			gen_count++;
			
			if ((i%10) == 9)
				cyg_thread_delay(200);
		}
		
		diag_printf("Generate %d logs\n", count);
	}
	else if ( strcmp(argv[0], "mail") == 0)
	{
		event_send_mail();
	}
	else
	{
	    goto help;
	}

    return;
    
help:
    diag_printf("Use following for more info.\n");
    diag_printf("  event print <start-index> <count>\n");
    diag_printf("  event gen <count>\n");
    diag_printf("  event mail\n");
    diag_printf("  event reset\n");	
    return;
}
#endif

