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
    cli main entry

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#include <cyg/kernel/kapi.h>
#include <cli_cmd.h>

//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

extern stCLI_cmd  os_cmds[];
extern stCLI_cmd net_cmds[];
extern stCLI_cmd cfg_cmds[];
extern stCLI_cmd dbg_cmds[];


//#ifdef CONFIG_MFT
#if 1
extern stCLI_cmd mft_cmds[];
#endif

// local
static int sys_cmd_init(int argc, char* argv[]);

stCLI_cmd sys_cmds[] =
{
	{ "CMD" , &sys_cmd_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), 0, 0 },
#ifdef CONFIG_CLI_CFG_CMD 
	{ "cfg" , &cfg_cmds, "CFG cmds", CLI_CMD_FLAG_SUBCMD, CLI_ROOT_CMD, 0 },
#endif
#ifdef CONFIG_CLI_NET_CMD 
	{ "net" , &net_cmds, "NET cmds", CLI_CMD_FLAG_SUBCMD, CLI_ROOT_CMD, 0 },
#endif
#ifdef CONFIG_CLI_OS_CMD 
	{ "os" , &os_cmds, "RTOS cmds", CLI_CMD_FLAG_SUBCMD, CLI_ROOT_CMD, 0 },
#endif
#ifdef CONFIG_CLI_DBG_CMD 
	{ "dbg" , &dbg_cmds, "DBG cmds", CLI_CMD_FLAG_SUBCMD, CLI_ROOT_CMD, 0 },
#endif
#ifdef CONFIG_CLI_MFT_CMD 	
	{ "mft" , &mft_cmds, "MFT cmds", CLI_CMD_FLAG_SUBCMD, CLI_ROOT_CMD, 0 },
#endif
};

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
static int sys_cmd_init(int argc, char* argv[])
{
	// default cmds
	CLI_init_cmd(&sys_cmds[0], CLI_NUM_OF_CMDS(sys_cmds) );	

	return 0;
}

static cyg_handle_t cli_thread_h;
static cyg_thread cli_thread;
static char cli_stack[4096*2];


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

void CLI_start(int cmd)
{
    if (cmd)
    {
        if (CLI_cmd_init() == 0)
        {
            cyg_thread_create(8, (cyg_thread_entry_t *)CLI_cmd_proc, (cyg_addrword_t) &sys_cmds[0], "CLI_thread",
                          (void *)&cli_stack[0], sizeof(cli_stack), &cli_thread_h, &cli_thread);
            cyg_thread_resume(cli_thread_h);
        }
    }
}



