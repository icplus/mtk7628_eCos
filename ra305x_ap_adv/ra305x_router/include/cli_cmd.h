/*
	cli_cmd.h
*/
#ifndef	CLI_CMD_H

#include <cyg/infra/diag.h>     // For dial_printf
#define printf      diag_printf

typedef struct _stCLI_cmd
{
	char * name;
    int	(*func)(int, char** );	// the main func , or init 
	char * doc;
	int mode;
	struct _stCLI_cmd * prev;
	struct _stCLI_cmd * next;
} stCLI_cmd;

#define	CLI_CMD_FLAG_HIDE	1		// don't show
#define	CLI_CMD_FLAG_SUBCMD 2 		// this command is a table which has sub command
#define	CLI_CMD_FLAG_NOADDABLE 4  	// cound'nt add new command
#define	CLI_CMD_FLAG_FIRST	8		// this is a container include commands
#define	CLI_CMD_FLAG_INITED	0x10	// inited?

#define CLI_MAX_CMD_LEN 512

#define	CLI_NUM_OF_CMDS(cmd)	(sizeof(cmd)/sizeof(stCLI_cmd))

extern stCLI_cmd sys_cmds[];

#define	CLI_ROOT_CMD	(&sys_cmds)

int CLI_cmd_init();
void CLI_cmd_proc(stCLI_cmd * cmds);
int CLI_add_cmd(stCLI_cmd * cur,  stCLI_cmd * newc);
int CLI_del_cmd(stCLI_cmd * cur,  char * name);
int CLI_init_cmd(stCLI_cmd * cmds,  int size);

void CLI_main(void);

#endif	//CLI_CMD_H


