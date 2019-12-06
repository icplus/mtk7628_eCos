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
    cmd_dbg.c

    Abstract:
      debug command

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#include <string.h>
#include <stdio.h>
#include <cli_cmd.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cfg_def.h>
#include <cfg_id.h>

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

static int dbg_cmds_init(int argc, char* argv[]);

static int dbg_dump_byte_cmd(int argc, char* argv[]);
static int dbg_dump_short_cmd(int argc, char* argv[]);
static int dbg_dump_word_cmd(int argc, char* argv[]);
#ifdef CONFIG_CLI_ADV_DBGCMD
static int dbg_copy_byte_cmd( int argc, char* argv[]);
static int dbg_copy_short_cmd( int argc, char* argv[]);
static int dbg_copy_word_cmd( int argc, char* argv[]);
static int dbg_fill_byte_cmd( int argc, char* argv[]);
static int dbg_fill_short_cmd( int argc, char* argv[]);
static int dbg_fill_word_cmd( int argc, char* argv[]);
#endif
static int dbg_write_byte_cmd( int argc, char* argv[]);
static int dbg_write_short_cmd( int argc, char* argv[]);
static int dbg_write_word_cmd( int argc, char* argv[]);
#ifdef CONFIG_CLI_ADV_DBGCMD
static int flsh_write_cmd( int argc, char* argv[]);
static int flsh_erase_cmd( int argc, char* argv[]);
static int flsh_read_cmd(int argc, char *argv[]);
#endif

#ifdef CONFIG_WATCHDOG
extern int watchdog_cmd(int argc,char** argv);
#endif

extern void sys_reboot(void);
#ifdef SUPPORT_BREAKP_CMD
static int sys_breakp( int argc, char* argv[]);
#endif

extern void if_ra305x_debug(char *cmd);
static int eth_set_debug(int argc, char* argv[]);

#ifdef CONFIG_CLI_DBG_CMD
#define CLI_DBGCMD_FLAG			0
#else
#define CLI_DBGCMD_FLAG			CLI_CMD_FLAG_HIDE
#endif


stCLI_cmd dbg_cmds[] =
{
	{ "DBG" , &dbg_cmds_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "d" , &dbg_dump_word_cmd,    "d  <addr> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "dh" , &dbg_dump_short_cmd,  "dh <addr> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "db" , &dbg_dump_byte_cmd,   "db <addr> <len>", CLI_DBGCMD_FLAG, 0, 0 },
#ifdef CONFIG_CLI_ADV_DBGCMD
	{ "c" , &dbg_copy_word_cmd,    "c  <src>  <dst> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "ch" , &dbg_copy_short_cmd,  "ch <src>  <dst> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "cb" , &dbg_copy_byte_cmd,   "cb <src>  <dst> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "f" , &dbg_fill_word_cmd,    "f  <addr> <pat> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "fh" , &dbg_fill_short_cmd,  "fh <addr> <pat> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "fb" , &dbg_fill_byte_cmd,   "fb <addr> <pat> <len>", CLI_DBGCMD_FLAG, 0, 0 },
#endif	/*  CONFIG_CLI_ADV_DBGCMD  */
	{ "w" , &dbg_write_word_cmd,   "w  <addr>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "wh" , &dbg_write_short_cmd, "wh <addr>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "wb" , &dbg_write_byte_cmd,  "wb <addr>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "rst" , &sys_reboot,  "reboot", 0, 0, 0 },
#ifdef SUPPORT_BREAKP_CMD
	{ "brk" , &sys_breakp,  "breakpoint", CLI_DBGCMD_FLAG, 0, 0 },
#endif

#ifdef CONFIG_CLI_ADV_DBGCMD
	{ "flr" , &flsh_read_cmd,  "flr <dst> <len>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "flw" , &flsh_write_cmd,  "flw <dst> <len> <src>", CLI_DBGCMD_FLAG, 0, 0 },
	{ "fle" , &flsh_erase_cmd,  "fle <dst> <len>", CLI_DBGCMD_FLAG, 0, 0 },
#ifdef	CONFIG_WATCHDOG
	{ "wdog" , &watchdog_cmd, "watchdog cmd", 0, 0, 0 },
#endif
	{ "eth" , &eth_set_debug, "eth <mask>", 0, 0, 0 },
#endif	/*  CONFIG_CLI_ADV_DBGCMD  */
};

static unsigned int cur_dump_addr=0x80000000;

//==============================================================================
//                               EXTERNAL VARIABLES
//==============================================================================
extern unsigned int spi_test_count;

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
static int dbg_cmds_init(int argc, char* argv[])
{
	// test cmds
	CLI_init_cmd(&dbg_cmds[0], CLI_NUM_OF_CMDS(dbg_cmds) );
	return 0;
}

#define PAGE_ECHO_HEIGHT    24

static int dump_mem( int argc, char* argv[], int type)
{
	unsigned int	addr,cur_addr;
	unsigned int	size;
	int				lines;
	char			c;

	addr = cur_dump_addr ;
	size	= 128;
	if( argc > 0 )
	{
		if ( !sscanf(argv[0], "%x",  &addr ) )
		{
		    printf("Invalid addr: %s\n", argv[0]);
		    return -1;
        }
		
	}
	if( argc > 1 )
	{
		if ( !sscanf(argv[1], "%x", &size ) )
		{
			printf(" Invalid size: %s.\n", argv[1]);
			return -1;
		}
	}

	if(type==1)
	{
		if(addr & 1)
		{
			printf(" Invalid addr(half word boundary) value.\n");
			return -1 ;
		}
	}
	else
	{
		if(type==2)
		{
			if(addr & 3)
			{
				printf(" Invalid addr(word boundary) value.\n");
				return -1 ;
			}
		}
	}
#if 1
	if( addr < 0x80000000 )
	{
		printf(" Can not dump kuseg address, address must >= 0x80000000.\n");
		return -1 ;
	}
#endif
	lines = 0;
	cur_addr = addr & 0xfffffff0 ;
	while(	cur_addr < (addr + size) )
	{
		if( cur_addr % 16 == 0 )
		{
			lines++ ;
			if( lines == PAGE_ECHO_HEIGHT )
			{
				printf("\n[.....Press any key to continue (<q> to quit).....]\r");
				c = (char) getchar();
				printf("						   \r");
				if(c=='q' || c=='Q')
				    break;
				lines = 0 ;
			}
			printf("\n%08X: ", cur_addr );
		}

		switch(type)
		{
			case 0:
				if( cur_addr < addr )
				    printf("   ");
				else
				    printf("%02X ", *((volatile unsigned char *)cur_addr) );
				cur_addr += 1;
				break;
			case 1:
				if( cur_addr < addr )
				    printf("	");
				else
				    printf("%04X ", *((volatile unsigned short *)cur_addr) );
				cur_addr += 2;
				break;
			case 2:
				if( cur_addr < addr )
				    printf("	    ");
				else
				    printf("%08X ", *((volatile unsigned int *)cur_addr) );
				cur_addr += 4;
				break;
		}
	}
	cur_dump_addr = addr + size ;
	printf("\n");
	return 0 ;
}

static int dbg_dump_byte_cmd( int argc, char* argv[])
{
    return dump_mem(argc, argv, 0);
}
static int dbg_dump_short_cmd( int argc, char* argv[])
{
    return dump_mem(argc, argv, 1);
}
static int dbg_dump_word_cmd( int argc, char* argv[])
{
    return dump_mem(argc, argv, 2);
}


#ifdef CONFIG_CLI_ADV_DBGCMD

static int copy_mem( int argc, char* argv[], int  type)
{
	unsigned int src_addr,dst_addr;
	unsigned int size, i;

	size	= 1;
	if( argc < 2 )
	{
		printf("C[B|H|W] <src_addr> <dst_addr> <size>\n");
		return -1 ;
	}

	if(!sscanf( argv[0], "%x", &src_addr ))
	{
	    printf("Invalid addr: %s\n", argv[0]);
		return -1 ;
	}
	if(!sscanf( argv[1], "%x", &dst_addr ))
	{
	    printf("Invalid addr: %s\n", argv[1]);
		return -1 ;
	}
	if( argc > 2 )
	{
		if(!sscanf( argv[2], "%x", &size ))
		{
		    printf(" Invalid size: %s.\n", argv[2]);
			return -1 ;
		}
	}
    
	if( type==1)
	{
		if(src_addr & 1)
		{
			printf(" Invalid src_addr(half word boundary) value.\n");
			return -1 ;
		}
		if(dst_addr & 1)
		{
			printf(" Invalid dst_addr(half word boundary) value.\n");
			return -1 ;
		}
	}
	else
	{
		if( type==2)
		{
			if(src_addr & 3)
			{
				printf(" Invalid src_addr(word boundary) value.\n");
				return -1 ;
			}
			if(dst_addr & 3)
			{
				printf(" Invalid dst_addr(word boundary) value.\n");
				return -1 ;
			}
		}
	}
#if 1
	if (( src_addr < 0x80000000 ) || ( dst_addr < 0x80000000 ))
	{
		printf("address must >= 0x80000000.\n");
		return -1 ;
	}
#endif
	for(i=0;i<size;i++)
	{
		switch( type)
		{
			case 0:
				*((volatile unsigned char *)dst_addr) = *((volatile unsigned char *)src_addr);
				src_addr += 1;
				dst_addr += 1;
				break;
			case 1:
				*((volatile unsigned short *)dst_addr)	= *((volatile unsigned short *)src_addr);
				src_addr += 2;
				dst_addr += 2;
				break;
			case 2:
				*((volatile unsigned int *)dst_addr) = *((volatile unsigned int *)src_addr);
				src_addr += 4;
				dst_addr += 4;
				break;
		}
	}
	return 0;
}

static int dbg_copy_byte_cmd( int argc, char* argv[])
{
    return copy_mem(argc, argv, 0);
}
static int dbg_copy_short_cmd( int argc, char* argv[])
{
    return copy_mem(argc, argv, 1);
}
static int dbg_copy_word_cmd( int argc, char* argv[])
{
    return copy_mem(argc, argv, 2);
}


/*
    fill memory
*/
static int fill_mem( int argc, char* argv[], int  type)
{
	unsigned int addr;
	unsigned int size,value,i;

	size	= 1;
	if( argc < 2 )
	{
		printf(" F[B|H|W]  <addr> <value> <size>.\n");
		return  -1 ;
	}
	if(!sscanf( argv[0], "%x",  &addr ))
	{
	    printf("Invalid addr: %s\n", argv[0]);
		return  -1 ;
	}
	if(!sscanf( argv[1], "%x", &value ))
	{
	    printf("Invalid hex: %s\n", argv[1]);
		return  -1 ;
	}
	if( argc > 2 )
	{
		if(!sscanf( argv[2], "%x", &size ))
		{
		    printf(" Invalid size: %s.\n", argv[2]);
			return  -1 ;
		}
	}

	if( type==1)
	{
		if(addr & 1)
		{
			printf(" Invalid addr(half word boundary) value.\n");
			return  -1 ;
		}
	}
	else
	{
		if( type==2)
		{
			if(addr & 3)
			{
				printf(" Invalid addr(word boundary) value.\n");
				return  -1 ;
			}
		}
	}
#if 1
	if( addr < 0x80000000 )
	{
	    printf("address must >= 0x80000000.\n");
		return  -1 ;
	}
#endif
	for(i=0;i<size;i++)
	{
		switch( type)
		{
			case 0:
				*(((volatile unsigned char *)addr)+i) = value;
				break;
			case 1:
				*(((volatile unsigned short *)addr)+i) = value;
				break;
			case 2:
				*(((volatile unsigned int *)addr)+i) = value;
				break;
		}
	}
	return 0 ;
}

static int dbg_fill_byte_cmd( int argc, char* argv[])
{
    return fill_mem(argc, argv, 0);
}
static int dbg_fill_short_cmd( int argc, char* argv[])
{
    return fill_mem(argc, argv, 1);
}
static int dbg_fill_word_cmd( int argc, char* argv[])
{
    return fill_mem(argc, argv, 2);
}
#endif	/* CONFIG_CLI_ADV_DBGCMD */



static int write_mem( int argc, char* argv[], int type)
{
	char buffer[30];
	unsigned int addr;
	unsigned int value;

	if( argc > 0 )
	{
		if(!sscanf( argv[0], "%x", &addr ))
		{
		    printf("Invalid addr: %s\n", argv[0]);
			return -1 ;
		}
	}
	else
	{
		printf(" W[B|H|W] <address>\n");
		return -1 ;
	}

	if( type==1)
	{
		if( addr & 1)
		{
			printf(" Invalid  addr(half word boundary) value.\n");
			return -1 ;
		}
	}
	else
	{
		if( type==2)
		{
			if( addr & 3)
			{
				printf(" Invalid  addr(word boundary) value.\n");
				return -1 ;
			}
		}
	}
#if 1
	if(  addr < 0x80000000 )
	{
        printf("address must >= 0x80000000.\n");
		return -1 ;
	}
#endif
	printf("Press '.' to end input data, <enter> skip to input next data\n");

	while(1)
	{
		printf( "[%08X: ",  addr );
		switch( type)
		{
			case 0:
				printf( "%02X] ",*((volatile unsigned char *) addr));
				break;
			case 1:
				printf( "%04X] ",*((volatile unsigned short *) addr));
				break;
			case 2:
				printf( "%08X] ",*((volatile unsigned int *) addr));
				break;
		}

		gets( buffer );

		if( buffer[0] == '.' )
		    break ;

        if( !sscanf( buffer, "%x", &value ))
		{
			printf("\t\tInvalid HEX value %s.\n", buffer );
				continue ;
        }

        switch( type)
		{
		    case 0:
			    *((volatile unsigned char *) addr)	=  value;
			    break;
		    case 1:
    		    *((volatile unsigned short *) addr)	=  value;
			    break;
		    case 2:
			    *((volatile unsigned int *) addr)	=  value;
			    break;
        }

		switch( type)
		{
			case 0:
				addr += 1;
		        break;
			case 1:
				addr += 2;
				break;
			case 2:
				addr += 4;
				break;
		}
	}
	return 0 ;
}

static int dbg_write_byte_cmd( int argc, char* argv[])
{
    return write_mem(argc, argv, 0);
}
static int dbg_write_short_cmd( int argc, char* argv[])
{
    return write_mem(argc, argv, 1);
}
static int dbg_write_word_cmd( int argc, char* argv[])
{
    return write_mem(argc, argv, 2);
}

#ifdef SUPPORT_BREAKP_CMD
static int sys_breakp( int argc, char* argv[])
{
#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS

	if (argc > 0)
	{
		unsigned int bp;
		if(sscanf( argv[0], "%x", &bp ))
		{
			install_async_breakpoint(bp);
			diag_printf("set BP at %x\n", bp);
		}
		else
			diag_printf("invalid bp!\n");
	}
	else
	{
    	diag_printf("\nBRK!!\n");
    	breakpoint();
	}
#endif
	return 0;
}
#endif	/*  SUPPORT_BREAKP_CMD  */



#ifdef CONFIG_CLI_ADV_DBGCMD
#define MAX_FLASH_READ_LEN		512
#define PRINT_COL_NUM			16	/*  Must be 2^N */
static int flsh_read_cmd(int argc, char *argv[])
{
    unsigned int dst, len;
    int result, pos=0, res, i;
    unsigned char lbuf[MAX_FLASH_READ_LEN];
    
    
    if (( argc == 0 ) || ( sscanf(argv[0],"%x", &dst )!=1)) {
        printf(" Invalid HEX: %s.\n", argv[0]);
		return -1 ;
    }

    if( argc > 1 ) {
        if(sscanf( argv[1], "%x", &len )!=1) {
            printf(" Invalid HEX: %s.\n", argv[1]);
			return -1 ;
	}
    } else
	len = 0x80;
    if (len > MAX_FLASH_READ_LEN)
	len = MAX_FLASH_READ_LEN;

    result=flsh_read(dst, lbuf, len );
    if (result != 0) {
        printf("Flash read err=%d\n", result);
	return result;
    }
    
    res = dst & (PRINT_COL_NUM-1);
    dst &= ~(PRINT_COL_NUM-1);
    if (res) {
        printf("\n%06x:", dst);
	dst += PRINT_COL_NUM;
        for (pos; pos<res; pos++)
	    printf("   ");
    }
    for (i=0; i<len; i++, pos++) {
	if ((pos & (PRINT_COL_NUM-1)) == 0) {
	    printf("\n%06x:", dst);
	    dst += PRINT_COL_NUM;
	}
	printf(" %02x", lbuf[i]);
    }
    printf("\n\n");
    
    return result;
}


static int flsh_write_cmd( int argc, char* argv[])
{
    unsigned int src, dst, len;
    int result;
    
	if (sscanf( argv[0],"%x", &dst )!=1)
	{
        printf(" Invalid Address(HEX) value.\n");
		return -1 ;
	}

	if (sscanf( argv[2],"%x", &src )!=1)
	{
        printf(" Invalid Address(HEX) value.\n");
		return -1 ;
	}

    if (sscanf( argv[1],"%x", &len )!=1)
	{
		printf(" Invalid len value.\n");
		return -1 ;
	}

    printf("flash-wr %08x, len=%08x,from=%08x!",dst,len,src);
    result=flsh_write(dst, src, len );
    printf("result=%08x\n",result);

    return result;
}

static int flsh_erase_cmd(int argc, char* argv[])
{
    unsigned int dst, len;
    int result;
    
	if (( argc == 0 )||( sscanf(argv[0],"%x", &dst )!=1) )
	{
        printf(" Invalid HEX: %s.\n", argv[0]);
		return -1 ;
	}

	if( argc > 1 )
	{
		if(sscanf( argv[1], "%x", &len )!=1)
		{
            printf(" Invalid HEX: %s.\n", argv[1]);
			return -1 ;
		}
	}
	else
	    len = 0x10000 ;

    printf("flsh-earse %08x, len=%08x!",dst,len);
    result=flsh_erase( dst, len );
    printf("result=%08x\n",result);

	return result;
}

#endif	/* CONFIG_CLI_ADV_DBGCMD */

static int eth_set_debug(int argc, char* argv[])
{
	extern unsigned int EthDebugMask;

	printf("==> eth_set_debug *******************\n");

	if (argc == 1 )
	{
		if (!strcmp(argv[0],"tx"))
			if_ra305x_debug("tx");
		else if (!strcmp(argv[0],"rx"))
			if_ra305x_debug("rx");
		else if (sscanf(argv[0], "%x", &EthDebugMask ) != 1)
		{
			printf(" Invalid HEX : %s\n", argv[0]);
			EthDebugMask = 0x0007;
		}

		if (EthDebugMask > 0x0fff)
		{
			printf(" Invalid EthDebugMask : %x\n", EthDebugMask);
			EthDebugMask = 0x0007;
		}
	}
		
	printf("<== eth_set_debug(EthDebugMask = %x)\n", EthDebugMask);
	
	return 0;
}

