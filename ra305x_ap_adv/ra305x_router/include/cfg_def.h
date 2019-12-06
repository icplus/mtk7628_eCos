#ifndef CFG_DEF_H
#define CFG_DEF_H

//#define FW_HEADER   "\0x01\0x000x11\0x04"
#include <cfg_range.h>
#include <cfg_id.h>

#define MON_CMD_NVRAM_UD    0x01   //update nvram 
#define MON_CMD_REBOOT      0x02   //reboot
#define MON_CMD_INIT		0x04	//module initial
#define MON_CMD_STOP		0x08	//module stop
#define MON_CMD_DOWN		0x10   //module has been down
#define MON_CMD_UPDATE		(MON_CMD_INIT|MON_CMD_STOP)	//module update
#define	MON_CMD_LINK_CHK	0x20
#define	MON_CMD_PS		0x40
#define MON_CMD_CFGCHANGE	0x80

#define MOD_LAN			0x00000100
#define MOD_WAN			0x00000200
#define MOD_WL			0x00000400
#define MOD_NAT			0x00000800
#define MOD_FW			0x00001000
#define MOD_UPNP		0x00002000
#define MOD_DHCPD		0x00004000
#define MOD_HTTPD		0x00008000
#define MOD_SYS			0x00010000	
#define MOD_RIP			0x00020000
#define MOD_DDNS		0x00040000
#define MOD_BR			0x00080000
#define MOD_DNS			0x00100000
#define MOD_NTP			0x00200000
#define MOD_SRT			0x01000000
#define MOD_DOT1X		0x02000000
#define MOD_MROUTED		0x04000000
#define	MOD_LOG			0x08000000
//added for VPN use
#define	MOD_VPN			0x10000000

#define MON_CMD_LAN_INIT		((MON_CMD_INIT&0xff)|MOD_LAN)
#define MON_CMD_WAN_INIT		((MON_CMD_INIT&0xff)|MOD_WAN)
#define MON_CMD_WL_INIT			((MON_CMD_INIT&0xff)|MOD_WL)
#define MON_CMD_NAT_INIT		((MON_CMD_INIT&0xff)|MOD_NAT)
#define MON_CMD_FW_INIT			((MON_CMD_INIT&0xff)|MOD_FW)
#define MON_CMD_UPNP_INIT		((MON_CMD_INIT&0xff)|MOD_UPNP)
#define MON_CMD_DHCPD_INIT		((MON_CMD_INIT&0xff)|MOD_DHCPD)
#define MON_CMD_HTTPD_INIT		((MON_CMD_INIT&0xff)|MOD_HTTPD)
#define MON_CMD_SYS_INIT		((MON_CMD_INIT&0xff)|MOD_SYS)
#define MON_CMD_RIP_INIT		((MON_CMD_INIT&0xff)|MOD_RIP)
#define MON_CMD_DDNS_INIT		((MON_CMD_INIT&0xff)|MOD_DDNS)
#define MON_CMD_BR_INIT			((MON_CMD_INIT&0xff)|MOD_BR)
#define MON_CMD_DNS_INIT		((MON_CMD_INIT&0xff)|MOD_DNS)
#define MON_CMD_MROUTED_INIT	((MON_CMD_INIT&0xff)|MOD_MROUTED)
#define MON_CMD_LOG_INIT		((MON_CMD_INIT&0xff)|MOD_LOG)
//added for VPN use
#define MON_CMD_VPN_INIT		((MON_CMD_INIT&0xff)|MOD_VPN)

#define MON_CMD_LAN_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_LAN)
#define MON_CMD_WAN_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_WAN)
#define MON_CMD_WL_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_WL)
#define MON_CMD_NAT_UPDTAE		((MON_CMD_UPDATE&0xff)|MOD_NAT)
#define MON_CMD_FW_UPDTAE		((MON_CMD_UPDATE&0xff)|MOD_FW)
#define MON_CMD_UPNP_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_UPNP)
#define MON_CMD_DHCPD_UPDATE	((MON_CMD_UPDATE&0xff)|MOD_DHCPD)
#define MON_CMD_HTTPD_UPDATE	((MON_CMD_UPDATE&0xff)|MOD_HTTPD)
#define MON_CMD_SYS_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_SYS)
#define MON_CMD_RIP_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_RIP)
#define MON_CMD_DDNS_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_DDNS)
#define MON_CMD_BR_UPDTAE		((MON_CMD_UPDATE&0xff)|MOD_BR)
#define MON_CMD_NTP_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_NTP)
#define MON_CMD_DNS_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_DNS)
#define MON_CMD_MROUTED_UPDATE	((MON_CMD_UPDATE&0xff)|MOD_MROUTED)
#define MON_CMD_LOG_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_LOG)
//added for VPN use
#define MON_CMD_VPN_UPDATE		((MON_CMD_UPDATE&0xff)|MOD_VPN)

#define MON_CMD_LAN_STOP		((MON_CMD_STOP&0xff)|MOD_LAN)
#define MON_CMD_WAN_STOP		((MON_CMD_STOP&0xff)|MOD_WAN)
#define MON_CMD_WL_STOP			((MON_CMD_STOP&0xff)|MOD_WL)
#define MON_CMD_NAT_STOP		((MON_CMD_STOP&0xff)|MOD_NAT)
#define MON_CMD_FW_STOP			((MON_CMD_STOP&0xff)|MOD_FW)
#define MON_CMD_UPNP_STOP		((MON_CMD_STOP&0xff)|MOD_UPNP)
#define MON_CMD_DHCPD_STOP		((MON_CMD_STOP&0xff)|MOD_DHCPD)
#define MON_CMD_HTTPD_STOP		((MON_CMD_STOP&0xff)|MOD_HTTPD)
#define MON_CMD_SYS_STOP		((MON_CMD_STOP&0xff)|MOD_SYS)
#define MON_CMD_RIP_STOP		((MON_CMD_STOP&0xff)|MOD_RIP)
#define MON_CMD_DDNS_STOP		((MON_CMD_STOP&0xff)|MOD_DDNS)
#define MON_CMD_BR_STOP			((MON_CMD_STOP&0xff)|MOD_BR)
#define MON_CMD_DNS_STOP		((MON_CMD_STOP&0xff)|MOD_DNS)
#define MON_CMD_MROUTED_STOP	((MON_CMD_STOP&0xff)|MOD_MROUTED)
#define MON_CMD_LOG_STOP		((MON_CMD_STOP&0xff)|MOD_LOG)
//added for VPN use
#define MON_CMD_VPN_STOP		((MON_CMD_STOP&0xff)|MOD_VPN)

#define MON_CMD_WAN_DOWN		((MON_CMD_DOWN&0xff)|MOD_WAN)


#define CFG_PROFILE     "/profile"
#define CFG_PROFILE_TMP "/profile.tmp"
#define CFG_PROFILE_HEADER "#PROFILE"
#define CFG_PROFILE_VER   "#VER_2_1"
#define CFG_PROFILE_HEADER_LEN 8
#ifndef	CONFIG_WP322X
#define CFG_FW_HEADER { 0x01, 0x00, 0x11, 0x04, 0, 0, 0, 0 };
#else
#define CFG_FW_HEADER { 0x60, 0x00, 0x00, 0x3e, 0x4e, 0x71, 0x4e, 0x71 };
#endif
#define CFG_FW_HEADER_LEN 8
#define CFG_FW_HEADER2 { 0x10, 0x00, 0x0c, 0x08, 0, 0, 0, 0 };
#define	CFG_FW_HEADER3	{ 0x0f, 0x00, 0x00, 0x10, 0, 0, 0, 0 };

#define CFG_PROFILE_LEN_MIN 80
#define CFG_PROFILE_LEN_MAX 30000

int CFG_get(int id, void *val);
int CFG_set(int id, void *val);
int CFG_set_no_event(int id, void *val);
int CFG_get_str(int id, char *val);
int CFG_set_str(int id, char *val);
int CFG_del(int id);
int CFG_load(int id);
int CFG_save(int id);
int CFG_write_prof(char * file, int size);
int CFG_read_prof(char * file, int size);
int CFG_str2id(char * str);
char * CFG_id2str(int id);
int CFG_init_prof(void);
int CFG_commit(int id);
void CFG_get_mac(int id, char *dp);
void CFG_reset_default(void);
int chk_fmw_hdr(char *file, int len);
int mon_snd_cmd(int cmd);

#endif  //CFG_DEF_H


