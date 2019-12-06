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

//#define CHECKING_PACKING 1

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define	DECLARING_GLOBALS
#include "globals.h"

#include "statemachines.h"
#include "packetio.h"

#include <net/netisr.h>


extern void qos_init(void);

cyg_mbox lltd_msgbox;
int lltd_msgbox_id = -1;
bool_t          isConfTest;
#define LLTP_STACK_SIZE 8192*2
unsigned char	LLTP_main_stack[LLTP_STACK_SIZE];
cyg_handle_t LLTP_thread;
cyg_thread LLTP_thread_struct;

//#ifdef RALINK_LLTD_SUPPORT
struct	ifqueue lltdintrq = {0, 0, 0, 50};
extern void packetio_recv_handler(struct ifnet *ifp, unsigned char *inpkt, int len);
//#endif /* RALINK_LLTD_SUPPORT */


void lltdintr()
{
	//diag_printf("dot1xintr\n");
	register struct mbuf *m;
	struct ifnet *ifp;
	int s;
	unsigned char *packet;
	int len;

	while (lltdintrq.ifq_head) {
		s = splimp();
		IF_DEQUEUE(&lltdintrq, m);
		splx(s);
		if (m == 0 || (m->m_flags & M_PKTHDR) == 0)
			panic("lltdintr");

		packet = m->m_data-14;
		len = m->m_len+14;

		ifp = m->m_pkthdr.rcvif;
		packetio_recv_handler(ifp, packet, len);

		m_freem(m);
	}
}


static void Cyg_LLTP(cyg_addrword_t param)
{
	DBGPRINT(RT_DEBUG_TRACE,"Enter Cyg_LLTP.....\n");
    	bool_t opt_debug = FALSE;
    	int    opt_trace = 0/*TRC_PACKET|TRC_STATE*/;
	int ret=0;
	unsigned char LAN_IF[]=CONFIG_DEFAULT_LAN_NAME;
	unsigned char WAN_IF[]=CONFIG_DEFAULT_WAN_NAME;
	unsigned char WIRELESS_IF[]=CONFIG_WIRELESS_NAME;
	// Create mail box
	
	g_Progname = "eCos_LLTD";
	g_interface=CONFIG_DEFAULT_LAN_NAME;//"eth0";
	g_wl_interface=CONFIG_WIRELESS_NAME;

	    g_trace_flags = opt_trace;
	  //  g_trace_flags |= TRC_TLVINFO;
		// icon_size=sizeof(ico_icon_file);
#if 0
		g_trace_flags |= TRC_BAND;
		g_trace_flags |= TRC_PACKET;
		g_trace_flags |= TRC_CHARGE;
		g_trace_flags |= TRC_STATE;
#endif	
	    memset(&g_hwaddr,0,sizeof(etheraddr_t));

	    g_smE_state = smE_Quiescent;
	    g_smT_state = smT_Quiescent;

	    memset (g_sessions,0,MAX_NUM_SESSIONS*sizeof(g_sessions[0]));

	    memset(&g_band,0,sizeof(band_t));		/* BAND algorthm's state */
	    g_osl = osl_init();				/* initialise OS-glue Layer */
	    memset(g_rxbuf,0,(size_t)RXBUFSZ);
	    memset(g_emitbuf,0,(size_t)RXBUFSZ);
	    memset(g_txbuf,0,(size_t)TXBUFSZ);
	    memset(g_re_txbuf,0,(size_t)TXBUFSZ);
	    g_sees = seeslist_new(NUM_SEES);
	    g_rcvd_pkt_len = 0;
	    g_rtxseqnum = 0;
	    g_re_tx_len = 0;
	    g_generation = 0;
	    g_sequencenum = 0;
	    g_opcode = Opcode_INVALID;
	    g_ctc_packets = 0;
	    g_ctc_bytes = 0;
	/* Initialize the timers (inactivity timers are init'd when session is created) */
	g_block_timer = g_charge_timer = g_emit_timer = g_hello_timer = NULL;

		cyg_mbox_create((cyg_handle_t*)&lltd_msgbox_id, &lltd_msgbox);
    		qos_init();

    		osl_interface_open(g_osl, g_interface, NULL);
    		osl_get_hwaddr(g_osl, &g_hwaddr);

		get_hwaddr("eth0", &g_hwaddr_eth0, TRUE);
		get_hwaddr("eth1", &g_hwaddr_eth1, TRUE);
		get_hwaddr("ra0", &g_hwaddr_ra0, TRUE);

	DBGPRINT(RT_DEBUG_TRACE,"%s: listening at address: " ETHERADDR_FMT "\n",
	    g_Progname, ETHERADDR_PRINT(&g_hwaddr));

	DBGPRINT(RT_DEBUG_TRACE,"%s Eth0 MAC address: " ETHERADDR_FMT "\n",
	    g_Progname, ETHERADDR_PRINT(&g_hwaddr_eth0));

	DBGPRINT(RT_DEBUG_TRACE,"%s Eth1 MAC address: " ETHERADDR_FMT "\n",
	    g_Progname, ETHERADDR_PRINT(&g_hwaddr_eth1));

	DBGPRINT(RT_DEBUG_TRACE,"%s ra0 MAC address: " ETHERADDR_FMT "\n",
	    g_Progname, ETHERADDR_PRINT(&g_hwaddr_ra0));

		osl_write_pidfile(g_osl);
    		osl_drop_privs(g_osl);

    /* add IO handlers & run main event loop forever */
    		//event_mainloop();
#if 0
	etheraddr_t bssid ;
	get_hostid(&bssid);
	DBGPRINT(RT_DEBUG_TRACE,"host_id(%02x:%02x:%02x:%02x:%02x:%02x).\n", MAC2STR(bssid.a));
	ipv4addr_t*  pipv4addr;
	pipv4addr=malloc(sizeof(ipv4addr_t));
	memset(&bssid,0x00,sizeof(etheraddr_t));
	get_hwaddr("ra0",(void*)&bssid,true);
	DBGPRINT(RT_DEBUG_TRACE,"bssid(%02x:%02x:%02x:%02x:%02x:%02x).\n", MAC2STR(bssid.a));
	get_ipv4addr(pipv4addr);
	//DBGPRINT(RT_DEBUG_TRACE,"ip(%:%02x:%02x:%02x:%02x:%02x).\n", MAC2STR(bssid.a));
      //  diag_printf("get_ipv4addr: returning %d.%d.%d.%d as ipv4addr\n", \
          //      *((uint8_t*)pipv4addr),*((uint8_t*)pipv4addr +1), \
              //  *((uint8_t*)pipv4addr +2),*((uint8_t*)pipv4addr +3));

	DBGPRINT(RT_DEBUG_TRACE,"get_ipv4addr: returning %d.%d.%d.%d as ipv4addr\n",
                *((uint8_t*)pipv4addr),*((uint8_t*)pipv4addr +1), 
                *((uint8_t*)pipv4addr +2),*((uint8_t*)pipv4addr +3));

	free(pipv4addr);

	ucs2char_t name;
	get_machine_name(&name);
	if (ret != -1)
    	{
		DBGPRINT(RT_DEBUG_TRACE,"system name %d\n", name);
	}
#endif
	event_mainloop();

}

int LLTP_Start(void)
{
		diag_printf("Start LLTD_Start.....(stack size=%d)\n",LLTP_STACK_SIZE);


		//register network isr handler
		register_netisr(NETISR_LLTD,lltdintr);

    	cyg_thread_create(8,
                       Cyg_LLTP,
                       0,
                       "lltp_daemon",
                       LLTP_main_stack,
                       sizeof(LLTP_main_stack),
                       &LLTP_thread,
                       &LLTP_thread_struct);

		cyg_thread_resume( LLTP_thread );
		return 0;
}

