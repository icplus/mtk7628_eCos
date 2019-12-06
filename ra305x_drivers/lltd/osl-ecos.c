/*
 * LICENSE NOTICE.
 *
 * Use of the Microsoft Windows Rally Development Kit is covered under
 * the Microsoft Windows Rally Development Kit License Agreement,
 * which is provided within the Microsoft Windows Rally Development
 * Kit or at http://www.microsoft.com/whdc/rally/rallykit.mspx. If you
 * want a license from Microsoft to use the software in the Microsoft
 * Windows Rally Development Kit, you must (1) complete the designated
 * "licensee" information in the Windows Rally Development Kit License
 * Agreement, and (2) sign and return the Agreement AS IS to Microsoft
 * at the address provided in the Agreement.
 */

/*
 * Copyright (c) Microsoft Corporation 2005.  All rights reserved.
 * This software is provided with NO WARRANTY.
 */

#include "globals.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <cyg/kernel/kapi.h>
#include <network.h>
#include <sys/types.h>

#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <ifaddrs.h>
#include <cyg/io/eth/eth_drv_stats.h>
#include <cyg/io/eth/eth_drv.h>
#include <cyg/io/eth/netdev.h>

#include <pkgconf/system.h>
#include <pkgconf/net.h>
#include <pkgconf/isoinfra.h>

#ifdef HAVE_CAPABILITIES
# include <sys/capability.h>
#endif

#define PF_PACKET PF_INET
#define AF_PACKET AF_INET
#define ECOS_AP_MODULE_NAME "eCos_AP"
typedef struct eth_drv_sc	*PNET_DEV;

#define MemPool_TYPE_Header 3
#define MemPool_TYPE_CLUSTER 4
#define BUFFER_HEAD_RESVERD 64

//extern	int mbtypes[];			/* XXX */
extern  struct	mbstat mbstat;
extern  union	mcluster *mclfree;
extern	char *mclrefcnt;

extern int icon_size;
extern int rt28xx_ap_ioctl(
	PNET_DEV	pEndDev, 
	int			cmd, 
	caddr_t		pData);

/* Do you have wireless extensions available? (most modern kernels do) */
#define HAVE_WIRELESS

#include <net/if.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "packetio.h"

/* helper functions */

/* Convert from name "interface" to its index, or die on error. */
static int
if_get_index(int sock, char *interface)
{
	return 0;
}


osl_t *
osl_init(void)
{
    osl_t *osl;

    osl = malloc(sizeof(*osl));
    osl->sock = -1;
    osl->wireless_if = g_wl_interface;
    return osl;
}



/* pidfile maintenance: this is not locking (there's plenty of scope
 * for races here!) but it should stop most accidental runs of two
 * lld2d instances on the same interface.  We open the pidfile for
 * read: if it exists and the named pid is running we abort ourselves.
 * Otherwise we reopen the pidfile for write and log our pid to it. */
void
osl_write_pidfile(osl_t *osl)
{
	return;
}


/* Open "interface", and add packetio_recv_handler(state) as the IO
 * event handler for its packets (or die on failure).  If possible,
 * the OSL should try to set the OS to filter packets so just frames
 * with ethertype == topology protocol are received, but if not the
 * packetio_recv_handler will filter appropriately, so providing more
 * frames than necessary is safe. */

void
osl_interface_open(osl_t *osl, char *interface, void *state)
{

    osl->responder_if = interface;
    osl->sock = socket(AF_INET,SOCK_DGRAM,0);
	if(osl->sock<0)
		diag_printf("*******socket create error********\n");
	else
		diag_printf("create sock ok\n");

}

/* Permanently drop elevated privilleges. */
/* Actually, drop all but CAP_NET_ADMIN rights, so we can still enable
 * and disable promiscuous mode, and listen to ARPs. */
void
osl_drop_privs(osl_t *osl)
{
#ifdef HAVE_CAPABILITIES
    cap_t caps = cap_init();
    cap_value_t netadmin[] = {CAP_NET_ADMIN};

    if (!caps)
	die("osl_drop_privs: cap_init failed: %s\n", strerror(errno));
    if (cap_set_flag(caps, CAP_PERMITTED, 1, netadmin, CAP_SET) < 0)
	die("osl_drop_privs: cap_set_flag (permitted) %s\n", strerror(errno));
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, netadmin, CAP_SET) < 0)
	die("osl_drop_privs: cap_set_flag (effective): %s\n", strerror(errno));
    if (cap_set_proc(caps) < 0)
	die("osl_drop_privs: cap_set_proc: %s\n", strerror(errno));
    cap_free(caps);
#endif
}


/* Turn promiscuous mode on or off */
void
osl_set_promisc(osl_t *osl, bool_t promisc)
{
    struct ifreq req;
    int ret=0;
    int rqfd;


    /* we query to get the previous state of the flags so as not to
     * change any others by accident */
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, osl->responder_if, sizeof(req.ifr_name)-1);
    ret = ioctl(osl->sock, SIOCGIFFLAGS, &req);
    if (ret != 0)
	die("osl_set_promisc: couldn't get interface flags for %s: %s\n",
	    osl->responder_if, strerror(errno));

    /* now clear (and optionally set) the IFF_PROMISC bit */
    req.ifr_flags &= ~IFF_PROMISC;
    if (promisc)
	req.ifr_flags |= IFF_PROMISC;
	ret = ioctl(osl->sock, SIOCSIFFLAGS, &req);
      if (ret != 0)
 	die("osl_set_promisc: couldn't set interface flags for %s: %s\n",
	    osl->responder_if, strerror(errno));
}



/* Return the Ethernet address for "interface", or FALSE on error. */
bool_t
get_hwaddr(const char *interface, /*OUT*/ etheraddr_t *addr,
	   bool_t expect_ethernet)
{
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;
	char buf[ETH_ALEN];
	int len=sizeof(buf);

	int rqfd;
    	struct ifreq req;
    	int ret;
	int one = 1;
	memset(buf,0x00,ETH_ALEN);

	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, interface) == 0) {
			found =1;
			break;
	        }
	}

	if ( found == 1 )
	{
		if(IS_RLAINK(interface))
		{
			if((LLTP_RT_ioctl(RT_PRIV_IOCTL, buf, len, interface,RT_OID_802_11_BSSID)) != 0)
			{
				DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_bssid from driver(len=%d, ifname=%s)\n", len,g_osl->wireless_if);
				return TLV_GET_FAILED;
			}
			else
			{
				memcpy(addr,buf,ETH_ALEN);
				return TLV_GET_SUCCEEDED;
			}
		}
		else
		{
			if_ra305x_t *pifra305x;
			pifra305x = (if_ra305x_t *) sc->driver_private;
			memcpy(addr,pifra305x->macaddr,ETH_ALEN);

		}
			
	}
    return TRUE;
}

/* Return the Ethernet address for socket "sock", or die. */
void
osl_get_hwaddr(osl_t *osl, /*OUT*/ etheraddr_t *addr)
{
    if (!get_hwaddr(osl->responder_if, addr, TRUE))
	die("osl_get_hw_addr: expected an ethernet address on our interface\n");

}


ssize_t
osl_read(int fd, void *buf, size_t count)
{
    //return read(fd, buf, count);
}

ssize_t
osl_write(osl_t *osl, const void *buf, size_t count)
{
    		LLTD_EcosSendToRaw(CONFIG_DEFAULT_LAN_NAME, (char*)buf, count);
		LLTD_EcosSendToRaw(CONFIG_DEFAULT_WAN_NAME,  (char*)buf, count);
		LLTD_EcosSendToRaw(CONFIG_WIRELESS_NAME,  (char*)buf, count);
    		return count;
}



/* TRUE if x is less than y (lexographically) */
static bool_t
etheraddr_lt(const etheraddr_t *x, const etheraddr_t *y)
{
    int i;

    for (i=0; i<6; i++)
    {
	if (x->a[i] > y->a[i])
	    return FALSE;
	else if (x->a[i] < y->a[i])
	    return TRUE;
    }

    return FALSE; /* they're equal */
}

/* Find hardware address of "interface" and if it's lower than
 * "lowest", update lowest. */
static void
pick_lowest_address(const char *interface, void *state)
{
    etheraddr_t *lowest = state;
    etheraddr_t candidate;

    /* query the interface's hardware address */
    if (!get_hwaddr(interface, &candidate, FALSE))
	return; /* failure; just ignore (maybe it's not an Ethernet NIC, eg loopback) */

    if (etheraddr_lt(&candidate, lowest))
	*lowest = candidate;
}

typedef void (*foreach_interface_fn)(const char *interface, void *state);


static bool_t
foreach_interface(foreach_interface_fn fn, void *state)
{
	fn(CONFIG_DEFAULT_LAN_NAME, state);
	return TRUE;
}
/* enumerate all interfaces on this host via /proc/net/dev */

/* Recipe from section 1.7 of the Unix Programming FAQ */
/* http://www.erlenstar.demon.co.uk/unix/faq_toc.html */
void
osl_become_daemon(osl_t *osl)
{
	return;
}


/************************* E X T E R N A L S   f o r   T L V _ G E T _ F N s ***************************/
#define TLVDEF(_type, _name, _repeat, _number, _access, _inHello) \
extern int get_ ## _name (void *data);
#include "tlvdef.h"
#undef TLVDEF


/********************** T L V _ G E T _ F N s ******************************/

/* Return a 6-byte identifier which will be unique and consistent for
 * this host, across all of its interfaces.  Here we enumerate all the
 * interfaces that are up and have assigned Ethernet addresses, and
 * pick the lowest of them to represent this host.  */
int
get_hostid(void *data)
{
/*    TLVDEF( etheraddr_t, hostid,              ,   1,  Access_unset ) */

    etheraddr_t *hid = (etheraddr_t*) data;

    memcpy(hid,&Etheraddr_broadcast,sizeof(etheraddr_t));

    if (!foreach_interface(pick_lowest_address, hid))
    { 
	    dbgprintf("get_hostid(): FAILED picking lowest address.\n");
	return TLV_GET_FAILED;
    }
    if (ETHERADDR_EQUALS(hid, &Etheraddr_broadcast))
    { 
	    dbgprintf("get_hostid(): FAILED, because chosen hostid = broadcast address\n");
	return TLV_GET_FAILED;
    } else { 
    	IF_TRACED(TRC_TLVINFO)
	    dbgprintf("get_hostid: hostid=" ETHERADDR_FMT "\n", ETHERADDR_PRINT(hid));
	END_DEBUG
	return TLV_GET_SUCCEEDED;
    }
}


int
get_net_flags(void *data)
{
/*    TLVDEF( uint32_t,         net_flags,           ,   2,  Access_unset ) */

    uint32_t   nf;

#define fLP  0x08000000         // Looping back outbound packets, if set
#define fMW  0x10000000         // Has management web page accessible via HTTP, if set
#define fFD  0x20000000         // Full Duplex if set
#define fNX  0x40000000         // NAT private-side if set
#define fNP  0x80000000         // NAT public-side if set

    /* If your device has a management page at the url
            http://<device-ip-address>/
       then use the fMW flag, otherwise, remove it */
    nf = htonl(fFD | fMW);

    memcpy(data,&nf,4);

    return TLV_GET_SUCCEEDED;
}


int
get_physical_medium(void *data)
{
/*    TLVDEF( uint32_t,         physical_medium,     ,   3,  Access_unset ) */

    uint32_t   pm;

    pm = htonl(6);	// "ethernet"

    memcpy(data,&pm,4);

    return TLV_GET_SUCCEEDED;
}

#ifdef HAVE_WIRELESS

/* Return TRUE on successful query, setting "wireless_mode" to:
 *  0 = ad-hoc mode
 *  1 = infrastructure mode ie ESS (Extended Service Set) mode
 *  2 = auto/unknown (device may auto-switch between ad-hoc and infrastructure modes)
 */
int
get_wireless_mode(void *data)
{
/*    TLVDEF( uint32_t,         wireless_mode,       ,   4,  Access_unset ) */

    uint32_t* wl_mode = (uint32_t*) data;
    *wl_mode = 1;
	return TLV_GET_SUCCEEDED;
}


/* If the interface is 802.11 wireless in infrastructure mode, and it
 * has successfully associated to an AP (Access Point) then it will
 * have a BSSID (Basic Service Set Identifier), which is usually the
 * AP's MAC address. 
 * Of course, this code resides in an AP, so we just return the AP's
 * BSSID. */
int
get_bssid(void *data)
{
/*    TLVDEF( etheraddr_t, bssid,               ,   5,  Access_unset ) */
	etheraddr_t* bssid = (etheraddr_t*) data;
	char buf[6];
	int len=sizeof(buf);
	memset(buf, 0, sizeof(buf));

	if((LLTP_RT_ioctl(RT_PRIV_IOCTL, buf, sizeof(buf),g_osl->wireless_if,RT_OID_802_11_BSSID)) != 0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_bssid from driver(len=%d, ifname=%s)\n", len,g_osl->wireless_if);
		return TLV_GET_FAILED;
	}
	else
	{
		memcpy(bssid,buf,len);
		return TLV_GET_SUCCEEDED;
	}
}


int
get_ssid(void *data)
{
/*    TLVDEF( ssid_t,           ssid,                ,   6,  Access_unset ) */
	ssid_t* ssid = (ssid_t*)data;
	ssid_t buf;
	memset(&buf,0x00,sizeof(buf));
	memset(buf.Ssid,0x00,32);

	if((LLTP_RT_ioctl(RT_PRIV_IOCTL, &buf, sizeof(buf),g_osl->wireless_if,OID_802_11_SSID)) != 0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_ssid from driver(len=%d, ifname=%s)\n", sizeof(buf),g_osl->wireless_if);
		return TLV_GET_FAILED;
	}
	else
	{
    		memcpy(ssid->Ssid,buf.Ssid, buf.SsidLength);
   	 	ssid->SsidLength= buf.SsidLength;
		IF_TRACED(TRC_TLVINFO)
       	 diag_printf("get_ssid(): ssid=%s len=%d\n", ssid->Ssid,ssid->SsidLength);
		END_TRACE
		return TLV_GET_SUCCEEDED;
	}
}

#endif /* HAVE_WIRELESS */


int
get_ipv4addr(void *data)
{
/*    TLVDEF( ipv4addr_t,       ipv4addr,            ,   7,  Access_unset ) */
    int          rqfd, ret;
    struct ifreq req;
    ipv4addr_t*  ipv4addr = (ipv4addr_t*) data;
    char         dflt_if[] = {"br0"};
    char        *interface = g_interface;

    if (interface == NULL) interface = dflt_if;
    memset(&req, 0, sizeof(req));
    strncpy(req.ifr_name, interface, sizeof(req.ifr_name));
    rqfd = socket(AF_INET,SOCK_DGRAM,0);
    if (rqfd<0)
    {
        warn("get_ipv4addr: FAILED creating request socket for \'%s\' : %s\n",interface,strerror(errno));
        return TLV_GET_FAILED;
    }
    ret = ioctl(rqfd, SIOCGIFADDR, &req);
    if (ret < 0)
    {
	warn("get_ipv4addr(%d,%s): FAILED : %s\n", rqfd, interface, strerror(errno));
	return TLV_GET_FAILED;
    }
	if (rqfd > 0)
    		close(rqfd);
    IF_TRACED(TRC_TLVINFO)
        /* Interestingly, the ioctl to get the ipv4 address returns that
           address offset by 2 bytes into the buf. Leaving this in case
           one of the ports results in a different offset... */
        unsigned char *d;
        d = (unsigned char*)req.ifr_addr.sa_data;
        /* Dump out the whole 14-byte buffer */
        diag_printf("get_ipv4addr: sa_data = 0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x-0x%02x\n",d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9],d[10],d[11],d[12],d[13]);
    END_DEBUG

    memcpy(ipv4addr,&req.ifr_addr.sa_data[2],sizeof(ipv4addr_t));
    IF_TRACED(TRC_TLVINFO)
        diag_printf("get_ipv4addr: returning %d.%d.%d.%d as ipv4addr\n", \
                *((uint8_t*)ipv4addr),*((uint8_t*)ipv4addr +1), \
                *((uint8_t*)ipv4addr +2),*((uint8_t*)ipv4addr +3));
    END_DEBUG

    return TLV_GET_SUCCEEDED;
}


int
get_ipv6addr(void *data)
{
/*    TLVDEF( ipv6addr_t,       ipv6addr,            ,   8,  Access_unset ) */

    ipv6addr_t* ipv6addr = (ipv6addr_t*) data;

    return TLV_GET_FAILED;
}


int
get_max_op_rate(void *data)
{
/*    TLVDEF( uint16_t,         max_op_rate,         ,   9,  Access_unset ) units of 0.5 Mbs */

    uint16_t	morate;

    morate = htons(108);	/* (OpenWRT, 802.11g) 54 Mbs / 0.5 Mbs = 108 */
    memcpy(data, &morate, 2);

    return TLV_GET_SUCCEEDED;
}


int
get_tsc_ticks_per_sec(void *data)
{
/*    TLVDEF( uint64_t,         tsc_ticks_per_sec,   , 0xA,  Access_unset ) */

    uint64_t   tick;

    tick = (uint64_t) 0xF4240LL;	// 1M (1us) ticks - YMMV

    cpy_hton64(data, &tick);

    return TLV_GET_SUCCEEDED;
}


int
get_link_speed(void *data)
{
/*    TLVDEF( uint32_t,         link_speed,          , 0xC,  Access_unset ) // 100bps increments */

    uint32_t   speed;

    /* OpenWRT:
     * Since this is a bridged pair of interfaces (br0 = vlan0 + eth1), I am returning the
     * wireless speed (eth1), which is the lowest of the upper limits on the two interfaces... */

    speed = htonl(540000);  // 54Mbit wireless... (540k x 100 = 54Mbs)

    memcpy(data, &speed, 4);

    return TLV_GET_SUCCEEDED;
}


/* Unless this box is used as a wireless client, RSSI is meaningless - which client should be measured? */
int
get_rssi(void *data)
{
/*    TLVDEF( uint32_t,         rssi,                , 0xD,  Access_unset ) */

    return TLV_GET_FAILED;
}


int
get_icon_image(void *data)
{
/*    TLVDEF( icon_file_t,      icon_image,   , 0xE )	// Usually LTLV style (Windows .ico file > 1k long) */
	return TLV_GET_FAILED;
}


int
get_machine_name(void *data)
{
/*    TLVDEF( ucs2char_t,       machine_name,    [16], 0xF,  Access_unset ) */

    ucs2char_t* name = (ucs2char_t*) data;
    int ret;
    char unamebuf[]=ECOS_AP_MODULE_NAME;

    /* use uname() to get the system's hostname */
    ret =0 ;
    if (ret != -1)
    {
        /* because I am a fool, and can't remember how to refer to the sizeof a structure
         * element directly, there is an unnecessary declaration here... */
        tlv_info_t* fool;
        size_t  namelen;

        namelen = strlen(unamebuf);

	util_copy_ascii_to_ucs2(name, sizeof(fool->machine_name), unamebuf);
			
        IF_TRACED(TRC_TLVINFO)
            dbgprintf("get_machine_name(): space=" FMT_SIZET ", len=" FMT_SIZET ", machine name = %s\n",
                       sizeof(fool->machine_name), namelen, unamebuf);
        END_TRACE
        return TLV_GET_SUCCEEDED;
    }

    IF_TRACED(TRC_TLVINFO)
        dbgprintf("get_machine_name(): uname() FAILED, returning %d\n", ret);
    END_TRACE

    return TLV_GET_FAILED;
}


int
get_support_info(void *data)
{
/*    TLVDEF( ucs2char_t,       support_info,    [32], 0x10, Access_unset ) // RLS: was "contact_info" */

//    ucs2char_t * support = (ucs2char_t*) data;

    return TLV_GET_FAILED;
}


int
get_friendly_name(void *data)
{
/*    TLVDEF( ucs2char_t,       friendly_name,   [32], 0x11, Access_unset ) */

//    ucs2char_t * fname = (ucs2char_t*) data;

    return TLV_GET_FAILED;
}


int
get_upnp_uuid(void *data)
{
/*    TLVDEF( uuid_t,           upnp_uuid,           , 0x12, Access_unset ) // 16 bytes long */

//    uuid_t* uuid = (uuid_t*) data;

    return TLV_GET_FAILED;
}


int
get_hw_id(void *data)
{
/*    TLVDEF( ucs2char_t,       hw_id,          [200], 0x13, Access_unset ) // 400 bytes long, max */

//    ucs2char_t * hwid = (ucs2char_t*) data;

    return TLV_GET_FAILED;
}


int
get_qos_flags(void *data)
{
/*    TLVDEF( uint32_t,         qos_flags,           , 0x14, Access_unset ) */
    int32_t qosf;

#define  TAG8P	0x20000000      // supports 802.1p priority tagging
#define  TAG8Q  0x40000000      // supports 802.1q VLAN tagging
#define  QW     0x80000000      // has Microsoft qWave service implemented; this is not LLTD QoS extensions

    qosf = htonl(0);

    memcpy(data,&qosf,4);

    return TLV_GET_SUCCEEDED;
}


int
get_wl_physical_medium(void *data)
{
/*    TLVDEF( uint8_t,          wl_physical_medium,  , 0x15, Access_unset ) */
	uint8_t* wpm = (uint8_t*) data;
	unsigned char mode;

	memset(&mode,0x00,sizeof(mode));

	if((LLTP_RT_ioctl(RT_PRIV_IOCTL, &mode, sizeof(mode),g_osl->wireless_if,RT_OID_GET_PHY_MODE)) != 0)
	{
		DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_ssid from driver(len=%d, ifname=%s)\n", sizeof(mode),g_osl->wireless_if);
		return TLV_GET_FAILED;
	}
	else
	{
		    switch (mode) {
		    case PHY_11A:
		    case PHY_11ABG_MIXED:
		    case PHY_11ABGN_MIXED:
		    case PHY_11AN_MIXED:
		    case PHY_11AGN_MIXED:
		    case PHY_11N_5G:
			    *wpm = WPM_OFDM_5G;
		        break;

		    case PHY_11B:
		    case PHY_11G:
		    case PHY_11BG_MIXED:
		    case PHY_11GN_MIXED:
		    case PHY_11BGN_MIXED:
			    *wpm = WPM_DSSS_24G;
		        break;

		    default:
		    	/* this is a wireless device that is probably acting
		    	 * as a listen-only monitor; we can't express its features like
		    	 * this, so we'll not return a wireless mode TLV. */
			    return TLV_GET_FAILED;
		    }
		return TLV_GET_SUCCEEDED;
	}
	

    return TLV_GET_SUCCEEDED;
}


int
get_accesspt_assns(void *data)
{
/*    TLVDEF( assns_t,           accesspt_assns,      , 0x16, Access_unset ) // RLS: Large_TLV */

	/* NOTE: This routine depends implicitly upon the data area being zero'd initially. */
	assns_t* assns = (assns_t*) data;    
	struct timeval              now;

	if(assns == NULL) {
		diag_printf("get_accesspt_assns assns_null\n");
		return TLV_GET_FAILED;
	}
	
	if (assns->table == NULL)
	{
		/* generate the associations-table, with a maximum size of 1000 entries */
		// TODO: check maximum size of assn table. 
        
		assns->table = (assn_table_entry_t*)xmalloc(1000*sizeof(assn_table_entry_t));

		if(assns->table == NULL)
		{
			diag_printf("get_accesspt_assns:allocate assns->table[%d] failed\n", (1000*sizeof(assn_table_entry_t)));
			return TLV_GET_FAILED;
		}
		assns->assn_cnt = 0;
	}

	/* The writer routine will zero out the assn_cnt when it sends the last packet of the LTLV..
	 * Keep the table around for two seconds at most, and refresh if older than that.
	 */
	gettimeofday(&now, NULL);

	if ((now.tv_sec - assns->collection_time.tv_sec) > 2)
	{
		assns->assn_cnt = 0;
	}
	
	/* Force a re-assessment, whenever the count is zero */
	if (assns->assn_cnt == 0)
	{
		int rqfd;
		struct iwreq req;
		int ret, i;
		rt_lltd_assoc_table *rt_assns;
		rt_assns=(rt_lltd_assoc_table*)xmalloc(sizeof(rt_lltd_assoc_table));
		memset(rt_assns, 0, sizeof(rt_lltd_assoc_table));
		if((LLTP_RT_ioctl(RT_PRIV_IOCTL, rt_assns, sizeof(rt_lltd_assoc_table),g_osl->wireless_if,RT_OID_GET_LLTD_ASSO_TABLE)) != 0)
		{
			DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_accesspt_assns from driver(len=%d, ifname=%s)\n", sizeof(rt_lltd_assoc_table),g_osl->wireless_if);
			xfree(rt_assns);
			return TLV_GET_FAILED;
		}
		else
		{
			assns->assn_cnt = rt_assns->Num;

			if (assns->assn_cnt <= 0) {
				xfree(rt_assns);
				return TLV_GET_FAILED;
			}
			
			for (i=0; i<assns->assn_cnt; i++)
			{
				memcpy(&assns->table[i].MACaddr, &rt_assns->Entry[i].MACaddr, sizeof(etheraddr_t));
				assns->table[i].MOR = htons(rt_assns->Entry[i].MOR);       // units of 1/2 Mb per sec
				
				switch (rt_assns->Entry[i].PHYtype) {
				case PHY_11A:
				    	assns->table[i].PHYtype = WPM_OFDM_5G;
		       	 	break;
			    	case PHY_11G:
			    	case PHY_11BG_MIXED:
		    		case PHY_11BGN_MIXED:
			    		assns->table[i].PHYtype = WPM_DSSS_24G;
		        		break;	
			    	case PHY_11B:
				    	assns->table[i].PHYtype = WPM_FHSS_24G;
			        break;
	        		}
			}
			assns->collection_time = now;
			xfree(rt_assns);
			return TLV_GET_SUCCEEDED;
		}
	}
    	return TLV_GET_SUCCEEDED;
}


int
get_jumbo_icon(void *data)
{

	return TLV_GET_FAILED;
}


int
get_sees_max(void *data)
{
/*    TLVDEF( uint16_t,     sees_max,            , 0x19, Access_unset, TRUE ) */

    uint16_t    *sees_max = (uint16_t*)data;
    
    g_short_reorder_buffer = htons(NUM_SEES);
    memcpy(sees_max, &g_short_reorder_buffer, 2);

    return TLV_GET_SUCCEEDED;
}


radio_t   my_radio;

int
get_component_tbl(void *data)
{
/*    TLVDEF( comptbl_t,    component_tbl,       , 0x1A, Access_unset, FALSE ) */
    comptbl_t    *cmptbl = (comptbl_t*)data;
    uint8_t    cur_mode;
    etheraddr_t bssid;
    
    cmptbl->version = 1;
    cmptbl->bridge_behavior = 0;            // all packets transiting the bridge are seen by Responder
    cmptbl->link_speed = htonl((uint32_t)(100000000/100));     // units of 100bps
    cmptbl->radio_cnt = 1;
    cmptbl->radios = &my_radio;

    cmptbl->radios->MOR = htons((uint16_t)(54000000/500000));  // units of 1/2 Mb/sec
    if (get_wl_physical_medium((void*)&cmptbl->radios->PHYtype) == TLV_GET_FAILED)
    {
        cmptbl->radios->PHYtype = 0;            // 0=>Unk; 1=>2.4G-FH; 2=>2.4G-DS; 3=>IR; 4=>OFDM5G; 5=>HRDSSS; 6=>ERP
    }
    if (get_wireless_mode((void*)&cur_mode) == TLV_GET_FAILED)
    {
        cur_mode = 2;   // default is "automatic or unknown"
    }
    cmptbl->radios->mode = cur_mode;
    if (get_bssid((void*)&bssid) == TLV_GET_FAILED)
    {
        memcpy(&bssid, &g_hwaddr, sizeof(etheraddr_t));
    }
    memcpy(&cmptbl->radios->BSSID, &bssid, sizeof(etheraddr_t));

    return TLV_GET_SUCCEEDED;
}

int
get_repeaterAP_lineage(void *data)
{
    	return TLV_GET_FAILED;
}

int
get_repeaterAP_tbl(void *data)
{
/*    TLVDEF( comptbl_t,    repeaterAP_tbl,      , 0x1C, Access_unset, FALSE ) */
    return TLV_GET_FAILED;
}


int LLTP_RT_ioctl(
		int 			param, 
		char  			*data, 
		int 			data_len, 
		char 			*wlan_name, 
		int 			flags)
{
	char			name[12];
	int				ret = 1;
	struct iwreq	wrq;
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	int found = 0;

	memset(name,'\0',12);
    	sprintf(name, "%s", wlan_name);
    	name[3] = '\0';
    	wrq.u.data.flags = flags;
	wrq.u.data.length = data_len;
    	wrq.u.data.pointer = (caddr_t) data;
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, name) == 0) {
			found =1;
			break;
	        }
	}

	if ( found == 1 )
	{
		ret=rt28xx_ap_ioctl(sc, param, (caddr_t)&wrq);
	}
	else
		ret=1;
    return ret;
}

unsigned char* LLTD_Ecos_MemPool_Alloc (
	unsigned long Length,
	int Type)
{
    struct mbuf *pMBuf = NULL;

	switch (Type)
	{        
		case MemPool_TYPE_Header:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
            break;
		case MemPool_TYPE_CLUSTER:
			MGETHDR(pMBuf, M_DONTWAIT, MT_DATA);
			if (pMBuf== NULL)
				return NULL;
			MCLGET(pMBuf, M_DONTWAIT);
			if ((pMBuf->m_flags & M_EXT) == 0)
            {
				m_freem(pMBuf);
                return NULL;
            }
            break;
        default:
		diag_printf("%s: Unknown Type %d\n", __FUNCTION__, Type);
    		break;
	}
    
    return pMBuf;
}

void LLTD_EcosSendToRaw(char* if_name, char* buf, size_t len)
{
	cyg_netdevtab_entry_t *t;
	struct eth_drv_sc *sc;
	struct ifnet *ifp;
	int found = 0;
	 struct mbuf *m = NULL;
		
	for (t = &__NETDEVTAB__[0]; t != &__NETDEVTAB_END__; t++) 
	{
		sc = (struct eth_drv_sc *)t->device_instance;
		if (strcmp(sc->dev_name, if_name) == 0) {
			found =1;
			break;
	        }
	}

	if ( found == 1 )
	{
		if(IS_RLAINK(if_name))
		{
			if((LLTP_RT_ioctl(RT_PRIV_IOCTL, buf, len, if_name,RT_OID_802_DOT1X_LLTD_DATA)) != 0)
			{
				DBGPRINT(RT_DEBUG_ERROR,"ioctl failed for get_bssid from driver(len=%d, ifname=%s)\n", len,g_osl->wireless_if);
			}

		}
		else
		{
			ifp = &sc->sc_arpcom.ac_if;
			m=LLTD_Ecos_MemPool_Alloc(24,MemPool_TYPE_CLUSTER);
			if(m == NULL)
			{
				diag_printf("m == NULL %s\n",if_name);
			}
			else
			{
				m->m_pkthdr.rcvif = ifp;
				m->m_data = buf;
				m->m_pkthdr.len = len;
				m->m_len = m->m_pkthdr.len;

				if (IF_QFULL(&ifp->if_snd)) {
			                // Let the interface try a dequeue anyway, in case the
			                // interface has "got better" from whatever made the queue
			                // fill up - being unplugged for example.
			                if ((ifp->if_flags & IFF_OACTIVE) == 0)
			                    (*ifp->if_start)(ifp);
					IF_DROP(&ifp->if_snd);
					//senderr(ENOBUFS);
				}
				ifp->if_obytes += m->m_pkthdr.len;
				IF_ENQUEUE(&ifp->if_snd, m);

				if (m->m_flags & M_MCAST)
					ifp->if_omcasts++;
				if ((ifp->if_flags & IFF_OACTIVE) == 0)
					(*ifp->if_start)(ifp);
			}
		}
	}
	else
	{
		diag_printf("not find %s\n",if_name);
	}

}
