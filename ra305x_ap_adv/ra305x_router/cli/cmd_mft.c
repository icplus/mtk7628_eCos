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
    cmd_mft.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================
#include <pkgconf/system.h>
#include <network.h>
#include <netinet/if_ether.h>

#include <cli_cmd.h>
#include <version.h>
#include <stdio.h>
#include <cfg_def.h>
#include <cfg_id.h>
#include <flash.h>

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

stCLI_cmd mft_cmds[]; 

static	int mft_cmds_init(int argc, char* argv[]);
static	int mft_fmwver_cmd(int argc, char* argv[]);
static	int mft_hwmac_cmd(int argc, char* argv[]);
static	int mft_bootcfg_cmd(int argc, char* argv[]);


#ifdef CONFIG_CLI_MFT_CMD
#define CLI_MFTCMD_FLAG			0
#else
#define CLI_MFTCMD_FLAG			CLI_CMD_FLAG_HIDE
#endif

stCLI_cmd mft_cmds[] =
{
	{ "MFT" , &mft_cmds_init, 0, (CLI_CMD_FLAG_HIDE|CLI_CMD_FLAG_FIRST), CLI_ROOT_CMD, 0 },
	{ "fmwver" , &mft_fmwver_cmd, "show firmware version", 0, 0, 0 },
	{ "bootcfg", &mft_bootcfg_cmd, "boot configuration", 0, 0, 0 },
	{ "hwmac", &mft_hwmac_cmd, "hardware mac cmd", CLI_MFTCMD_FLAG, 0, 0 },
};

static int mft_cmds_init(int argc, char* argv[])
{
	// test cmds
	CLI_init_cmd(&mft_cmds[0], CLI_NUM_OF_CMDS(mft_cmds) );
	return 0;
}


static int mft_fmwver_cmd(int argc, char* argv[])
{
#if 0 /* modified by Rorscha */
#ifndef	CONFIG_CUST_VER_STR
	char val[255];
	
	CFG_get_str(CFG_STS_FMW_VER, val);
	printf("%s\n", val);
#else
	printf("%s\n", SYS_CUST_VER_STR);
#endif
#else
	printf("%s\n", SYS_CUST_VER_STR);
#endif

	return 0;
}


//***********************
//* Boot Configuration
//***********************
#define IP_ADDR(a,b,c,d)	((a<<24)|(b<<16)|(c<<8)|(d))

struct boot_cfg
{
	cfgid	cid;
	void	*parm;
	void	*def;
};

struct	boot_parm
{
	int	hw_ver;
	int	serial;
	int app;
	int vlan;
	int	intf;
	unsigned int load_addr;
	unsigned int load_sz;
	unsigned int load_src;
	unsigned int my_ip;
	unsigned int net_mask;
	unsigned int gw_ip;
	unsigned int server_ip;
	char fname[32];
	int boot_ver;
	unsigned char lanmac[6];
	unsigned char wanmac[6];
	unsigned char wlmac[6];
};

static	struct boot_parm BootParm;
static	struct boot_parm BootDef = 
{
	IP_ADDR(1,2,3,4),
	12345678,
	2,
	0,
	1,
	0x80500000,
	0x200000,
	0x50000,
	IP_ADDR(10,10,10,1),
	IP_ADDR(255,255,255,0),
	IP_ADDR(10,10,10,100),
	IP_ADDR(10,10,10,100),
	{"zxrouter.img"},
	-1,
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};

static	struct boot_cfg BootCfg[] =
{
	{CFG_HW_VER,	&BootParm.hw_ver,	&BootDef.hw_ver},
	{CFG_HW_SN,		&BootParm.serial,	&BootDef.serial},
	{CFG_HW_APP,	&BootParm.app,		&BootDef.app},
	{CFG_HW_VLAN,	&BootParm.vlan,		&BootDef.vlan},
	{CFG_HW_INTF,	&BootParm.intf,		&BootDef.intf},
	{CFG_HW_RUN_LOC,&BootParm.load_addr,&BootDef.load_addr},
	{CFG_HW_IMG_SZ,	&BootParm.load_sz,	&BootDef.load_sz},
	{CFG_HW_IMG_LOC,&BootParm.load_src,	&BootDef.load_src},
	{CFG_HW_IP,		&BootParm.my_ip,	&BootDef.my_ip},
	{CFG_HW_MSK,	&BootParm.net_mask,	&BootDef.net_mask},
	{CFG_HW_GW,		&BootParm.gw_ip,	&BootDef.gw_ip},
	{CFG_HW_SVR,	&BootParm.server_ip,&BootDef.server_ip},
	{CFG_HW_NAME,	&BootParm.fname,	&BootDef.fname},
	{CFG_HW_BOT_VER,&BootParm.boot_ver,	&BootDef.boot_ver},
	{CFG_HW_MAC+1,	&BootParm.lanmac,	&BootDef.lanmac},
	{CFG_HW_MAC+2,	&BootParm.wanmac,	&BootDef.wanmac},
	{CFG_HW_MAC+3,	&BootParm.wlmac,	&BootDef.wlmac},
	{-1, 0, 0}
};

extern unsigned int flsh_cfg_boot_off;
#define BOOT_CFG_BUF_SZ		1024
static void read_bootcfg(void)
{
	struct boot_cfg *cfg;
	cfgid cid;
	unsigned *lbuf;
	int err;
	
	if ((lbuf = malloc(BOOT_CFG_BUF_SZ)) == NULL) {
	    printf("Read bootcfg failed: nobuf\n");
	    return;
	}
	if ((err = flsh_read(flsh_cfg_boot_off, lbuf, BOOT_CFG_BUF_SZ)) != 0) {
		printf("Read bootcfg err=%d\n", err);
		goto readcfg_err;
	}
	for (cfg=BootCfg; cfg->parm; cfg++)
	{
		cid = cfg->cid;
		
		// Special take care with boot version
		if (cid == CFG_HW_BOT_VER)
		{
			char *p;
			
			p = (char *)(lbuf + 8);
			if (strcmp(p, "bootver") == 0)
			{
				memcpy(cfg->parm, p+8, 4);
				continue;
			}
		}
		
		// Take care of other CID
		if (CFG_get(cid, cfg->parm) < 0)
		{
			switch (CFG_ID_TYPE(cid))
			{
			case CFG_TYPE_MAC:
				memcpy(cfg->parm, cfg->def, 6);
				break;
				
			case CFG_TYPE_IP:
			case CFG_TYPE_INT:
				memcpy(cfg->parm, cfg->def, 4);
				break;
				
			case CFG_TYPE_STR:
				strcpy((char *)cfg->parm, (char *)cfg->def);
				break;
			}
		}
	}
	
	cfg++;
	
readcfg_err:

	if (lbuf != NULL)
	    free(lbuf);
}

static void write_bootcfg(void)
{
	struct boot_cfg *cfg = BootCfg;
	cfgid cid;
	
	for (cfg=BootCfg; cfg->parm; cfg++)
	{
		cid = cfg->cid;
		
		// Write to boot configuration
		CFG_set(cid, cfg->parm);
	}
}

static int mft_bootcfg_cmd(int argc, char* argv[])
{
	struct boot_cfg *cfg;
	cfgid cid;
	
	// Make boot configuration
	read_bootcfg();
	
	for (cfg=BootCfg; cfg->parm; cfg++)
	{
		cid = cfg->cid;
		
		// Print out;
		printf("%s=", CFG_id2str(cid));
		
        switch (CFG_ID_TYPE(cid))
		{
		case CFG_TYPE_MAC:
			printf("%s\n", ether_ntoa(cfg->parm));
			break;
			
		case CFG_TYPE_IP:
			printf("%s\n", inet_ntoa(*(struct in_addr *)cfg->parm));
			break;
		
		case CFG_TYPE_INT:
			printf("%lx\n", *(unsigned long *)(cfg->parm));
			break;
			
		case CFG_TYPE_STR:
		default:
			printf("%s\n", (char *)cfg->parm);
			break;
		}
	}
	
	return 0;
}

//*********************
// MAC address
//*********************
static int mft_hwmac_cmd(int argc, char* argv[])
{
	unsigned char *mac;
	unsigned char lanmac[6];
	unsigned char wanmac[6];
	
	if (argc == 0)
	{
		memset(lanmac, 0xff, 6);
		memset(wanmac, 0xff, 6);
		
		CFG_get(CFG_HW_MAC+1, lanmac);
		CFG_get(CFG_HW_MAC+2, wanmac);
		
		printf("LANMAC : %s \n", ether_ntoa((struct ether_addr *)lanmac));
		printf("WANMAC : %s \n", ether_ntoa((struct ether_addr *)wanmac));
		return 0;
	}
	else if (argc == 2)
	{
		// Process lanmac
		mac = (char *)ether_aton(argv[0]);
		if (mac == 0)
			goto usage;
		
		memcpy(lanmac, mac, 6);
		
		// Process wanmac
		mac = (char *)ether_aton(argv[1]);
		if (mac == 0)
			goto usage;
		
		memcpy(wanmac, mac, 6);
		
		// Make configuration
		read_bootcfg();
		
		// Update the new mac
		memcpy(BootParm.lanmac, lanmac, 6);
		memcpy(BootParm.wanmac, wanmac, 6);
		
		// Now write configuration
		write_bootcfg();
		
		printf("\n");
		printf("LANMAC is set to %s \n", ether_ntoa((struct ether_addr *)lanmac));
		printf("WANMAC is set to %s \n", ether_ntoa((struct ether_addr *)wanmac));
		return 0;
	}
	
usage:
	printf("Usage: hwmac [lanmac wanwan]\n");
	printf("       MAC format xx:xx:xx:xx:xx:xx\n");
	return -1;
}

