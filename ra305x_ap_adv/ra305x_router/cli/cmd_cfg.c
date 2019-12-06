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
    cmd_cfg.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#include <cli_cmd.h>
#include <version.h>
#include <stdio.h>
#include <cfg_def.h>

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

static int cfg_cmds_init(int argc, char* argv[]);
static int cfg_set_cmd(int argc, char* argv[]);
static int cfg_get_cmd(int argc, char* argv[]);
static int cfg_del_cmd(int argc, char* argv[]);
static int cfg_prof_cmd(int argc, char* argv[]);
static int cfg_show_prof(char *hint);

stCLI_cmd cfg_cmds[] =
{
	{ "CFG" , &cfg_cmds_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "get" , &cfg_get_cmd, "get/show variable", 0, 0, 0 },
	{ "set" , &cfg_set_cmd, "set variablle value", 0, 0, 0 },
	{ "del" , &cfg_del_cmd, "del variablle", 0, 0, 0 },
	{ "prof" , &cfg_prof_cmd, "profie init/save/commit/show", 0, 0, 0 },
};


static void CHK_RET_CODE(int rc)
{
    switch(rc)
    {
        case CFG_OK:
            printf("OK\n");
            break;
        case CFG_NFOUND:
            printf("Not Found\n");
            break;
        case CFG_ETYPE:
            printf("Type Error\n");
            break;
        case CFG_ERANGE:
            printf("Range Error\n");
            break;
        case CFG_EIDSTR:
            printf("ID Error\n");
            break;
        case CFG_INSERT:
            printf("Insert Done\n");
            break;
        case CFG_CHANGE:
            printf("Change Done\n");
            break;
        default:
            break;
    }
}


static int cfg_cmds_init(int argc, char* argv[])
{
	// test cmds
	return CLI_init_cmd(&cfg_cmds[0], CLI_NUM_OF_CMDS(cfg_cmds) );
}


static int cfg_get_cmd(int argc, char* argv[])
{
	char var[256]="\0\0";
    int id;
    int rc;

	if ( argc < 1 )
	{
        printf("Usuage:   get <name|id>\n" );
        printf("Arguments:\n");
        printf("\tname/id config name or id.\n");
		return -1;
	}
    // to avoid the scanf missing case "Fxxx", "E..." "D..." case
    if(!sscanf( argv[0], "%x",  &id ) || id < 256 )
    {
        rc = CFG_get_str( CFG_str2id(argv[0]), var);
	    printf("%s=%s\n", argv[0], var);
    }
    else
    {
        rc = CFG_get_str( id, var );
        printf("%s=%s\n", CFG_id2str(id), var);
    }

    CHK_RET_CODE(rc);
    return 0;
}

static int cfg_set_cmd(int argc, char* argv[])
{
    int id;
    int rc;
    if (argc < 2)
    {
        printf("Usuage:   set <name|id> <value>\n" );
        printf("Arguments:\n");
        printf("\tname/id config name or id.\n");
        printf("\tvalue   config value.\n");
        return -1;
    }

    if(!sscanf( argv[0], "%x",  &id ) || id < 256 )
    {
	    rc=CFG_set_str( CFG_str2id(argv[0]), argv[1]);
    }
    else
    {
        printf("set %s\n", CFG_id2str(id));
        rc=CFG_set_str( id, argv[1]);
    }

    CHK_RET_CODE(rc);
    return 0;
}

static int cfg_del_cmd(int argc, char* argv[])
{
	if ( argc < 1 )
	{
        printf("Usuage:   del <name>\n" );
        printf("Arguments:\n");
        printf("\tname    config name.\n");
		return -1;
	}

	CHK_RET_CODE(CFG_del( CFG_str2id(argv[0])));
    return 0;
}

static int cfg_prof_cmd(int argc, char* argv[])
{
    if (argc < 1)
    {
show_usage:
        printf("Usuage:   prof <init|save|commit|show>\n" );
        printf("Arguments:\n");
        printf("\tinit    reset factory default.\n");
        printf("\tsave    save config to flash.\n");
        printf("\tcommit  save config to flash and notify monitor thread.\n");
        printf("\tshow    list all availble config.\n");
        return -1;
    }

    if (!strcmp(argv[0],"init"))    //reset to default
        CFG_reset_default();
    else if (!strcmp(argv[0],"save"))
        CFG_save(0);
    else if (!strcmp(argv[0],"commit"))
        CFG_commit(0);
    else if (!strcmp(argv[0],"show"))    //show profile
    {
    	if (argc > 1)
    		cfg_show_prof(argv[1]);
    	else
    		cfg_show_prof(NULL);
    }
    else
        goto show_usage;

    return 0;
}


static int cfg_show_prof(char *hint)
{
	char * fb, *sp, *ep;
	int size, line_cnt, hint_len = 0;

	if ((fb = malloc(CFG_PROFILE_LEN_MAX))<=0)
	{
		printf("error alloc buf!\n");
		return -1;
	}

	if ((size=CFG_write_prof(fb, CFG_PROFILE_LEN_MAX))<=0)
	{
		printf("error profile!\n");
		free(fb);
		return -1;
	}
	line_cnt = 0;
	if (hint != NULL)
		hint_len = strlen(hint);

	for (sp=fb;(ep=strchr(sp, 0x0a)) && (ep < (fb+size)); sp=ep+1)
    {
		*ep=0;
		if (line_cnt < 2)
        {
			/*  Show profile header  */
			printf("%s\n", sp);
			line_cnt++;
		}
		else
        {
			if (hint != NULL && strncmp(hint, sp, hint_len) != 0)
				continue;

			/*  Show matched items  */
			printf("%s\n", sp);
			line_cnt ++;
		}
	}
	if (line_cnt <= 2)
		printf("\nNo entry found!\n");

	free(fb);
	return 0;
}

