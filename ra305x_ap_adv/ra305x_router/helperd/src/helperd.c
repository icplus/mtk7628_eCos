/* 
 * This program is designed as a helper for those dirty jobs.
 *													-- Lea
 */
#include "helperd.h"
#include <cyg/kernel/kapi.h>
#include <cyg/fileio/fileio.h>

#include <cfg_def.h>
#include <unistd.h>
#include <stdlib.h>

#define APCLIINTF "apcli0"

cyg_handle_t apcli_init_thread_h = 0;
cyg_thread   apcli_init_thread;
static cyg_uint8 helperd_stack[5120];
static int helperd_running = 0;

void ApCli_init(cyg_addrword_t data);
/*
 * Integrated this program into the init program, 
 * so that we can easily access other threads resources.
 */
int HELPERD_init(void)
{
	if (!helperd_running) {
		int val = 0;

		if (CFG_get(CFG_WLN_ApCliEnable, &val) != -1
				&& val == 1) {
			DEBUG("Trying to run ApCli thread...\n");

			cyg_thread_create(8, /* priority */
					ApCli_init, /* entry function */
					0, /* entry data */
					"apcli_init", /* optional thread name */
					&helperd_stack, /* stack base, NULL = alloc */
					sizeof(helperd_stack), /* stack size, 0 = default */
					&apcli_init_thread_h, /* returned thread handle */
					&apcli_init_thread/* put thread here */
					);
			cyg_thread_resume(apcli_init_thread_h);
		}
		helperd_running++;
	}
}

void ApCli_init(cyg_addrword_t data)
{
	extern int CmdIwpriv(int argc, char *argv[]);
	int val = 0;

	/* Check ApCliAutoConnect */
	if (CFG_get(CFG_WLN_ApCliAutoConnect, &val) != -1
			&& val == 1) {
		char *const args[] = {APCLIINTF, "set", "ApCliAutoConnect=1"};

		DEBUG("Enabling ApCliAutoConnect for %s...\n", APCLIINTF);

		CmdIwpriv(sizeof(args) / sizeof(args[0]), args);
	}

	/* Stop DHCP server */
	if (CFG_get(CFG_SYS_OPMODE, &val) != -1) {
		extern cyg_handle_t dhcpd_thread_h;

		switch (val) {
			case 1:/* Gateway mode */
				/* TODO:
				 *	Stop the DHCP server only if we are really connected, and
				 *	resume the DHCP server while the connection is lost.
				 */
				cyg_thread_suspend(dhcpd_thread_h);
				break;

			case 2:/* ApClient mode */
				break;

			case 3:/* Repeater mode */
				break;

			default:
				WARN("The router is in an unhandled opertion mode...\n");
				break;
		}
	}
}


