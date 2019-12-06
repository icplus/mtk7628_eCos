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
	if_ra305x.c

    Abstract:
	Hardware driver for Ralink Ra305x ethernet devices
   
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <pkgconf/system.h>
#include <pkgconf/devs_eth_mips_ra305x.h>
#include <pkgconf/io_eth_drivers.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_io.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/eth/netdev.h>
#include <cyg/io/eth/eth_drv.h>

#ifdef CYGPKG_NET
#include <pkgconf/net.h>
#include <net/if.h>  /* Needed for struct ifnet */
#else
#include <cyg/hal/hal_if.h>
#endif

#if defined(CYGPKG_NET_OPENBSD_STACK) || defined(CYGPKG_NET_FREEBSD_STACK)
#define ETH_DRV_RXMBUF
#include <sys/mbuf.h>
#endif

#ifdef CYGPKG_IO_ETH_DRIVERS
#include <cyg/io/eth/eth_drv_stats.h>
#endif
#define KEEP_PRIVATE_STAT
#include "if_ra305x.h"

#ifdef BRANCH_ADV
#define ETH_DRV_SET_PHY	 1
#include <cfg_def.h>
#endif // BRANCH_ADV //

#include <sys/ioctl.h>
#include <sys/socket.h>




/*=====================================================================*/
#ifdef CYGDBG_DEVS_ETH_MIPS_RA305X_CHATTER
#define DBG_ERR			BIT0
#define DBG_INFO		BIT1
#define DBG_INIT		BIT2
#define DBG_INITMORE	BIT3
#define DBG_TX			BIT4
#define DBG_TXMORE		BIT5
#define DBG_RX			BIT6
#define DBG_RXMORE		BIT7
#define DBG_IOCTL		BIT8
#define DBG_ISR			BIT9
#define DBG_DSR			BIT10
#define DBG_DELIVER		BIT11

#define DBG_INFO_MASK		(DBG_ERR | DBG_INFO | DBG_INIT)

unsigned int EthDebugMask = DBG_INFO_MASK;

#define DBGPRINTF(_type, fmt, ...)				\
	do {							\
		if ((_type) & EthDebugMask)			\
			diag_printf(fmt, ## __VA_ARGS__);	\
	} while(0)

#else	/* !CYGDBG_DEVS_ETH_MIPS_RA305X_CHATTER  */

#define DBGPRINTF(_type, fmt, ...)				\
	do { } while(0)
#endif	/* CYGDBG_DEVS_ETH_MIPS_RA305X_CHATTER  */


/*=====================================================================*/
/*  extern functions                                                   */
#ifdef BRANCH_ADV
extern void CFG_get_mac(int id, char *dp);
#endif // BRANCH_ADV //


/*=====================================================================*/
/*  Interface function for upper layer                                 */
static void if_ra305x_start(struct eth_drv_sc *sc, unsigned char *enaddr, int flags);
static void if_ra305x_stop(struct eth_drv_sc *sc);
static int if_ra305x_ioctl(struct eth_drv_sc *sc, unsigned long key, void *data, int data_len);
static int if_ra305x_cansend(struct eth_drv_sc *sc);
static void if_ra305x_send(struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len, int total_len, unsigned long key);
static void if_ra305x_recv(struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len);
static void if_ra305x_deliver(struct eth_drv_sc *sc);
static void if_ra305x_poll(struct eth_drv_sc *sc);
static int if_ra305x_intvector(struct eth_drv_sc *sc);
static bool if_ra305x_init(struct cyg_netdevtab_entry *pnde);

static cyg_uint32 ra305x_isr(cyg_vector_t vec, cyg_addrword_t data);
static void ra305x_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data);

static cyg_uint32 ra305x_esw_isr(cyg_vector_t vec, cyg_addrword_t data);
static void ra305x_esw_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data);

void if_ra305x_debug(char *cmd);
/*=====================================================================*/
/*  Parameters                                                         */
#define NUM_RX0_DESC	CYGNUM_DEVS_ETH_MIPS_RA305X_RX_RING_SIZE
#define NUM_TX0_DESC	CYGNUM_DEVS_ETH_MIPS_RA305X_TX_RING_SIZE

#define NUM_RX_DESC	NUM_RX0_DESC
#define NUM_TX_DESC	NUM_TX0_DESC

#define HWDESC_AREA_SZ	(NUM_RX_DESC*sizeof(pdma_rxdesc_t) + NUM_TX_DESC*sizeof(pdma_txdesc_t))

#define ZEROBUF_LEN	64
#define UBUF_LEN	1536

#define NUM_VLANTAG_BUF	16
#define VLANTAG_BUF_LEN	(ETH_VLANTAG_LEN * NUM_VLANTAG_BUF)

#ifdef ETH_DRV_RXMBUF
#define NUM_UBUF 	64
#else 
#define NUM_UBUF	(8 + NUM_RX_DESC)
#endif
#define UBUF_AREA_SZ	(UBUF_LEN*NUM_UBUF)

#define UCMEM_SZ	(HAL_DCACHE_LINE_SIZE + HWDESC_AREA_SZ + UBUF_AREA_SZ + ZEROBUF_LEN + VLANTAG_BUF_LEN)


#define TX_MAX_BUFCNT	4

unsigned long eth_restart=0;
/*=====================================================================*/
/*  variables                                                          */
#ifdef CYGPKG_DEVS_ETH_MIPS_RA305X_ETH0
static if_ra305x_t ra305x_eth0_priv_data = {
	unit: 0,
	init_portmask: CONFIG_RA305X_LAN_PORTMASK,
	vid: 1,
#ifdef CYGSEM_DEVS_ETH_MIPS_RA305X_ETH0_SET_ESA
	macaddr: CYGDAT_DEVS_ETH_MIPS_RA305X_ETH0_ESA,
#else
	macaddr: {0, 0, 0, 0, 0, 0},
#endif
};

ETH_DRV_SC(ra305x_eth_sc0,
	   &ra305x_eth0_priv_data,			/*  Driver specific data  */
	   CYGDAT_DEVS_ETH_MIPS_RA305X_ETH0_NAME,	/*  Name of device  */
	   if_ra305x_start,
	   if_ra305x_stop,
	   if_ra305x_ioctl,
	   if_ra305x_cansend,
	   if_ra305x_send,
	   if_ra305x_recv,
	   if_ra305x_deliver,
	   if_ra305x_poll,
	   if_ra305x_intvector);

NETDEVTAB_ENTRY(ra305x_eth_netdev0,
	"ra305x_" CYGDAT_DEVS_ETH_MIPS_RA305X_ETH0_NAME,
	if_ra305x_init,
	&ra305x_eth_sc0);
#endif	/*  CYGPKG_DEVS_ETH_MIPS_RA305X_ETH0  */


#ifdef CYGPKG_DEVS_ETH_MIPS_RA305X_ETH1
static if_ra305x_t ra305x_eth1_priv_data = {
	unit: 1,
	init_portmask: CONFIG_RA305X_WAN_PORTMASK,
	vid: 2,
#ifdef CYGSEM_DEVS_ETH_MIPS_RA305X_ETH1_SET_ESA
	macaddr: CYGDAT_DEVS_ETH_MIPS_RA305X_ETH1_ESA,
#else
	macaddr: {0, 0, 0, 0, 0, 0},
#endif
};

ETH_DRV_SC(ra305x_eth_sc1,
	   &ra305x_eth1_priv_data,			/*  Driver specific data  */
	   CYGDAT_DEVS_ETH_MIPS_RA305X_ETH1_NAME,	/*  Name of device  */
	   if_ra305x_start,
	   if_ra305x_stop,
	   if_ra305x_ioctl,
	   if_ra305x_cansend,
	   if_ra305x_send,
	   if_ra305x_recv,
	   if_ra305x_deliver,
	   if_ra305x_poll,
	   if_ra305x_intvector);

NETDEVTAB_ENTRY(ra305x_eth_netdev1,
	"ra305x_" CYGDAT_DEVS_ETH_MIPS_RA305X_ETH1_NAME,
	if_ra305x_init,
	&ra305x_eth_sc1);
#endif	/*  CYGPKG_DEVS_ETH_MIPS_RA305X_ETH1  */


/*=====================================================================*/
/*  Local variables                                                    */
static eth_ra305x_t ra305xobj;
static eth_ra305x_t *pra305x= NULL;
static bufdesc_t bufdesc_pool[NUM_TX_DESC];
static cyg_uint32 rxbufdesc_pool[NUM_RX_DESC];
static cyg_uint8 ucmem[UCMEM_SZ];

//Copy from eCos kernel, remove the eth_drv_send(ifp);
static void ra305x_eth_drv_tx_done(struct eth_drv_sc *sc, CYG_ADDRESS key, int status)
{
    struct ifnet *ifp = &sc->sc_arpcom.ac_if;
    struct mbuf *m0 = (struct mbuf *)key;
    CYGARC_HAL_SAVE_GP();

    // Check for errors here (via 'status')
    ifp->if_opackets++;
    // Done with packet

    // Guard against a NULL return - can be caused by race conditions in
    // the driver, this is the neatest fixup:
    if (m0) { 
//        mbuf_key = m0;
        m_freem(m0);
    }
    // Start another if possible
//    eth_drv_send(ifp); 
    CYGARC_HAL_RESTORE_GP();
}

#ifdef ETH_DRV_RXMBUF
#define RA305X_MBUF_RESERVED		(16*4)  //pre-version set 16;change to 16*4 for PPTP/L2TP BOOST VERSION
struct mbuf *ra305x_getbuf(void)
{
	struct mbuf *m = NULL;
	
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m== NULL)
		return NULL;
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return NULL;
	}
	m->m_data += RA305X_RXBUF_OFFSET + RA305X_MBUF_RESERVED;
	return m;
}


static void ra305x_indicate_rx(if_ra305x_t *pif, struct mbuf *m, int frame_len, int remove_vlan)
{
	struct ifnet *ifp = &pif->sc->sc_arpcom.ac_if;
	struct ether_header *eh;
	cyg_uint16 *src, *dst;

    ifp->if_ipackets++;
    m->m_pkthdr.rcvif = ifp;
    m->m_pkthdr.len = frame_len;
	
	if (remove_vlan) {
		src = mtod(m, cyg_uint16 *);
		eh = (struct ether_header *) (m->m_data + ETH_VLANTAG_LEN);
		m->m_data += sizeof(struct ether_header) + ETH_VLANTAG_LEN;
		m->m_pkthdr.len = frame_len - sizeof(struct ether_header) - ETH_VLANTAG_LEN;
		m->m_len = m->m_pkthdr.len;
		
		/*  move the address field backward  */
		dst = (cyg_uint16 *) eh;
		dst[5] = src[5];	dst[4] = src[4];	dst[3] = src[3];
		dst[2] = src[2];	dst[1] = src[1];	dst[0] = src[0];
	} else {
		eh = mtod(m, struct ether_header *);
		m->m_data += sizeof(struct ether_header);
		m->m_pkthdr.len = frame_len - sizeof(struct ether_header);
		m->m_len = frame_len - sizeof(struct ether_header);
	}

    // Push data into protocol stacks
    ether_input(ifp, eh, m);
}
#endif


/*=====================================================================*
 *  ubuf_get:
 *=====================================================================*/
static inline ubuf_t *ubuf_get(void)
{
	ubuf_t *u = NULL;
	
	if ((u = pra305x->pfree_ubuf) != NULL) {
		pra305x->num_freeubuf--;
		pra305x->pfree_ubuf = u->next;
		u->next = NULL;
	}
	
	return u;
}
 

/*=====================================================================*
 *  ubuf_get:
 *=====================================================================*/
static inline void ubuf_free(ubuf_t *u)
{
	
	if (u != NULL) {
		u->next = pra305x->pfree_ubuf;
		pra305x->pfree_ubuf = u;
		pra305x->num_freeubuf++;
	}
}


/*=====================================================================*
 *  sg_to_buf:
 *=====================================================================*/
static int sg_to_buf(void *dst, int len, struct eth_drv_sg *sg_list, int sg_len, if_ra305x_t *pif)
{
	int cplen, total_len=0;
	struct eth_drv_sg *sg_last;
	cyg_uint8 *to = (cyg_uint8 *) dst;
	int cp1_len;
	
	for (sg_last=&sg_list[sg_len]; sg_list != sg_last; sg_list++) {
		if (sg_list-> len <=0 || sg_list->buf == 0)
			continue;
		cplen = (len > sg_list->len) ? sg_list->len : len;
		if ((total_len<= 2*ETH_ADDR_LEN) && ((total_len + cplen) > 2*ETH_ADDR_LEN)) {
			
			/*  Check if there is enough room to insert the VLAN tag  */
			if (len < cplen + 4)
				return total_len;
			
			/*  Insert VLAN tag  */
			cp1_len = 2 * ETH_ADDR_LEN - total_len;
			memcpy(to, (void *)sg_list->buf, cp1_len);
			
			to += cp1_len;
			to[0] = pif->ptxvtag[0];
			to[1] = pif->ptxvtag[1];
			to[2] = pif->ptxvtag[2];
			to[3] = pif->ptxvtag[3];
			
			/*  Copy the remaining in this sg_list  */
			to +=ETH_VLANTAG_LEN;
			len -= ETH_VLANTAG_LEN;
			memcpy(to, (void *)(sg_list->buf+cp1_len), cplen - cp1_len);
			to += cplen - cp1_len;
			total_len += ETH_VLANTAG_LEN;
		} else {
			memcpy(to, (void *)sg_list->buf, cplen);
			to += cplen;
		}
		
		total_len += cplen;
		if ((len -= cplen) <=0)
			break;
	}
	return total_len;
}
 

/*=============================================================*
 *  txctrl_init:
 *=============================================================*/
#ifdef BRANCH_STD
static
#endif // BRANCH_STD //
void txctrl_init(txctrl_t *ptxc, cyg_uint32 tx_base, int num_txd)
{
	int i;
	
	ptxc->num_txd = num_txd;

	/* Zero all tx desc */
	memset((void *) ptxc->ptxd, 0, ptxc->num_txd * sizeof(pdma_txdesc_t));
	
	DBGPRINTF(DBG_INIT, "ifra305x: init numtxd=%d txd-base=%03x at %08x\n",
			ptxc->num_txd, tx_base, (cyg_uint32) ptxc->ptxd);

	ptxc->tx_base = tx_base;
	
	/*  Init Tx descriptor  */
	for(i=0; i<ptxc->num_txd; i++) {
		ptxc->ptxd[i].tx_buf = TX_DDONE | TX_LS0;
	}

	/*  Init buffer descriptor  */
	for(i=0; i<ptxc->num_txd; i++) {
		ptxc->ptxbuf[i].sc = NULL;
		ptxc->ptxbuf[i].buf = NULL;
	}	
	
	ptxc->num_freetxd = ptxc->num_txd;
	ptxc->tx_head = ptxc->tx_tail = 0;
}


/*=============================================================*
 *  rxctrl_init:
 *=============================================================*/
static void rxctrl_init(rxctrl_t *prxc, cyg_uint32 rx_base, int num_rxd)
{
	int i;
	pdma_rxdesc_t *prxd;
	cyg_uint32 *prxbuf;
	
	prxc->num_rxd = num_rxd;
	
	// Zero all rx desc
	memset((void *) prxc->prxd, 0, prxc->num_rxd * sizeof(pdma_rxdesc_t));	

	DBGPRINTF(DBG_INIT, "ifra305x: init num_rxd=%d rxd_base=%03x at %08x\n",
			prxc->num_rxd, rx_base, (cyg_uint32) prxc->prxd);

	prxc->rx_base = rx_base;


	prxd = prxc->prxd;
	prxbuf = prxc->prxbuf;
	for (i=0; i< prxc->num_rxd; i++, prxd++)
	{
#ifdef ETH_DRV_RXMBUF
        struct mbuf *m;

		m = ra305x_getbuf();
		CYG_ASSERTC(m != NULL);
		if(!m)
			{panic("rxctrl_init ra305x_getbuf"); return;}
		prxd->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)m->m_data);
		prxbuf[i] = (cyg_uint32) m;
#else
        ubuf_t *u;

		u = ubuf_get();
		CYG_ASSERTC(u != NULL);
		
		prxd->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)u + RA305X_RXBUF_OFFSET);
		prxbuf[i] = (cyg_uint32) u;
#endif
#if defined (CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		prxd->rx_status = UBUF_LEN << 16;
#else
		prxd->rx_status = RX_LS0;
#endif
	}

	prxc->num_freerxd = prxc->num_rxd;
	prxc->rx_head = 0;
//	prxc->rx_tail = 0;
}

/*=============================================================*
/
==============================================================*/
static void rxctrl_free(rxctrl_t *prxc)
{
		int i;
		cyg_uint32 *prxbuf;
		
		prxbuf = prxc->prxbuf;
		for (i=0; i< prxc->num_rxd; i++)
		{
#ifdef ETH_DRV_RXMBUF
			struct mbuf *m;

			m = (struct mbuf *)prxbuf[i];	
			prxbuf[i] = NULL;
			if (m != NULL)
				m_freem(m);
#endif
			}
	}



void ra305x_close(void)
{

diag_printf("[%s]====>\n",__FUNCTION__);
	struct ifnet *ifp = NULL;

	if (pra305x == NULL)
			return;

	rxctrl_free(&pra305x->rx0);

	pra305x = NULL;
	
	diag_printf("[%s]<====\n",__FUNCTION__);
}


void ra305x_restart(void)
{
	 cyg_netdevtab_entry_t *t;

	diag_printf("[%s]====>\n",__FUNCTION__);
	 eth_restart = 1;
	 t =&ra305x_eth_netdev0;
	 
	t->init(t);

	 t =&ra305x_eth_netdev1;
	  t->init(t);
	  if_ra305x_start( &ra305x_eth_sc1, NULL, 0);

	eth_restart = 0;	
		diag_printf("[%s]<====\n",__FUNCTION__);

}
/*=============================================================*
 *  ra305x_set_macaddr:
 *=============================================================*/
static void ra305x_set_macaddr(cyg_uint8 *mac)
{
	cyg_uint32 regH, regL;
	
	regH = mac[0] << 8 | mac[1];
	regL = (mac[2] << 24) | (mac[3]<<16) | (mac[4]<<8) | mac[5];

#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
	RAFE_REG(RAFE_SDM_MAC_ADRH) = regH;	
	RAFE_REG(RAFE_SDM_MAC_ADRL) = regL;
#else /* !CONFIG_RT5350_ASIC */
	RAFE_REG(RAFE_GDMA1_MAC_ADRH) = regH;
	RAFE_REG(RAFE_GDMA1_MAC_ADRL) = regL;
#endif /* CONFIG_RT5350_ASIC||CONFIG_MT7628_ASIC */
}


#if defined (CONFIG_MT7628_ASIC)

#if 0		//2014-10-15
void mt7628_ephy_init(void)
{
	int i;
	cyg_uint32 phy_val;
	mii_write_register(0, 31, 0x2000); //change G2 page
	mii_write_register(0, 26, 0x0020);

	for(i=0; i<5; i++){
		mii_write_register(i, 31, 0x8000); //change L0 page
		mii_write_register(i,  0, 0x3100);
		mii_write_register(i, 31, 0xa000); // change L2 page
		mii_write_register(i, 16, 0x0606);
		mii_write_register(i, 23, 0x0f0e);
		mii_write_register(i, 24, 0x1610);
		mii_write_register(i, 30, 0x1f1d);
		mii_write_register(i, 28, 0x6111);
		phy_val=mii_read_register(i, 4);
		phy_val |= (1 << 10);
		mii_write_register(i, 4, phy_val);// turn FC
	}

	//100Base AOI setting
	mii_write_register(0, 31, 0x5000);  //change G5 page
	mii_write_register(0, 19, 0x004a);
	mii_write_register(0, 20, 0x015a);
	mii_write_register(0, 21, 0x00ee);
	mii_write_register(0, 22, 0x0033);
	mii_write_register(0, 23, 0x020a);
	mii_write_register(0, 24, 0x0000);
	mii_write_register(0, 25, 0x024a);
	mii_write_register(0, 26, 0x035a);
	mii_write_register(0, 27, 0x02ee);
	mii_write_register(0, 28, 0x0233);
	mii_write_register(0, 29, 0x000a);
	mii_write_register(0, 30, 0x0000);
}
#endif

void mt7628_ephy_init(void)
{
	int i;
	cyg_uint32 phy_val;
	mii_write_register(0, 31, 0x2000); //change G2 page
	mii_write_register(0, 26, 0x0000);

	for(i=0; i<5; i++){
		mii_write_register(i, 31, 0x8000); //change L0 page
		mii_write_register(i,  0, 0x3100);

#if defined (CONFIG_RAETH_8023AZ_EEE)	//2014-10-15:not support yet in eCos MT7628K now.
		phy_val=mii_read_register(i, 26);// EEE setting
		phy_val |= (1 << 5);
		mii_write_register(i, 26, phy_val);
#else
		//disable EEE
		mii_write_register(i, 13, 0x7);
		mii_write_register(i, 14, 0x3C);
		mii_write_register(i, 13, 0x4007);
		mii_write_register(i, 14, 0x0);
#endif
		mii_write_register(i, 30, 0xa000);
		mii_write_register(i, 31, 0xa000); // change L2 page
		mii_write_register(i, 16, 0x0606);
		mii_write_register(i, 23, 0x0f0e);
		mii_write_register(i, 24, 0x1610);
		mii_write_register(i, 30, 0x1f15);
		mii_write_register(i, 28, 0x6111);

		phy_val=mii_read_register(i, 4);
		phy_val |= (1 << 10);
		mii_write_register(i, 4, phy_val);// turn FC
	}

        //100Base AOI setting
	mii_write_register(0, 31, 0x5000);  //change G5 page
	mii_write_register(0, 19, 0x004a);
	mii_write_register(0, 20, 0x015a);
	mii_write_register(0, 21, 0x00ee);
	mii_write_register(0, 22, 0x0033);
	mii_write_register(0, 23, 0x020a);
	mii_write_register(0, 24, 0x0000);
	mii_write_register(0, 25, 0x024a);
	mii_write_register(0, 26, 0x035a);
	mii_write_register(0, 27, 0x02ee);
	mii_write_register(0, 28, 0x0233);
	mii_write_register(0, 29, 0x000a);
	mii_write_register(0, 30, 0x0000);
}


#endif

/*=============================================================*
 *  ra305x_fe_reset:
 *=============================================================*/
#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
static void ra305x_fe_reset(void)
{
        cyg_uint32 sysctl_cfg;

        /* RT5350 need to reset ESW and FE at the same time*/
        sysctl_cfg = HAL_SYSCTL_REG(RA305X_SYSCTL_RESET);
        sysctl_cfg |= (RA305X_SYSCTL_FE_RST | RA305X_SYSCTL_SW_RST);
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = sysctl_cfg;

        sysctl_cfg &= ~(RA305X_SYSCTL_FE_RST | RA305X_SYSCTL_SW_RST);
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = sysctl_cfg;
}
#elif defined(CONFIG_RT7620_ASIC)
static void ra305x_fe_reset(void)
{
        cyg_uint32 sysctl_cfg;

        /* RT5350 need to reset ESW and FE at the same time*/
        sysctl_cfg = HAL_SYSCTL_REG(RA305X_SYSCTL_RESET);
        sysctl_cfg |= RA305X_SYSCTL_FE_RST;
        //sysctl_cfg |= (RA305X_SYSCTL_FE_RST | RA305X_SYSCTL_SW_RST);
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = sysctl_cfg;

        sysctl_cfg &= ~(RA305X_SYSCTL_FE_RST | RA305X_SYSCTL_SW_RST);
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = sysctl_cfg;
}
#else /* !CONFIG_RT5350_ASIC */
static void ra305x_fe_reset(void)
{
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) |= RA305X_SYSCTL_FE_RST;
	HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = 0;
}
#endif /* CONFIG_RT5350_ASIC */


/*=============================================================*
 *  ra305x_set_fe_pdma_glo_cfg:
 *=============================================================*/
static inline void ra305x_set_fe_pdma_glo_cfg(void)
{
#if !defined(CONFIG_RT5350_ASIC) && !defined(CONFIG_RT7620_ASIC)&& !defined(CONFIG_MT7628_ASIC) 	
	cyg_uint32 fe_glo_cfg;
#endif

	DBGPRINTF(DBG_INITMORE, "ifra305x: fe_pdma_glo_cfg\n");
#ifdef CONFIG_RT7620_ASIC
#if defined(CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
	RAFE_REG(RAFE_PDMA_GLO_CFG) |= RAFE_PDMA_TX_DMA_EN | RAFE_PDMA_RX_DMA_EN 
				| RAFE_PDMA_BT_SIZE_16DWORDS | RAFE_PDMA_TX_WB_DDONE | RAFE_RX_2B_OFFSET;
#else
	RAFE_REG(RAFE_PDMA_GLO_CFG) |= RAFE_PDMA_TX_DMA_EN | RAFE_PDMA_RX_DMA_EN 
				| RAFE_PDMA_BT_SIZE_16DWORDS | RAFE_PDMA_TX_WB_DDONE;
#endif
#else
	RAFE_REG(RAFE_PDMA_GLO_CFG) |= RAFE_PDMA_TX_DMA_EN | RAFE_PDMA_RX_DMA_EN 
				| RAFE_PDMA_BT_SIZE_4DWORDS | RAFE_PDMA_TX_WB_DDONE;
#endif

#if !defined(CONFIG_RT5350_ASIC) && !defined(CONFIG_RT7620_ASIC)&& !defined(CONFIG_MT7628_ASIC) 	
	/* set 1us timer count in unit of clock cycle  */
	fe_glo_cfg = RAFE_REG(RAFE_FE_GLO_CFG) & ~(0xff << 8);
	fe_glo_cfg |= (HAL_GET_SYSBUS_FREQ()/1000000) << 8;
	RAFE_REG(RAFE_FE_GLO_CFG) = fe_glo_cfg;
#endif /* !CONFIG_RT5350_ASIC */        

	DBGPRINTF(DBG_INITMORE, "ifra305x: fe_pdma_glo_cfg done\n");
}


/*=============================================================*
 *  ra305x_forward_cfg:
 *=============================================================*/
#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
/* RT5350: No GDMA, PSE, CDMA, PPE */
static inline void ra305x_forward_cfg(void)
{
	cyg_uint32 sdm_cfg;
        
	DBGPRINTF(DBG_INITMORE, "ifra305x: forward_cfg\n");

	sdm_cfg = RAFE_REG(RAFE_SDM_CON);
        sdm_cfg |= 0x7<<16; // UDPCS, TCPCS, IPCS=1
        RAFE_REG(RAFE_SDM_CON) = sdm_cfg;
	
}
#else /* !CONFIG_RT5350_ASIC */
static inline void ra305x_forward_cfg(void)
{
	cyg_uint32 fwd_cfg, csg_cfg;
	
	DBGPRINTF(DBG_INITMORE, "ifra305x: forward_cfg\n");

#if defined(CONFIG_PDMA_NEW)
	fwd_cfg = RAFE_REG(RAFE_GDMA1_FWD_CFG) & ~0x7;
#else
	fwd_cfg = RAFE_REG(RAFE_GDMA1_FWD_CFG) & ~0xffff;
#endif
	csg_cfg = RAFE_REG(RAFE_CDMA_CSG_CFG) & ~0x7;
	
#ifdef HW_CHECKSUM_OFFLOAD
	/*  enable IPv4 header sum check and generation */
	fwd_cfg |= RAFE_GDMA1_ICS_EN;
	csg_cfg |= RAFE_ICS_GEN_EN;
	
	/*  enable TCP sum check and generation */
	fwd_cfg |= RAFE_GDMA1_TCS_EN;
	csg_cfg |= RAFE_TCS_GEN_EN;

	/*  enable UDP sum check and generation */
	fwd_cfg |= RAFE_GDMA1_UCS_EN;
	csg_cfg |= RAFE_UCS_GEN_EN;
	
#else
	/*  disable IPv4 header sum check and generation */
	fwd_cfg &= ~RAFE_GDMA1_ICS_EN;
	csg_cfg &= ~RAFE_ICS_GEN_EN;
	
	/*  disable TCP sum check and generation */
	fwd_cfg &= ~RAFE_GDMA1_TCS_EN;
	csg_cfg &= ~RAFE_TCS_GEN_EN;

	/*  disable UDP sum check and generation */
	fwd_cfg &= ~RAFE_GDMA1_UCS_EN;
	csg_cfg &= ~RAFE_UCS_GEN_EN;
#endif

	fwd_cfg |= RAFE_GDMA1_STRPCRC;

	RAFE_REG(RAFE_GDMA1_FWD_CFG) = fwd_cfg;
	RAFE_REG(RAFE_CDMA_CSG_CFG) = csg_cfg;

#if !defined(CONFIG_RT7620_ASIC)	
	RAFE_REG(RAFE_PSE_FQ_CFG) = DEF_RAFE_PSE_FQ_VAL;
#endif	

	/*  Reset PSE afer re-programming RAFE_PSE_FQ_CFG  */
	RAFE_REG(RAFE_FE_RST_GL) = RAFE_RST_GL_PSE_RESET;
	RAFE_REG(RAFE_FE_RST_GL) = 0;
	
	DBGPRINTF(DBG_INITMORE, "ifra305x: forward_cfg done\n");
}
#endif /* CONFIG_RT5350_ASIC */


/*=============================================================*
 *  enable_mdio:
 *=============================================================*/
static void enable_mdio(int enable)
{
#if !defined (CONFIG_P5_MAC_TO_PHY_MODE)
	cyg_uint32 data;
	
	data = HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE);
	if (enable)
		data &= ~RA305X_GPIO_MDIO_DIS;
	else
		data |= RA305X_GPIO_MDIO_DIS;
	HAL_SYSCTL_REG(RA305X_SYSCTL_GPIO_PURPOSE) = data;
#endif
}


/*=============================================================*
 *  ra305x_mii_readreg:
 *=============================================================*/
static int ra305x_mii_readreg(int phyaddr, int regnum)
{
	cyg_uint32 status;
//	cyg_uint32 oldint;
	
//	HAL_DISABLE_INTERRUPTS(oldint);
	enable_mdio(1);
#if defined(CONFIG_RT7620_ASIC)
	do {
		status = RAESW_REG(RAESW_PHY_CTRL0_REG);
	} while ((status & (1<<31)));
	
	RAESW_REG(RAESW_PHY_CTRL0_REG) = (0x2 << 18) | (0x1 << 16) | (phyaddr << 20) | (regnum << 25);
	RAESW_REG(RAESW_PHY_CTRL0_REG) = (0x2 << 18) | (0x1 << 16) | (phyaddr << 20) | (regnum << 25) | (1 << 31);
	do {
		status = RAESW_REG(RAESW_PHY_CTRL0_REG);
	} while ((status & (1<<31)));
#else
	RAESW_REG(RAESW_PHY_CTRL0_REG) = RAESW_PHY_MII_READ | phyaddr
		 | (regnum << RAESW_PHY_MII_REGNUM_SHIFT);
	 do {
		status = RAESW_REG(RAESW_PHY_CTRL1_REG);
	} while (!(status & RAESW_PHY_MII_READOK));
#endif
	enable_mdio(0);
//	HAL_RESTORE_INTERRUPTS(oldint);

#if defined(CONFIG_RT7620_ASIC)
	return (status & 0xffff);
#else
	return (status >> RAESW_PHY_MII_DATA_SHIFT);
#endif
}


/*=============================================================*
 *  ra305x_mii_writereg:
 *=============================================================*/
static void ra305x_mii_writereg(int phyaddr, int regnum, int val)
{
	cyg_uint32 status;
//	cyg_uint32 oldint;

//	HAL_DISABLE_INTERRUPTS(oldint);
	enable_mdio(1);
#if defined(CONFIG_RT7620_ASIC)
	do {
		status = RAESW_REG(RAESW_PHY_CTRL0_REG);
	} while ((status & (1<<31)));

	RAESW_REG(RAESW_PHY_CTRL0_REG) = (0x1 << 18) | (0x1 << 16) | (phyaddr << 20) | (regnum << 25) | val;
	RAESW_REG(RAESW_PHY_CTRL0_REG) = (0x1 << 18) | (0x1 << 16) | (phyaddr << 20) | (regnum << 25) | val | (1 << 31);

	do {
		status = RAESW_REG(RAESW_PHY_CTRL0_REG);
	} while ((status & (1<<31)));

#else
	RAESW_REG(RAESW_PHY_CTRL0_REG) = RAESW_PHY_MII_WRITE | phyaddr 
		| (val<<RAESW_PHY_MII_DATA_SHIFT) | (regnum << RAESW_PHY_MII_REGNUM_SHIFT);

	while (!(RAESW_REG(RAESW_PHY_CTRL1_REG) & RAESW_PHY_MII_WRITEOK));
#endif
	enable_mdio(0);
//	HAL_RESTORE_INTERRUPTS(oldint);
}


/*=============================================================*
 *  mii_read_register:
 *=============================================================*/
#ifdef BRANCH_STD
static
#endif // BRANCH_STD //
int mii_read_register(int phyaddr, int regnum)
{
	cyg_uint32 val;
	cyg_uint32 oldint;
	
	HAL_DISABLE_INTERRUPTS(oldint);
	val = ra305x_mii_readreg(phyaddr, regnum);
	HAL_RESTORE_INTERRUPTS(oldint);

	return (val & 0xffff);
}


/*=============================================================*
 *  mii_write_register:
 *=============================================================*/
#ifdef BRANCH_STD
static
#endif // BRANCH_STD //
void mii_write_register(int phyaddr, int regnum, int val)
{
	cyg_uint32 oldint;

	HAL_DISABLE_INTERRUPTS(oldint);
	ra305x_mii_writereg(phyaddr, regnum, val&0xffff);
	HAL_RESTORE_INTERRUPTS(oldint);
}

/*=============================================================*
 *  mii_write_register:
 *=============================================================*/
#define RF_CSR_CFG      0xb0180500
#define RF_CSR_KICK     (1<<17)
int rw_rf_reg(int write, int reg, int *data)
{
        unsigned long    rfcsr, i = 0;

        while (1) {
                rfcsr =  sysRegRead(RF_CSR_CFG);
                if (! (rfcsr & (cyg_uint32)RF_CSR_KICK) )
                        break;
                if (++i > 10000) {
                        DBGPRINTF(DBG_INITMORE, "Warning: Abort rw rf register: too busy\n");
                        return -1;
                }
        }

        rfcsr = (cyg_uint32)(RF_CSR_KICK | ((reg&0x3f) << 8) | (*data & 0xff));
        if (write)
                rfcsr |= 0x10000;

         sysRegRead(RF_CSR_CFG) = cpu_to_le32(rfcsr);

        i = 0;
        while (1) {
                rfcsr =  sysRegRead(RF_CSR_CFG);
                if (! (rfcsr & (cyg_uint32)RF_CSR_KICK) )
                        break;
                if (++i > 10000) {
                        DBGPRINTF(DBG_INITMORE, "Warning: still busy\n");
                        return -1;
                }
        }

        rfcsr =  sysRegRead(RF_CSR_CFG);

        if (((rfcsr&0x1f00) >> 8) != (reg & 0x1f)) {
                DBGPRINTF(DBG_INITMORE, "Error: rw register failed\n");
                return -1;
        }
        *data = (int)(rfcsr & 0xff);

        return 0;
}

#ifdef CONFIG_BRIDGE
/*=============================================================*
 *  rtbr_del:
 *=============================================================*/
static void rtbr_del(void)
{
#if 0 //Eddy6
	struct eth_drv_sc *ethsc = NULL;
	struct bridge_softc *sc = NULL;
	struct ifnet        *ifp = NULL;

	ethsc = pra305x->iflist->sc;
	ifp = &ethsc->sc_arpcom.ac_if;
	if ((ifp != NULL) && (ifp->if_bridge))
	{
		sc = (struct bridge_softc *)ifp->if_bridge;
		bridge_rtflush(sc, 1);
	}
#endif
}
#endif // CONFIG_BRIDGE //


#if defined(CONFIG_RT7620_ASIC)
/*=============================================================*
 *  ra305x_esw_init:
 *=============================================================*/
static void ra305x_esw_init(void)
{
	cyg_uint8 is_BGA;
	int i;

        /*  Disable all switch interrupt  */
        RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;
        /*  Ack all switch interrupt  */
        RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;

	/* LAN/WAN ports as security mode */
	for (i = 0; i < 6; i++)
		RAESW_REG(0x2004 + i*0x100) = 0xff0003;
	/* LAN/WAN ports as transparent port */
	for (i = 0; i < 6; i++)
		RAESW_REG(0x2010 + i*0x100) = 0x810000c0;

	/* Set CPU/P7 ports as user port*/
	RAESW_REG(0x2610) = 0x81000000;
	RAESW_REG(0x2710) = 0x81000000;

	/* CPU/P7 Egress VLAN Tag Attribution=tagged */
	RAESW_REG(0x2604) = 0x20ff0003;
	RAESW_REG(0x2704) = 0x20ff0003;


	is_BGA = (HAL_SYSCTL_REG(0xc) >> 16) & 0x1;
	DBGPRINTF(DBG_INITMORE, "is_BGA=%d\n", is_BGA);
	/*
	* Reg 31: Page Control
	* Bit 15	 => PortPageSel, 1=local, 0=global
	* Bit 14:12  => PageSel, local:0~3, global:0~4
	*
	* Reg16~30:Local/Global registers
	*
	*/
	/*correct  PHY  setting L3.0 BGA*/
	ra305x_mii_writereg(1, 31, 0x4000); //global, page 4
  
	ra305x_mii_writereg(1, 17, 0x7444);
	if(is_BGA){
	ra305x_mii_writereg(1, 19, 0x0114);
	}else{
	ra305x_mii_writereg(1, 19, 0x0117);
	}

	ra305x_mii_writereg(1, 22, 0x10cf);
	ra305x_mii_writereg(1, 25, 0x6212);
	ra305x_mii_writereg(1, 26, 0x0777);
	ra305x_mii_writereg(1, 29, 0x4000);
	ra305x_mii_writereg(1, 28, 0xc077);
	ra305x_mii_writereg(1, 24, 0x0000);
	
	ra305x_mii_writereg(1, 31, 0x3000); //global, page 3
	ra305x_mii_writereg(1, 17, 0x4838);

	ra305x_mii_writereg(1, 31, 0x2000); //global, page 2
	if(is_BGA){
	ra305x_mii_writereg(1, 21, 0x0515);
	ra305x_mii_writereg(1, 22, 0x0053);
	ra305x_mii_writereg(1, 23, 0x00bf);
	ra305x_mii_writereg(1, 24, 0x0aaf);
	ra305x_mii_writereg(1, 25, 0x0fad);
	ra305x_mii_writereg(1, 26, 0x0fc1);
	}else{
	ra305x_mii_writereg(1, 21, 0x0517);
	ra305x_mii_writereg(1, 22, 0x0fd2);
	ra305x_mii_writereg(1, 23, 0x00bf);
	ra305x_mii_writereg(1, 24, 0x0aab);
	ra305x_mii_writereg(1, 25, 0x00ae);
	ra305x_mii_writereg(1, 26, 0x0fff);
	}
	ra305x_mii_writereg(1, 31, 0x1000); //global, page 1
	ra305x_mii_writereg(1, 17, 0xe7f8);
	
	ra305x_mii_writereg(1, 31, 0x8000); //local, page 0
	ra305x_mii_writereg(0, 30, 0xa000);
	ra305x_mii_writereg(1, 30, 0xa000);
	ra305x_mii_writereg(2, 30, 0xa000);
	ra305x_mii_writereg(3, 30, 0xa000);
#if !defined (CONFIG_RAETH_HAS_PORT4)   
	ra305x_mii_writereg(4, 30, 0xa000);
#endif

	ra305x_mii_writereg(1, 31, 0xa000); //local, page 2
	ra305x_mii_writereg(0, 16, 0x1111);
	ra305x_mii_writereg(1, 16, 0x1010);
	ra305x_mii_writereg(2, 16, 0x1515);
	ra305x_mii_writereg(3, 16, 0x0f0f);
#if !defined (CONFIG_RAETH_HAS_PORT4)   
	ra305x_mii_writereg(4, 16, 0x1313);
#endif

	RAESW_REG(0x3600) = 0x5e33b;//CPU Port6 Force Link 1G, FC ON
	RAESW_REG(0x0010) = 0x7f7f7fe0;//Set Port6 CPU Port
	RAESW_REG(0x3500) = 0x8000;//link down
	HAL_SYSCTL_REG(0x14) |= (0x3 << 14); //GE2_MODE=RJ45 Mode
	HAL_SYSCTL_REG(0x60) |= (3 << 9); //set RGMII/RGMII2 to GPIO mode

	
	DBGPRINTF(DBG_INITMORE, "ra305x_esw_init done\n");
} 

#else // #if defined(CONFIG_RT7620_ASIC)

/*=============================================================*
 *  ra305x_esw_init:
 *=============================================================*/
static void ra305x_esw_init(void)
{ 
	int i=0;
	cyg_uint32 phy_val=0;
	cyg_uint32 val=0;

	DBGPRINTF(DBG_INITMORE, "ifra305x: esw init\n");
	
#ifdef CONFIG_MT7628_ASIC
	{
		/*
		*set Switch Port PIO mode to Analog PAD
		*/
		unsigned int reg =0;
		reg = HAL_SYSCTL_REG( 0x3C) ;		
#ifdef CONFIG_ETH_ONE_PORT_ONLY
			reg |= 0x1F0000;

			reg = reg & ~(0X10000);
#else
			reg &= ~(0x1F<<16);
#endif
         if (HAL_SYSCTL_REG(8) & 0x10000)
				val &= ~0x1f0000;
		HAL_SYSCTL_REG(0x3C)=reg;
		}
#endif			

	/*  Disable all switch interrupt  */
	RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;
	/*  Ack all switch interrupt  */
	RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;

	/*
	 * FC_RLS_TH=200, FC_SET_TH=160
	 * DROP_RLS=120, DROP_SET_TH=80
	 */
	RAESW_REG(RAESW_REG_OFF_08) = 0xC8A07850;
	RAESW_REG(RAESW_REG_OFF_E4) = 0;
	RAESW_REG(RAESW_REG_OFF_14) = 0x405555;
	
	RAESW_REG(RAESW_VLANID0_REG) = 0x2001;
	RAESW_REG(RAESW_VLANID1_REG) = 0X4003;
	RAESW_REG(RAESW_VLANID2_REG) = 0X6005;
	
	RAESW_REG(RAESW_PVID0_REG) = 0x1001;
	RAESW_REG(RAESW_PVID1_REG) = 0x1001;
	RAESW_REG(RAESW_PVID2_REG) = 0x1001;
	RAESW_REG(RAESW_PVID3_REG) = 0x1;
	
	RAESW_REG(RAESW_VLANMEM0_REG) = 0;
	RAESW_REG(RAESW_VLANMEM1_REG) = 0;
	RAESW_REG(RAESW_VLANMEM2_REG) = 0;
	RAESW_REG(RAESW_VLANMEM3_REG) = 0;
	
	RAESW_REG(RAESW_PORTCTRL0_REG) = 0x7f7f;
	RAESW_REG(RAESW_PORTCTRL2_REG) = 0x7f3f;
	RAESW_REG(RAESW_GCTRL_REG) = 0x8a301;	//hashing algorithm=XOR48, aging interval=300sec
	
    RAESW_REG(RAESW_REG_OFF_CC) = 0x00d6500c;
    RAESW_REG(RAESW_REG_OFF_8C) = 0x02404040;
	
   	RAESW_REG(RAESW_REG_OFF_C8) = 0x3f502b28; 	//Change polling Ext PHY Addr=0x1F
    RAESW_REG(RAESW_REG_OFF_84) = 0;

#if defined(CONFIG_RT5350_ASIC) 
    RAESW_REG(RAESW_LED_CONTROL) = 0x17;            //Per Port LED Polarity Control
#else if defined(CONFIG_MT7628_ASIC)	

	val =HAL_SYSCTL_REG(0x64);
#if defined (CONFIG_ETH_ONE_PORT_ONLY)
	val &= 0xf003f003;
	val |= 0x05500550;
	HAL_SYSCTL_REG(0x64)=val; // set P0 EPHY LED mode
#else
	val &= 0xf003f003;
	HAL_SYSCTL_REG(0x64)=val; // set P0~P4 EPHY LED mode
#endif


	//HAL_SYSCTL_REG(0x64) = 0x00010001;
#endif


#ifdef CONFIG_MT7628_ASIC
  	mt7628_ephy_init( );

#else

     ra305x_mii_writereg(0, 31, 0x8000);   		//---> select local register
     for(i=0;i<5;i++)
      {
        ra305x_mii_writereg(i, 26, 0x1600);	//TX10 waveform coefficient
#if defined (CONFIG_RT3052_ASIC)
		rw_rf_reg(0, 0, &phy_val);
		phy_val = phy_val >> 4;

		if(phy_val > 0x5)
		{
			ra305x_mii_writereg(i, 29, 0x7015);   //TX100/TX10 AD/DA current bias
			ra305x_mii_writereg(i, 30, 0x0038);   //TX100 slew rate control
		}
		else
		{
			ra305x_mii_writereg(i, 29, 0x7058);   //TX100/TX10 AD/DA current bias
			ra305x_mii_writereg(i, 30, 0x0018);   //TX100 slew rate control
		}
#elif defined (CONFIG_RT3352_ASIC) || defined (CONFIG_RT5350_ASIC) ||defined (CONFIG_MT7628_ASIC)
   	    ra305x_mii_writereg(i, 29, 0x7015);   	//TX100/TX10 AD/DA current bias
		ra305x_mii_writereg(i, 30, 0x0038);   	//TX100 slew rate control
#else
#error "Chip is not supported"
#endif
        }

    /* PHY IOT */
    ra305x_mii_writereg(0, 31, 0x0);		//select global register
    ra305x_mii_writereg(0, 1, 0x4a40);		//enlarge agcsel threshold 3 and threshold 2
	ra305x_mii_writereg(0, 2, 0x6254);		//enlarge agcsel threshold 5 and threshold 4
	ra305x_mii_writereg(0, 3, 0xa17f);		//enlarge agcsel threshold 6

#if defined (CONFIG_RT3052_ASIC)
	rw_rf_reg(0, 0, &phy_val);
	phy_val = phy_val >> 4;
	if(phy_val > 0x5)
	{
	    ra305x_mii_writereg(0, 12, 0x7eaa);
	    ra305x_mii_writereg(0, 22, 0x252f); 	//tune TP_IDL tail and head waveform, enable power down slew rate control
	    ra305x_mii_writereg(0, 27, 0x2fda); 	//set PLL/Receive bias current are calibrated
	}
	else
	{
		ra305x_mii_writereg(0, 22, 0x052f); 	//tune TP_IDL tail and head waveform
		ra305x_mii_writereg(0, 27, 0x2fce); 	//set PLL/Receive bias current are calibrated
	}
#elif defined (CONFIG_RT3352_ASIC)
	ra305x_mii_writereg(0, 12, 0x7eaa);
	ra305x_mii_writereg(0, 22, 0x252f); 		//tune TP_IDL tail and head waveform, enable power down slew rate control
	ra305x_mii_writereg(0, 27, 0x2fda); 		//set PLL/Receive bias current are calibrated
#elif defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)

    ra305x_mii_writereg(0, 12, 0x7eaa);
	ra305x_mii_writereg(0, 22, 0x253f);             //tune TP_IDL tail and head waveform, enable power down slew rate control
	ra305x_mii_writereg(0, 27, 0x2fda);             //set PLL/Receive bias current are calibrated
#else
#error "Chip is not supported"
#endif
   	ra305x_mii_writereg(0, 14, 0x65);   		//longer TP_IDL tail length
	ra305x_mii_writereg(0, 16, 0x0684);  		//increased squelch pulse count threshold
    ra305x_mii_writereg(0, 17, 0x0fe0);		//set TX10 signal amplitude threshold to minimum
    ra305x_mii_writereg(0, 18, 0x40ba);		//set squelch amplitude to higher threshold        
	ra305x_mii_writereg(0, 28, 0xc410);             //change PLL/Receive bias current to internal
	ra305x_mii_writereg(0, 29, 0x598b);             //change PLL bias current to internal
    ra305x_mii_writereg(0, 31, 0x8000);		//select local register

	for(i=0;i<5;i++)
     {
		//LSB=1 enable PHY
		phy_val = ra305x_mii_readreg(i, 26);
		phy_val |= 0x0001;
		ra305x_mii_writereg(i, 26, phy_val);
	 }
#endif
	DBGPRINTF(DBG_INITMORE, "ifra305x: ese init done\n");
}
#endif // #if defined(CONFIG_RT7620_ASIC)

#if defined(CONFIG_RT7620_ASIC)
/*=============================================================*
 *  ra305x_esw_vlancfg:
 *=============================================================*/
static int ra305x_esw_vlancfg(if_ra305x_t *pifra305x)
{
	int i;
	cyg_uint32 regval;
	cyg_uint32 idx = pifra305x->vid - 1;
	cyg_uint32 portmask_r = 0;

	/* Set PVID */
	for (i = 0; i < 5; i++)
	{
		if ((1 << (4-i)) & pifra305x->portmask)
		{
			RAESW_REG(i*0x100+0x2014) = (0x10000 + pifra305x->vid);
				portmask_r |= (1 << i);
		}
	}
	
	if (pifra305x->vid == 1)
	{
		RAESW_REG(0x2514) = 0x10001;
		portmask_r |= 1 << 0x5;
	}
	//RAESW_REG(0x2014) = 0x10001;
	//RAESW_REG(0x2114) = 0x10001;
	//RAESW_REG(0x2214) = 0x10001;
	//RAESW_REG(0x2314) = 0x10001;
	//RAESW_REG(0x2414) = 0x10002;
	//RAESW_REG(0x2514) = 0x10001;

	/* VLAN member port */
	regval =  RAESW_REG(0x100+4*(idx/2));
	if (idx % 2 == 0) {
		regval &= 0xfff000;
		regval |= pifra305x->vid;
	} else {
		regval &= 0xfff;
		regval |= (pifra305x->vid << 12);
	}
	RAESW_REG(0x100+4*(idx/2)) = regval;

	/* Set VLAN member */
	DBGPRINTF(DBG_INITMORE, "ra305x_esw_vlancfg: portmask=0x%x\n", pifra305x->portmask);

	if (pifra305x->vid == 1)
	{
		//RAESW_REG(0x94) = 0x40ef0001;
		RAESW_REG(0x94) = 0x40c00001 + (portmask_r << 16);
		DBGPRINTF(DBG_INITMORE, "40ef0001, portmask=%x\n", pifra305x->portmask);
	}
	if (pifra305x->vid == 2)
	{
		//RAESW_REG(0x94) = 0x40d00001;
		RAESW_REG(0x94) = 0x40c00001 + (portmask_r << 16);
		DBGPRINTF(DBG_INITMORE, "40d00001, portmask=%x\n", pifra305x->portmask);
	}
	//RAESW_REG(0x94) = (pifra305x->portmask << 16) | 0x1;
	RAESW_REG(0x90) = (0x80001000 + idx);
	
	do {
		regval = RAESW_REG(0x90);
	} while ((regval & (1<<31)));

	return 0;
}
#else
/*=============================================================*
 *  ra305x_esw_vlancfg:
 *=============================================================*/
static int ra305x_esw_vlancfg(if_ra305x_t *pifra305x)
{
	int idx, pos, i;
	cyg_uint32 regval;
	
	
	/*  Set VID  */
	idx = pifra305x->unit / 2;
	pos = pifra305x->unit & 1;
	regval = RAESW_REG(RAESW_VLANID_REG_BASE + idx*sizeof(cyg_uint32));
	regval &= ~(RAESW_VLANID_MASK << (pos * RAESW_VLANID_SHIFT));
	regval |= pifra305x->vid << (pos * RAESW_VLANID_SHIFT);
	RAESW_REG(RAESW_VLANID_REG_BASE + idx*sizeof(cyg_uint32)) = regval;
	
	/*  Set VLAN member  */
	idx = pifra305x->unit / 4;
	pos = pifra305x->unit % 4;
	regval = RAESW_REG(RAESW_VLANMEM_REG_BASE + idx*sizeof(cyg_uint32));
	regval &= ~(RAESW_VLANMEM_MASK << (pos * RAESW_VLANMEM_SHIFT));
	regval |= (pifra305x->portmask | (1 << RAESW_CPU_PORT)) << (pos * RAESW_VLANMEM_SHIFT);
	RAESW_REG(RAESW_VLANMEM_REG_BASE + idx*sizeof(cyg_uint32)) = regval;
	
	/*  Set PVID  */
	for (i=0; i< RAESW_CPU_PORT; i++)
    {
		if (!(pifra305x->portmask & (1 << i)))
			continue;
		idx = i/2;
		pos = i & 1;
		regval = RAESW_REG(RAESW_PVID_REG_BASE + idx*sizeof(cyg_uint32));
		regval &= ~(RAESW_PVID_MASK << (pos * RAESW_PVID_SHIFT));
		regval |= pifra305x->vid << (pos * RAESW_PVID_SHIFT);
		RAESW_REG(RAESW_PVID_REG_BASE + idx*sizeof(cyg_uint32)) = regval;
	}
	
	return 0;
}
#endif

/*=============================================================*
 *  ra305x_esw_setphymode:
 *=============================================================*/
static int ra305x_esw_setphymode(int port, int mode)
{
	int anar, old_cap, new_cap;

	anar = ra305x_mii_readreg(port, PHY_AUTONEG_ADVERT);
	old_cap = anar & PHY_AUTONEG_CAP_MASK;

	switch (mode)
	{
	case 1:
		new_cap = PHY_AUTONEG_10BASET_HDX ;
		break;
		
	case 2:
		new_cap = (PHY_AUTONEG_10BASET_HDX  | PHY_AUTONEG_10BASET_FDX);
		break;
	
	case 3:
		new_cap = PHY_AUTONEG_100BASETX_HDX;
		break;
		
	case 4:
		new_cap = (PHY_AUTONEG_100BASETX_HDX | PHY_AUTONEG_100BASETX_FDX);
		break;

	case 0:
	default:
		new_cap = (PHY_AUTONEG_10BASET_HDX |
			PHY_AUTONEG_10BASET_FDX    |
			PHY_AUTONEG_100BASETX_HDX  |
			PHY_AUTONEG_100BASETX_FDX );
		break;
	}
	if (old_cap == new_cap)
		return 0;

	anar &= ~PHY_AUTONEG_CAP_MASK;
	anar |= new_cap;

	ra305x_mii_writereg(port, PHY_AUTONEG_ADVERT, anar);

	/*  Restart AN  */
	ra305x_mii_writereg(port, PHY_CONTROL_REG, 0x3300);

	return 0;
}


/*=============================================================*
 *  ra305x_hwinit:
 *=============================================================*/
static int ra305x_hwinit(void)
{
	cyg_uint32 reg;
	
	DBGPRINTF(DBG_INITMORE, "ifra305x: hwinit\n");
	
	RAFE_REG(RAFE_FE_INT_ENABLE) = RAFE_INT_NONE;
	
	ra305x_esw_init();

	/*  Wait until Rx/Tx DMA stop  */
	do {
		reg = RAFE_REG(RAFE_PDMA_GLO_CFG);
		if (reg & RAFE_PDMA_RX_DMA_BUSY)
			DBGPRINTF(DBG_ERR, "ifra305x: rxdma busy\n");
		if (reg & RAFE_PDMA_TX_DMA_BUSY)
			DBGPRINTF(DBG_ERR, "ifra305x: txdma busy\n");
	} while (reg & (RAFE_PDMA_RX_DMA_BUSY | RAFE_PDMA_TX_DMA_BUSY));

	RAFE_REG(RAFE_FE_INT_STATUS) = RAFE_INT_ALL;
	
	reg = RAFE_REG(RAFE_PDMA_GLO_CFG) & 0xff;
	RAFE_REG(RAFE_PDMA_GLO_CFG) = reg;
	reg = RAFE_REG(RAFE_PDMA_GLO_CFG);
	
	/*  Setup Tx machine  */
	RAFE_REG(RAFE_TX_BASE_PTR0) = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)pra305x->tx0.ptxd);
	RAFE_REG(RAFE_TX_MAX_CNT0) = pra305x->tx0.num_txd;
	RAFE_REG(RAFE_TX_CTX_IDX0) = 0;
	RAFE_REG(RAFE_PDMA_RST_CFG) = RAFE_PDMA_PST_DTX_IDX0;
	
	/*  Setup Rx machine  */
	RAFE_REG(RAFE_RX_BASE_PTR0) = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)pra305x->rx0.prxd);
	RAFE_REG(RAFE_RX_MAX_CNT0) = pra305x->rx0.num_rxd;
	RAFE_REG(RAFE_RX_CALC_IDX0) = pra305x->rx0.num_rxd - 1;
	RAFE_REG(RAFE_PDMA_RST_CFG) = RAFE_PDMA_PST_DRX_IDX0;
	
	ra305x_set_fe_pdma_glo_cfg();
	
	ra305x_forward_cfg();
	DBGPRINTF(DBG_INITMORE, "ifra305x: hwinit done\n");
	
	return 0;
}


/*=============================================================*
 *  ra305x_init:
 *=============================================================*/
static int ra305x_init(void)
{
	cyg_uint8 *pucmem;
	cyg_uint8 *pbuf;
	int i;
	
	DBGPRINTF(DBG_INIT, "ifra305x: driver init\n");
	
	/*  Reset FE  */
	ra305x_fe_reset();

	pra305x = &ra305xobj;
	memset(pra305x, 0, sizeof(eth_ra305x_t));
	
	/*  Make sure the uncached memory area is not in the cache  */
	HAL_DCACHE_FLUSH(ucmem, UCMEM_SZ);
	pucmem = (cyg_uint8 *) CYGARC_UNCACHED_ADDRESS((cyg_uint32) ucmem);
	/*  Align uncached memory to cache line boundry  */
	pucmem = (cyg_uint8 *)(((cyg_uint32)pucmem + HAL_DCACHE_LINE_SIZE - 1) 
			& ~(HAL_DCACHE_LINE_SIZE - 1));
	
	pra305x->tx0.ptxd = (pdma_txdesc_t *) pucmem;
	pra305x->rx0.prxd = (pdma_rxdesc_t *)(pra305x->tx0.ptxd + NUM_TX_DESC);
	pbuf = (cyg_uint8 *)(pra305x->rx0.prxd + NUM_RX_DESC);

	pra305x->tx0.ptxbuf = bufdesc_pool;
	pra305x->rx0.prxbuf = rxbufdesc_pool;
	
	pra305x->pvtag = (cyg_uint32 *) pbuf;
	pbuf += VLANTAG_BUF_LEN;

	pra305x->pzerobuf = pbuf;
	pbuf += ZEROBUF_LEN;
	memset(pra305x->pzerobuf, 0, ZEROBUF_LEN);
	
	/*  Init ubuf  */
	pra305x->num_ubuf = NUM_UBUF;
	pra305x->pfree_ubuf = NULL;
	pra305x->num_freeubuf = 0;
	for (i=pra305x->num_ubuf; i > 0; i--, pbuf += UBUF_LEN)
		ubuf_free((ubuf_t *)pbuf);
	
	txctrl_init(&pra305x->tx0, RAFE_TX_BASE_PTR0, NUM_TX0_DESC);
	rxctrl_init(&pra305x->rx0, RAFE_RX_BASE_PTR0, NUM_RX0_DESC);
	
	pra305x->int_mask = DEF_RA305X_INT_MASK;
	pra305x->vector = CYGNUM_HAL_INTERRUPT_ETH;
	
	cyg_drv_interrupt_create(pra305x->vector,
			0,				/*  Priority  */
			(CYG_ADDRWORD) pra305x, 	/*  interupt context  */
			ra305x_isr,			/*  ISR function  */
			ra305x_dsr,			/*  DSR function  */
			&pra305x->int_hdl,		/*  interrupt handle  */
			&pra305x->int_obj);		/*  interrupt object  */
	cyg_drv_interrupt_attach(pra305x->int_hdl);
	cyg_drv_interrupt_configure(pra305x->vector, 1, 0);//this function do nothing

	pra305x->esw_linkmask = 0;
	pra305x->esw_int_mask = DEF_RA305X_ESW_INT_MASK;
	pra305x->esw_vector = CYGNUM_HAL_INTERRUPT_ESW;

	cyg_drv_interrupt_create(pra305x->esw_vector,
			0,				/*  Priority  */
			(CYG_ADDRWORD) pra305x, 	/*  interupt context  */
			ra305x_esw_isr,			/*  ISR function  */
			ra305x_esw_dsr,			/*  DSR function  */
			&pra305x->esw_int_hdl,		/*  interrupt handle  */
			&pra305x->esw_int_obj);		/*  interrupt object  */
	cyg_drv_interrupt_attach(pra305x->esw_int_hdl);
	cyg_drv_interrupt_configure(pra305x->esw_vector, 1, 0);
	
	ra305x_hwinit();

	cyg_drv_interrupt_unmask(pra305x->vector);
	cyg_drv_interrupt_unmask(pra305x->esw_vector);

	DBGPRINTF(DBG_INITMORE, "ifra305x: driver init done\n");
	return 0;
}

#ifdef CONFIG_IPTV //iptv support ,add by ziqiang.yu
static int iptv_portmask(void)
{

#ifdef CONFIG_IPTV_FIX_PORT
	return CONFIG_RA305X_IPTV_PORTMASk;
#else

	int  iptv_port=0,return_mask=0;

	CFG_get(CFG_IPTV_PORT, &iptv_port);

	if (iptv_port  & 0x1)
		return_mask |= CONFIG_RA305X_PORT1_MASK;
	if (iptv_port  & 0x2)
		return_mask |= CONFIG_RA305X_PORT2_MASK;
	if (iptv_port  & 0x4)
		return_mask |= CONFIG_RA305X_PORT3_MASK;
	if (iptv_port  & 0x8)
		return_mask |= CONFIG_RA305X_PORT4_MASK;
	return return_mask;
#endif

}
#endif
/*=============================================================*
 *  ra305x_addifp:
 *=============================================================*/
 extern int opmode;
static int ra305x_addifp(int unit, if_ra305x_t *pifra305x)
{
	if_ra305x_t *pif;
	cyg_uint16 *pvtag;
#ifdef CONFIG_IPTV
	int iptv_enable;
	int iptv_port;
#endif
	char line[100], *p;

	CFG_get_str(CFG_SYS_OPMODE, line);
	opmode = strtol(line, &p, 10);

	if (unit<0 || unit >=CYGNUM_DEVS_ETH_MIPS_RA305X_DEV_COUNT)
		return -1;
	
	for (pif=pra305x->iflist; pif != NULL; pif=pif->next) {
		if (pif->unit == unit)
			return -1;
	}
	pifra305x->ptxvtag = (cyg_uint8 *) &pra305x->pvtag[unit];
	pifra305x->next = pra305x->iflist;
	pra305x->iflist = pifra305x;
	pra305x->if_cnt++;

#ifdef CONFIG_IPTV //iptv support ,add by ziqiang.yu
	CFG_get(CFG_IPTV_EN, &iptv_enable);
	diag_printf("IPTV enable =%d",iptv_enable);

	if( (iptv_enable == 1) && (opmode == 1))
	{
	iptv_port = iptv_portmask();
		//clean IPTV port from LAN VLAN group
		if (pifra305x->init_portmask == CONFIG_RA305X_LAN_PORTMASK)
			{
		pifra305x->portmask = (pifra305x->init_portmask & (~iptv_port));
		diag_printf("LAN port mask =%x\n",pifra305x->portmask);
			}
		//add IPTV port to WAN VLAN group
		if (pifra305x->init_portmask == CONFIG_RA305X_WAN_PORTMASK)
			{
			pifra305x->portmask = (pifra305x->init_portmask | (iptv_port));
			diag_printf("WAN  port mask =%x\n",pifra305x->portmask);
			}
	}
	else 
#endif
	if (opmode ==3 || opmode ==2)//set LAN and WAN to one VLAN
	{
		if (pifra305x->init_portmask == CONFIG_RA305X_LAN_PORTMASK)
		{
		pifra305x->portmask = (pifra305x->init_portmask| (CONFIG_RA305X_WAN_PORTMASK));
		diag_printf("LAN port mask =%x\n",pifra305x->portmask);
		}
	
		if (pifra305x->init_portmask == CONFIG_RA305X_WAN_PORTMASK)
		{
			pifra305x->portmask = (pifra305x->init_portmask & (~CONFIG_RA305X_WAN_PORTMASK));
			diag_printf("WAN  port mask =%x\n",pifra305x->portmask);
		}

	} 
	else
	pifra305x->portmask = pifra305x->init_portmask;
#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
        {
                cyg_uint16 reverse_portmask = 0;
                /* RT5350 WAN designs in the leftest port, others design in the rightest port. */
                reverse_portmask |= ((pifra305x->portmask & 0x1) << 4);
                reverse_portmask |= ((pifra305x->portmask & 0x2) << 2);
                reverse_portmask |= ((pifra305x->portmask & 0x4));
                reverse_portmask |= ((pifra305x->portmask & 0x8) >> 2);
                reverse_portmask |= ((pifra305x->portmask & 0x10) >> 4);
                pifra305x->portmask = reverse_portmask;
        }
#endif /* CONFIG_RT5350_ASIC */ 
	ra305x_esw_vlancfg(pifra305x);
	pvtag = (cyg_uint16 * ) pifra305x->ptxvtag;
	*pvtag++ = htons(ETHERTYPE_VLAN);
	*pvtag = htons(pifra305x->vid);
	
	return 0;
}


/*=============================================================*
 *  ra305x_startifp:
 *=============================================================*/
static void ra305x_startifp(if_ra305x_t *pifra305x)
{
	cyg_uint16 if_bit;
	
	if (pra305x== NULL)
		return ;
	DBGPRINTF(DBG_INITMORE, "ifra305x%d: start ifp, actmask=%02x\n", 
		pifra305x->unit, pra305x->act_ifmask);

	if_bit = 1 << pifra305x->unit;
	if (pra305x->act_ifmask & if_bit)
		return;
	
	if (pra305x->act_ifmask == 0) {
		/*  The first active interface  */
		pra305x->sc_main = pifra305x->sc;
		
		DBGPRINTF(DBG_INITMORE, "ifra305x%d: start FE\n", pifra305x->unit);
		/*  Start FE  */
		/*  Enable interrupt  */
		RAFE_REG(RAFE_FE_INT_ENABLE) = pra305x->int_mask;
		cyg_drv_interrupt_unmask(pra305x->vector);
		
		/*  Enable switch interrupt  */
		RAESW_REG(RAESW_INTMASK_REG) &= ~pra305x->esw_int_mask;
		cyg_drv_interrupt_unmask(pra305x->esw_vector);
	}                                                                
	pra305x->act_ifmask |= if_bit;
}


/*=============================================================*
 *  ra305x_stopifp:
 *=============================================================*/
static void ra305x_stopifp(if_ra305x_t *pifra305x)
{                                           
	cyg_uint16 if_bit;
	if_ra305x_t *pif;

	if (pra305x== NULL)
		return ;
	if_bit = 1 << pifra305x->unit;;
	if (!(pra305x->act_ifmask & if_bit))
		return;
	
	pra305x->act_ifmask &= ~if_bit;
	if (pra305x->act_ifmask == 0) {
		/*  No more active interface */
		/*  Stop FE  */
		DBGPRINTF(DBG_INITMORE, "ifra305x%d: stop FE\n", pifra305x->unit);

		RAFE_REG(RAFE_FE_INT_ENABLE) = RAFE_INT_NONE;
		cyg_drv_interrupt_mask(pra305x->vector);

		/*  Disable switch interrupt  */
		RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;
		cyg_drv_interrupt_mask(pra305x->esw_vector);

		pra305x->sc_main = NULL;
		return;
	}
	if (pra305x->sc_main == pifra305x->sc) {
		for (pif=pra305x->iflist; pif != NULL; pif = pif->next) {
			if (pra305x->act_ifmask & (1 << pif->unit)) {
				pra305x->sc_main = pif->sc;
				break;
			}
		}
	}
}


#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN		0x8100	/* VLAN tag */
#endif
/*=============================================================*
 *  ra305x_stopifp:
 *=============================================================*/
static if_ra305x_t *ra305x_lookupifp(cyg_uint8 *pbuf)
{
	if_ra305x_t *pif;
	struct ether_header *eh;
	cyg_uint16 vid;
	
	eh = (struct ether_header *) pbuf;
	if (ntohs(eh->ether_type) == ETHERTYPE_VLAN) {
		vid = ntohs(*((cyg_uint16 *)(eh+1))) & 0x3ff;
	}
	else 
		return NULL;
	if (pra305x== NULL)
		return ;
	/*  Find interface by vid  */
	for (pif=pra305x->iflist; pif != NULL; pif = pif->next) {
		
	//	diag_printf("VLAN ID:VID=%d,unit=%d\n\n",vid,pif->unit);
		if ((pif->vid == vid) && (pra305x->act_ifmask & (1 << pif->unit))) {
			return pif;
			
		}
	}
	return NULL;
}


/*=====================================================================*/
/*  Interrupt handling                                                 */

/*=============================================================*
 *  ra305x_isr:
 *=============================================================*/
static cyg_uint32 ra305x_isr(cyg_vector_t vec, cyg_addrword_t data)
{
	/*  Disable futher interupt from the device  */
	RAFE_REG(RAFE_FE_INT_ENABLE) = RAFE_INT_NONE;
	
	DBGPRINTF(DBG_ISR, "ifra305x: isr\n");
	cyg_drv_interrupt_acknowledge(vec);
	
	return CYG_ISR_CALL_DSR;	/*  schedule DSR  */
}


/*=============================================================*
 *  ra305x_dsr:
 *=============================================================*/
static void ra305x_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
	cyg_uint32 intstatus;

	DBGPRINTF(DBG_ISR, "ifra305x: dsr\n");
	
	if (pra305x== NULL)
		return ;
	if (pra305x->sc_main != NULL) {
		/*  schecdule deliver routine  */
		eth_drv_dsr(vec, count, (cyg_addrword_t) pra305x->sc_main);
	} else {
		DBGPRINTF(DBG_ISR, "ifra305x: early dsr\n");

		/*  Ack the interrupt  */
		intstatus = RAFE_REG(RAFE_FE_INT_STATUS);
		RAFE_REG(RAFE_FE_INT_STATUS) = intstatus;

		/*  re-enable interrupt  */
		RAFE_REG(RAFE_FE_INT_ENABLE) = pra305x->int_mask;
	}
}


/*=============================================================*
 *  ra305x_esw_isr:
 *=============================================================*/
static cyg_uint32 ra305x_esw_isr(cyg_vector_t vec, cyg_addrword_t data)
{
	/*  Disable futher interupt from the device  */
	RAESW_REG(RAESW_INTMASK_REG) = RA305X_ESW_INT_ALL;
	DBGPRINTF(DBG_ISR, "ifra305x: esw isr\n");
	cyg_drv_interrupt_acknowledge(vec);
	
	return CYG_ISR_CALL_DSR;	/*  schedule DSR  */
}


#ifdef CONFIG_BRIDGE
static void rtbr_del(void);
#endif

#if defined(CONFIG_RT7620_ASIC)
/*=============================================================*
 *  ra305x_dsr:
 *=============================================================*/
static void ra305x_esw_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
	cyg_uint32 intstatus, port_no, reg_val;

	DBGPRINTF(DBG_ISR, "ifra305x: dsr\n");

	/*  Read interrupt status and Ack */
	intstatus = RAESW_REG(RAESW_INTSTATUS_REG);
	RAESW_REG(RAESW_INTSTATUS_REG) = intstatus;

	if (pra305x== NULL)
		return ;
	for (port_no=0;port_no <= 5;port_no++)
	{
		reg_val =  RAESW_REG(0x3008 + port_no * 0x100);
		if (reg_val & 0x1)
		{
			if (!(pra305x->esw_linkmask & (1 << (4 - port_no))))
			{
				 DBGPRINTF(DBG_INFO,"ESW: Link Status Changed - Port %d Link up\n", port_no);
				pra305x->esw_linkmask |= (1 << (4 - port_no));
#ifdef ETH_DRV_SET_PHY
				mon_snd_cmd(MON_CMD_LINK_CHK);
#endif
#ifdef CONFIG_BRIDGE
				rtbr_del();
#endif
			}
		}
		else
		{
			if (pra305x->esw_linkmask & (1 << (4 - port_no)))
			{
				DBGPRINTF(DBG_INFO,"ESW: Link Status Changed - Port %d Link down\n", port_no);
				pra305x->esw_linkmask &= ~(1 << (4 - port_no));
#ifdef ETH_DRV_SET_PHY
				mon_snd_cmd(MON_CMD_LINK_CHK);
#endif
#ifdef CONFIG_BRIDGE
				rtbr_del();
#endif
			}
		}
	}
	/*  Enable switch interrupt again  */
	RAESW_REG(RAESW_INTMASK_REG) &= ~pra305x->esw_int_mask;
}

#else

int get_port_index(int a)
{
	int i = 0;

	do{
		if(a & 0x1 == 1)
			return i;
		a = a >> 1;
		i++;
	}while(i<8);

	return -1;
}

/*=============================================================*
 *  ra305x_dsr:
 *=============================================================*/
static void ra305x_esw_dsr(cyg_vector_t vec, cyg_ucount32 count, cyg_addrword_t data)
{
	cyg_uint32 new_link, link_chg;
	cyg_uint32 intstatus;

	DBGPRINTF(DBG_ISR, "ifra305x: dsr\n");
	if (pra305x== NULL)
		return ;

	/*  Read interrupt status and Ack */
	intstatus = RAESW_REG(RAESW_INTSTATUS_REG);
	RAESW_REG(RAESW_INTSTATUS_REG) = intstatus;

	if (intstatus & RAESW_INT_PORT_ST_CHG)
    {
		new_link = (RAESW_REG(RAESW_PORT_ABILITY_REG) >> RAESW_PHY_LINK_SHIFT) & RAESW_PHY_PORT_MASK;
		link_chg = pra305x->esw_linkmask ^ new_link;
		pra305x->esw_linkmask = new_link;

		if (link_chg)
        {
			diag_printf("ifra305x: Port%d  Link %s\n", get_port_index(link_chg), new_link? "Up": "Down");
#ifdef ETH_DRV_SET_PHY
            mon_snd_cmd(MON_CMD_LINK_CHK);
#endif
#ifdef CONFIG_BRIDGE
            rtbr_del();
#endif
		}
	}
	/*  Enable switch interrupt again  */
	RAESW_REG(RAESW_INTMASK_REG) &= ~pra305x->esw_int_mask;

}
#endif

/*=============================================================*
 *  ra305x_rxint:
 *=============================================================*/
static void ra305x_rxint(rxctrl_t *prxc)
{
	volatile pdma_rxdesc_t *prxd;
	if_ra305x_t *pifra305x;
	int frame_len, loopcnt = 0;
	cyg_uint32 rx_status;
#ifdef ETH_DRV_RXMBUF
	struct mbuf *m0, *m;
#endif

	if (pra305x== NULL)
		return ;
	DBGPRINTF(DBG_RX, "ifra305x: rxint, rxc=%08x, rxhead=%d calc_idx=%d drx_idx=%d\n",
		(cyg_uint32) prxc, prxc->rx_head, 
		RAFE_REG(prxc->rx_base+RAFE_RX_CALC_IDX), 
		RAFE_REG(prxc->rx_base+RAFE_RX_DRX_IDX));
	
	pra305x->act_rx = prxc;
	
	prxd = &prxc->prxd[prxc->rx_head];
	rx_status = prxd->rx_status;
	while ((rx_status & RX_DDONE) && (loopcnt++ < prxc->num_rxd)) {
		frame_len = RX_PLEN0(rx_status);
		ADD_PRIVATE_COUNTER(prxc->rx_pktcnt, 1);
		ADD_PRIVATE_COUNTER(prxc->rx_bytecnt, frame_len);
		if (frame_len < ETH_MIN_FRAMELEN) {
			ADD_PRIVATE_COUNTER(prxc->rx_tooshort, 1);
			goto next_rx;
		}
#ifdef ETH_DRV_RXMBUF
		m0 = (struct mbuf *) prxc->prxbuf[prxc->rx_head];
		if (m0  && ((pifra305x = ra305x_lookupifp((cyg_uint8 *) m0->m_data)) != NULL)) {
			m = ra305x_getbuf();
			if (m != NULL) {
				/*  Indicate the new frame  */
				ra305x_indicate_rx(pifra305x, m0, frame_len, 1);
	
				/*  Invalidate the new buf area  */
				HAL_DCACHE_INVALIDATE(m->m_data, UBUF_LEN);
				prxd->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)m->m_data);
				prxc->prxbuf[prxc->rx_head] = (cyg_uint32) m;
			}
			else {
				ADD_PRIVATE_COUNTER(prxc->rx_drop, 1);
			}
		} else {
			DBGPRINTF(DBG_RXMORE, "ifra305x: rx noifp!!\n");
			ADD_PRIVATE_COUNTER(prxc->rx_noifp, 1);
		}
#else
		pifra305x = ra305x_lookupifp((cyg_uint8 *)(prxc->prxbuf[prxc->rx_head]+RA305X_RXBUF_OFFSET));
		if (pifra305x != NULL)
			(pifra305x->sc->funs->eth_drv->recv)(pifra305x->sc, frame_len - ETH_VLANTAG_LEN);
		else {
			DBGPRINTF(DBG_RXMORE, "ifra305x: rx noifp!!\n");
			ADD_PRIVATE_COUNTER(prxc->rx_noifp, 1);
		}
#endif

next_rx:
		/*  Release the descriptor to DMA  */
#if defined(CONFIG_RAETH_SCATTER_GATHER_RX_DMA)
		prxd->rx_status = UBUF_LEN << 16;
#else
		prxd->rx_status = RX_LS0;
#endif
		RAFE_REG(prxc->rx_base+RAFE_RX_CALC_IDX) = prxc->rx_head;
		if (++prxc->rx_head == prxc->num_rxd)
			prxc->rx_head = 0;
		prxd = &prxc->prxd[prxc->rx_head];
		rx_status = prxd->rx_status;
	}
	DBGPRINTF(DBG_RXMORE, "ifra305x: rxint end, rxc=%08x, rxhead=%d, calc_idx=%d, drx_idx=%d\n", 
			(cyg_uint32) prxc, prxc->rx_head, 
			RAFE_REG(prxc->rx_base+RAFE_RX_CALC_IDX),
			RAFE_REG(prxc->rx_base+RAFE_RX_DRX_IDX));


}
 
 
/*=============================================================*
 *  ra305x_txint:
 *=============================================================*/
static void ra305x_txint(txctrl_t *ptxc)
{
	pdma_txdesc_t *ptxd;
	bufdesc_t *ptxbuf;
	cyg_uint32 tx_dtx_idx;
	
	if (pra305x== NULL)
		return ;
	tx_dtx_idx = RAFE_REG(ptxc->tx_base+RAFE_TX_DTX_IDX);
	DBGPRINTF(DBG_TX, "ifra305x: txint, txc=%08x, txtail=%d ctxidx=%d dxidx=%d\n", 
			(cyg_uint32) ptxc, ptxc->tx_tail,
			RAFE_REG(ptxc->tx_base+RAFE_TX_CTX_IDX),
			RAFE_REG(ptxc->tx_base+RAFE_TX_DTX_IDX));

	ptxd = ptxc->ptxd;
	ptxbuf = ptxc->ptxbuf;
	
	while((ptxd[ptxc->tx_tail].tx_buf & TX_DDONE) && ptxc->num_freetxd < ptxc->num_txd) {
		if (ptxbuf[ptxc->tx_tail].buf != NULL) {
			if (ptxbuf[ptxc->tx_tail].sc != NULL) {
				/*  sg_list */
				/*  Call tx_done to release the frame  */
                                ra305x_eth_drv_tx_done(
                                ptxbuf[ptxc->tx_tail].sc, (cyg_uint32) ptxbuf[ptxc->tx_tail].buf, 0);
			} else {
				/*  Release Ubuf */
				ubuf_free((ubuf_t *)ptxbuf[ptxc->tx_tail].buf);
			}
			ptxbuf[ptxc->tx_tail].sc = NULL;
			ptxbuf[ptxc->tx_tail].buf = NULL;
		}
		ptxc->num_freetxd++;
		if (++ptxc->tx_tail >= ptxc->num_txd)
			ptxc->tx_tail = 0;
	}

	DBGPRINTF(DBG_TXMORE, "ifra305x: txint end, txc=%08x, txtail=%d, ctxidx=%d, dtxidx=%d\n", 
			(cyg_uint32) ptxc, ptxc->tx_tail, 
			RAFE_REG(ptxc->tx_base+RAFE_TX_CTX_IDX),
			RAFE_REG(ptxc->tx_base+RAFE_TX_DTX_IDX));
	
}

 
 
/*=====================================================================*/
/*  Interface functions                                                */

/*=============================================================*
 *  if_ra305x_init:
 *=============================================================*/
static bool if_ra305x_init(struct cyg_netdevtab_entry *pnde)
{
	struct eth_drv_sc *sc;
	if_ra305x_t *pifra305x;
	int unit;
	
	sc = (struct eth_drv_sc *) (pnde->device_instance);
	pifra305x = (if_ra305x_t *) sc->driver_private;
	unit = pifra305x->unit;

	DBGPRINTF(DBG_INFO, "ifra305x eth%d: init pnde=%08x, sc=%08x, ifp=%08x\n",
		unit, (cyg_uint32)pnde, (cyg_uint32) sc, (cyg_uint32) pifra305x);
	
	if ((pra305x == NULL) && ra305x_init() != 0) {
		DBGPRINTF(DBG_ERR, "ifra305x eth%d: init failed\n", unit);
		return false;
	}

#ifdef BRANCH_ADV
    // Read MAC value from flash
	CFG_get_mac(unit, pifra305x->macaddr);
#endif // BRANCH_ADV //
	
	pifra305x->sc = sc;
	if (ra305x_addifp(unit, pifra305x) < 0)
		goto err_out;
	
	if (eth_restart== 0)
	(sc->funs->eth_drv->init)(sc, pifra305x->macaddr);
	return true;
	
err_out:
	return false;	
}


/*=============================================================*
 *  if_ra305x_start:
 *=============================================================*/
static void if_ra305x_start(struct eth_drv_sc *sc, unsigned char *enaddr, int flags)
{
	if_ra305x_t *pifra305x;

	pifra305x = (if_ra305x_t *) sc->driver_private;
	CYG_ASSERTC(pifra305x->sc == sc);
	
//	DBGPRINTF(DBG_INFO, "ifra305x%d: start\n", pifra305x->unit);
	diag_printf("ifra305x%d: start\n", pifra305x->unit);
	
	/*  Start hardware  */
	ra305x_startifp(pifra305x);
}


/*=============================================================*
 *  if_ra305x_stop:
 *=============================================================*/
static void if_ra305x_stop(struct eth_drv_sc *sc)
{
	if_ra305x_t *pifra305x;

	pifra305x = (if_ra305x_t *) sc->driver_private;
	CYG_ASSERTC(pifra305x->sc == sc);
	
	DBGPRINTF(DBG_INFO, "ifra305x%d: stop\n", pifra305x->unit);

	/*  Stop hardware  */
	ra305x_stopifp(pifra305x);
}


/*=============================================================*
 *  if_ra305x_ioctl:
 *=============================================================*/
static int if_ra305x_ioctl(struct eth_drv_sc *sc, unsigned long key, void *data, int data_len)
{
	if_ra305x_t *pifra305x;
		if (pra305x== NULL)
		return -2;
#if defined(ETH_DRV_GET_IF_STATS_UD) || defined(ETH_DRV_GET_IF_STATS)
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
#endif

	pifra305x = (if_ra305x_t *) sc->driver_private;
	CYG_ASSERTC(pifra305x->sc == sc);
	
	DBGPRINTF(DBG_IOCTL, "ifra305x%d: ioctl=%08lx\n", pifra305x->unit, key);
	
	switch(key)
	{
#ifdef ETH_DRV_SET_MAC_ADDRESS
	case ETH_DRV_SET_MAC_ADDRESS:
		if (data_len != 6)
			return -2;
		return 0;
#endif 	/*  ETH_DRV_SET_MAC_ADDRESS  */

#ifdef ETH_DRV_GET_IF_STATS_UD
	case ETH_DRV_GET_IF_STATS_UD:
#endif	/*  ETH_DRV_GET_IF_STATS_UD  */

#ifdef ETH_DRV_GET_IF_STATS
	case ETH_DRV_GET_IF_STATS:
#endif /*  ETH_DRV_GET_IF_STATS  */

#if defined(ETH_DRV_GET_IF_STATS_UD) || defined(ETH_DRV_GET_IF_STATS)
	{
		struct ether_drv_stats *p = (struct ether_drv_stats *)data;

		memset(p, 0, sizeof(struct ether_drv_stats));
		p->operational = 1;
		p->duplex = 3;
		p->speed = 100000000;

#ifdef KEEP_EXTRA_STATS
		p->supports_dot3        = true;

		// Those commented out are not available on this chip.

		p->tx_good = ifp->if_opackets - ifp->if_oerrors;
		// p->tx_max_collisions = 0;
		// p->tx_late_collisions = 0;
		// p->tx_underrun = 0;
		// p->tx_carrier_loss = 0;
		// p->tx_deferred = 0;
		// p->tx_sqetesterrors = 0;
		// p->tx_single_collisions = 0;
		// p->tx_mult_collisions = 0;
		// p->tx_total_collisions = 0;

		p->rx_good = ifp->if_ipackets - ifp->if_ierrors;
		// p->rx_crc_errors = 0;
		// p->rx_align_errors = 0;
		// p->rx_resource_errors = 0;
		// p->rx_overrun_errors = 0;
		// p->rx_collisions = 0;
		// p->rx_short_frames = 0;
		// p->rx_too_long_frames = 0;
		// p->rx_symbol_errors = 0;
        
		p->interrupts= ifp->if_opackets+ ifp->if_ipackets;
		p->rx_count = ifp->if_ibytes;
		p->rx_deliver = ifp->if_ipackets;
		p->rx_resource = NUM_RX0_DESC;
		// p->rx_restart = 0;

		p->tx_count = ifp->if_Obytes;
		p->tx_complete = ifp->if_ipackets;
		// p->tx_dropped = 0;
		p->tx_queue_len = ifp->ifp_snd.ifq_len;
#endif	/*  KEEP_EXTRA_STATS  */
	}

	return 0;
#endif	/*  ETH_DRV_GET_IF_STATS_UD || ETH_DRV_GET_IF_STATS  */

#ifdef ETH_DRV_SET_MC_ALL
	case ETH_DRV_SET_MC_ALL:
		return 0;
#endif /*  ETH_DRV_SET_MC_ALL  */

#ifdef ETH_DRV_SET_MC_LIST
	case ETH_DRV_SET_MC_LIST:
		return 0;
#endif /*  ETH_DRV_SET_MC_LIST  */

#ifdef ETH_DRV_SET_PHY
#if defined(CONFIG_RT7620_ASIC)
	case SIOCGIFPHY:
	{
		struct ifreq *ifr = (struct ifreq *)data;
		unsigned int *p = (unsigned int *)ifr->ifr_ifru.ifru_data;
		DBGPRINTF(DBG_IOCTL, "SIOCGIFPHY ifra305x%d: portmask=%x, pra305x->esw_linkmask=%x\n", pifra305x->unit, pifra305x->portmask, pra305x->esw_linkmask);
		if (pifra305x->portmask & pra305x->esw_linkmask)
		{
			*p = 1;
		}
		else
		{
			*p = 0;
		}
	}
		return 0;
	case SIOCSIFPHY:
	{
		struct ifreq *ifr= (struct ifreq *)data;
		unsigned int *p = (unsigned int *)ifr->ifr_ifru.ifru_data;
		unsigned int mode = *p;
		int i, port;

		/* Check port changed */
		if (mode > 4)
		{
			mode = 0;
			DBGPRINTF(DBG_ERR, "ifra305x%d: SIOCSIFPHY mode=%d error\n", pifra305x->unit, mode);
		}
		port = pifra305x->portmask;
		for (i=0; i<5; i++) {
			if (port & (1<< (4-i))) {
				ra305x_esw_setphymode(i, mode);
			}
		}
		
	}
		return 0;
#else
	case SIOCGIFPHY:
	{
		struct ifreq *ifr = (struct ifreq *)data;
		unsigned int *p = (unsigned int *)ifr->ifr_ifru.ifru_data;
		cyg_uint32 link;

		link = (RAESW_REG(RAESW_PORT_ABILITY_REG) >> RAESW_PHY_LINK_SHIFT) & RAESW_PHY_PORT_MASK;
		if (link & pifra305x->portmask) {
			/*  at least one port is linked up  */
			*p = 1;
		} else {
			*p = 0;
		}
	}
		return 0;
	case SIOCSIFPHY:
	{
		struct ifreq *ifr= (struct ifreq *)data;
		unsigned int *p = (unsigned int *)ifr->ifr_ifru.ifru_data;
		unsigned int mode = *p;
		int i, port;

		/* Check port changed */
		if (mode > 4)
		{
			mode = 0;
			DBGPRINTF(DBG_ERR, "ifra305x%d: SIOCSIFPHY mode=%d error\n", pifra305x->unit, mode);
		}
		port = pifra305x->portmask;
		for (i=0; i<5; i++) {
			if (port & (1<< i)) {
				ra305x_esw_setphymode(i, mode);
			}
		}
		
	}
		return 0;
#endif
#endif
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_PROMISC) {
			DBGPRINTF(DBG_IOCTL, "ifra305x%d: promisc mode\n", pifra305x->unit);
		}
		else {
			DBGPRINTF(DBG_IOCTL, "ifra305x%d: non promisc mode\n", pifra305x->unit);
		}
		return 0;

	case SIOCSIFMTU:
		return 0;		
	}

	
	return -1;	
}


/*=============================================================*
 *  if_ra305x_cansend:
 *=============================================================*/
static int if_ra305x_cansend(struct eth_drv_sc *sc)
{
	if_ra305x_t *pifra305x;

	pifra305x = (if_ra305x_t *) sc->driver_private;
	CYG_ASSERT(pifra305x->sc == sc, "Invalid device pointer");
	
	DBGPRINTF(DBG_TXMORE, "ifra305x%d: cansend=%d\n", pifra305x->unit, pra305x->tx0.num_freetxd);

        return( (pra305x!=NULL) && (pra305x->tx0.num_freetxd > 1) ? 1:0);
}


/*=============================================================*
 *  if_ra305x_send:
 *=============================================================*/
static void if_ra305x_send(struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len, int total_len, unsigned long key)
{
	if_ra305x_t *pifra305x;
	txctrl_t *ptxc = NULL;
	pifra305x = (if_ra305x_t *) sc->driver_private;
	volatile pdma_txdesc_t *ptxd, *ptxd_start;
	ubuf_t *u;
	int i, j, desc_cnt, flen;
	struct eth_drv_sg lsg_list[TX_MAX_BUFCNT+4];

	if (pra305x==NULL)
		return ;
	ptxc = &pra305x->tx0;
	
	CYG_ASSERT(pifra305x->sc == sc, "Invalid device pointer");

	DBGPRINTF(DBG_TX, "ifra305x%d: send, key=%08lx, len=%d, sglen=%d\n", 
		pifra305x->unit, key, total_len, sg_len);

	if (ptxc->num_freetxd <= 1 || total_len > 1518)
		goto drop_tx;

	ptxd_start = &ptxc->ptxd[ptxc->tx_head];
		
	if (!(ptxd_start->tx_buf & TX_DDONE)) {
		DBGPRINTF(DBG_ERR, "ifra305x%d: txd%d is not free\n", pifra305x->unit, ptxc->tx_head);
		goto drop_tx;
	}

	/*  Keep at least two freetxd for inserting VLAN tag and zero padding  */
 
       if ((sg_len >TX_MAX_BUFCNT)|| (ptxc->num_freetxd < 5)) {
		/*  Coalesce the segments into a single buffer  */
		if ((u=ubuf_get()) == NULL)
			goto drop_tx;
		DBGPRINTF(DBG_TXMORE, "ifra305x%d: tx ubuf, txd=%08x\n", pifra305x->unit, (cyg_uint32) ptxd_start);
		flen = sg_to_buf(u, UBUF_LEN, sg_list, sg_len, pifra305x);
		total_len = flen;
		if (total_len < ETH_MIN_FRAMELEN) {
#ifndef SKIP_ZEROPADDING
			memset((cyg_uint8 *)u+total_len, 0, ETH_MIN_FRAMELEN-total_len);
#endif
			total_len = ETH_MIN_FRAMELEN;
		}
		ADD_PRIVATE_COUNTER(ptxc->tx_ubuf, 1);
		ptxc->num_freetxd--;
		ptxd_start->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)u);
		ptxd_start->pbuf1 = 0;
#if defined(CONFIG_PDMA_NEW)
//		ptxd_start->tx_ctrl = TX_FP_BMAP(0);
#else
		ptxd_start->tx_ctrl = TX_PN(1) | TX_QN(3);
#endif
		ptxd_start->tx_buf = TX_LS0 | TX_BUF0_LEN(total_len);
		
		ptxc->ptxbuf[ptxc->tx_head].buf = (void *)u;
		ptxc->ptxbuf[ptxc->tx_head].sc = NULL;
		if (++ptxc->tx_head == ptxc->num_txd)
			ptxc->tx_head = 0;
		/*  Kick hardware to send  */
		RAFE_REG(ptxc->tx_base+RAFE_TX_CTX_IDX) = ptxc->tx_head;
		
		/*  Release the sg buf  */
                ra305x_eth_drv_tx_done(sc, key, 0);
	} else {
		for (i=0, j=0, flen=0; i<sg_len; i++, j++) {
			if (sg_list[i].buf < CYGARC_KSEG_UNCACHED_BASE)
				HAL_DCACHE_FLUSH(sg_list[i].buf, sg_list[i].len);
			DBGPRINTF(DBG_TXMORE, "1. ifra305x%d: buf%d=%08x, len=%d\n", pifra305x->unit, 
				i, sg_list[i].buf, sg_list[i].len);
			
			if ((flen <= 2*ETH_ADDR_LEN) && (flen+sg_list[i].len) > 2*ETH_ADDR_LEN) {
				/*  Insert VLAN tag  */
				
				if (flen < 2*ETH_ADDR_LEN) {
					lsg_list[j].buf = sg_list[i].buf;
					lsg_list[j].len = (2*ETH_ADDR_LEN) - flen;
					DBGPRINTF(DBG_TXMORE, "2. ifra305x%d: buf%d=%08x, len=%d\n", pifra305x->unit, 
						j, lsg_list[j].buf, lsg_list[j].len);
					j++;
				}
				lsg_list[j].buf = (cyg_uint32) pifra305x->ptxvtag;
				lsg_list[j].len = ETH_VLANTAG_LEN;
				DBGPRINTF(DBG_TXMORE, "3. ifra305x%d: buf%d=%08x, len=%d\n", pifra305x->unit, 
					j, lsg_list[j].buf, lsg_list[j].len);
				j++;
				lsg_list[j].buf = sg_list[i].buf + (2*ETH_ADDR_LEN) - flen;
				lsg_list[j].len = sg_list[i].len - (2*ETH_ADDR_LEN) + flen;
				flen += ETH_VLANTAG_LEN;
			} else {
				lsg_list[j] = sg_list[i];
			}
			DBGPRINTF(DBG_TXMORE, "4. ifra305x%d: buf%d=%08x, len=%d\n", pifra305x->unit, 
				j, lsg_list[j].buf, lsg_list[j].len);

			flen += sg_list[i].len;
		}
		sg_len = j;

		if (total_len < ETH_MIN_FRAMELEN) {
#ifdef SKIP_ZEROPADDING
			lsg_list[sg_len -1].len += ETH_MIN_FRAMELEN - total_len;
#else
			lsg_list[sg_len].len = ETH_MIN_FRAMELEN - total_len;
			lsg_list[sg_len].buf = (cyg_uint32) pra305x->pzerobuf;
			sg_len ++;
			DBGPRINTF(DBG_TXMORE, "ifra305x%d: tx pad zero, sglen=%d\n", pifra305x->unit, sg_len);
#endif
			total_len = ETH_MIN_FRAMELEN;
		}
		ADD_PRIVATE_COUNTER(ptxc->tx_sglist, 1);

		if (sg_len > 2) {
			ptxd = &ptxc->ptxd[ptxc->tx_head];
			/*  Number of descriptors with two buffers (excluding the first one)  */
			desc_cnt = sg_len / 2 - 1;
			for (i=2; desc_cnt > 0 && i+1 < 8; desc_cnt--, i+=2) {
				/*  Fill descriptor with two buffers  */
				--ptxc->num_freetxd;
				if (++ptxc->tx_head == ptxc->num_txd)
					ptxc->tx_head = 0;
				ptxd = &ptxc->ptxd[ptxc->tx_head];
				ptxd->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)lsg_list[i].buf);
				ptxd->pbuf1 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)lsg_list[i+1].buf);
#if defined(CONFIG_PDMA_NEW)
//				ptxd->tx_ctrl = TX_FP_BMAP(0);
#else
				ptxd->tx_ctrl = TX_PN(1) | TX_QN(3);
#endif
				ptxd->tx_buf = TX_BUF0_LEN(lsg_list[i].len) | TX_BUF1_LEN(lsg_list[i+1].len);
			}
			if (sg_len & 1) {
				--ptxc->num_freetxd;
				if (++ptxc->tx_head == ptxc->num_txd)
					ptxc->tx_head = 0;
				ptxd = &ptxc->ptxd[ptxc->tx_head];
				ptxd->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)lsg_list[i].buf);
				ptxd->pbuf1 = 0;
#if defined(CONFIG_PDMA_NEW)
//				ptxd->tx_ctrl = TX_FP_BMAP(0);
#else
				ptxd->tx_ctrl = TX_PN(1) | TX_QN(3);
#endif
				ptxd->tx_buf = TX_BUF0_LEN(lsg_list[i].len) | TX_LS0;
			} else 
				ptxd->tx_buf |= TX_LS1;
		}
		/*  Fill the first descriptor  */
#if defined(CONFIG_PDMA_NEW)
//		ptxd_start->tx_ctrl = TX_FP_BMAP(0);
#else
		ptxd_start->tx_ctrl = TX_PN(1) | TX_QN(3);
#endif
		ptxd_start->pbuf0 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)lsg_list[0].buf);

		if (sg_len == 1) {
			ptxd_start->pbuf1 = 0;
			ptxd_start->tx_buf = TX_LS0 | TX_BUF0_LEN(total_len);
		} else {
			ptxd_start->pbuf1 = CYGARC_PHYSICAL_ADDRESS((cyg_uint32)lsg_list[1].buf);
			if (sg_len == 2)
				ptxd_start->tx_buf = TX_LS1 | TX_BUF0_LEN(lsg_list[0].len) | TX_BUF1_LEN(lsg_list[1].len);
			else 
				ptxd_start->tx_buf = TX_BUF0_LEN(lsg_list[0].len) | TX_BUF1_LEN(lsg_list[1].len);
		}
		ptxc->ptxbuf[ptxc->tx_head].sc = sc;
		ptxc->ptxbuf[ptxc->tx_head].buf = (void *)key;
		--ptxc->num_freetxd;
		if (++ptxc->tx_head == ptxc->num_txd)
			ptxc->tx_head = 0;

		/*  Kick hardware to send  */
		RAFE_REG(ptxc->tx_base+RAFE_TX_CTX_IDX) = ptxc->tx_head;
	}

	ADD_PRIVATE_COUNTER(ptxc->tx_pktcnt, 1);
	ADD_PRIVATE_COUNTER(ptxc->tx_bytecnt, flen);

	return;
drop_tx:
	/*  No tx resource for this frame  */
	DBGPRINTF(DBG_TX, "ifra305x%d: drop frame key=%08lx\n", 
		pifra305x->unit, key);
	ADD_PRIVATE_COUNTER(ptxc->tx_drop, 1);
        ra305x_eth_drv_tx_done(sc, key, 0);
}


/*=============================================================*
 *  if_ra305x_recv:
 *=============================================================*/
static void if_ra305x_recv(struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len)
{
#ifndef ETH_DRV_RXMBUF
	rxctrl_t *prxc = NULL;
	cyg_uint8 *psrc;
	if_ra305x_t *pifra305x;
	int frame_len, cp_len;
	struct eth_drv_sg *sg_last;
	cyg_uint16 *src, *dst;
	
	if (pra305x == NULL)
		return ;
	prxc = pra305x->act_rx;
	if (sg_len == 0 || sg_list == NULL) {
		/*  Caller is out of buffers  */
		/*  Drop the frame  */
		return;
	}

	frame_len = RX_PLEN0(prxc->prxd[prxc->rx_head].rx_status) - ETH_VLANTAG_LEN;
	
	pifra305x = (if_ra305x_t *) sc->driver_private;
	
	CYG_ASSERTC(frame_len >= ETH_MIN_FRAMELEN);
	
	DBGPRINTF(DBG_RX, "ifra305x%d: recv idx=%d, len=%d, sglen=%d\n",
		pifra305x->unit, prxc->rx_head, frame_len, sg_len);

	/*  move the address field backward  */
	src = (cyg_uint16 *)(prxc->prxbuf[prxc->rx_head] + RA305X_RXBUF_OFFSET);
	dst = (cyg_uint16 *)(prxc->prxbuf[prxc->rx_head] + RA305X_RXBUF_OFFSET + ETH_VLANTAG_LEN);
	dst[5] = src[5];	dst[4] = src[4];	dst[3] = src[3];
	dst[2] = src[2];	dst[1] = src[1];	dst[0] = src[0];
	
	psrc = (cyg_uint8 *) dst;
	for (sg_last=&sg_list[sg_len]; sg_list != sg_last; sg_list++) {
		cp_len = sg_list->len;
		if (cp_len <=0 || sg_list->buf == 0) {
			DBGPRINTF(DBG_RXMORE, "ifra305x%d: recv out of buf\n", pifra305x->unit);
			return;
		}
		if (frame_len < cp_len)
			cp_len = frame_len;
		memcpy((void *)sg_list->buf, psrc, cp_len);
		psrc += cp_len;
		frame_len -= cp_len;
	}
	CYG_ASSERT(frame_len == 0, "total len mismatch");
	CYG_ASSERT(sg_last == sg_list, "sg count mismatch");
#endif /* !ETH_DRV_RXMBUF */
}


/*=============================================================*
 *  if_ra305x_deliver:
 *=============================================================*/
static void if_ra305x_deliver(struct eth_drv_sc *sc)
{
	cyg_uint32 int_status;
	
	if (pra305x== NULL)
		return ;
	int_status = RAFE_REG(RAFE_FE_INT_STATUS);
	/*  ack all interrupts  */
	RAFE_REG(RAFE_FE_INT_STATUS) = int_status;

	DBGPRINTF(DBG_DELIVER, "ifra305x: deliver intstaus=%08x\n", int_status);
	do {
		if (int_status & RAFE_INT_RX_DONE_INT0)
			ra305x_rxint(&pra305x->rx0);
#if defined(CONFIG_RT5350_ASIC) || defined(CONFIG_MT7628_ASIC)
		if (int_status & RAFE_INT_RX_DONE_INT1)
			ra305x_rxint(&pra305x->rx0);
#endif /* CONFIG_RT5350_ASIC */

		if (int_status & RAFE_INT_TX_DONE_INT0)
			ra305x_txint(&pra305x->tx0);
	} while(0);
	/*  enable interrupts  */
	RAFE_REG(RAFE_FE_INT_ENABLE) = pra305x->int_mask;
}


/*=============================================================*
 *  if_ra305x_poll:
 *=============================================================*/
static void if_ra305x_poll(struct eth_drv_sc *sc)
{
	/*  Disable interrupt from device  */
	RAFE_REG(RAFE_FE_INT_ENABLE) = RAFE_INT_NONE;
	
	DBGPRINTF(DBG_DELIVER, "ifra305x: poll\n");
	
	if_ra305x_deliver(sc);
}


/*=============================================================*
 *  if_ra305x_intvector:
 *=============================================================*/
static int if_ra305x_intvector(struct eth_drv_sc *sc)
{
	return pra305x->vector;
}

void if_ra305x_get_eth_count(cyg_uint32* rx_pktcnt, cyg_uint32* rx_bytes,cyg_uint32* tx_pktcnt,cyg_uint32* tx_bytes)
{
	diag_printf("ifra305x: if_ra305x_get_eth_ra305x_t\n");
	*rx_pktcnt=pra305x->rx0.rx_pktcnt;
	*rx_bytes=pra305x->rx0.rx_bytecnt;
	*tx_pktcnt=pra305x->tx0.tx_pktcnt;
	*tx_bytes=pra305x->tx0.tx_bytecnt;
}

void if_ra305x_debug(char *cmd)
{	
	if (!strcmp(cmd,"tx"))
	{
		diag_printf("if_ra305x_debug :: txc=%08x, txhead=%d, txtail=%d, num_txd=%d\nnum_freetxd=%d, tx_base=%08x, ctxidx=%d, dtxidx=%d\n", 
				(cyg_uint32) &pra305x->tx0, pra305x->tx0.tx_head, pra305x->tx0.tx_tail, 
				pra305x->tx0.num_txd, pra305x->tx0.num_freetxd, pra305x->tx0.tx_base, 
				RAFE_REG(pra305x->tx0.tx_base+RAFE_TX_CTX_IDX),
				RAFE_REG(pra305x->tx0.tx_base+RAFE_TX_DTX_IDX));
	}
	else if (!strcmp(cmd,"rx"))
	{
		diag_printf("if_ra305x_debug :: rxc=%08x, rxhead=%d, calc_idx=%d, drx_idx=%d\n", 
				(cyg_uint32) &pra305x->rx0, pra305x->rx0.rx_head, 
				RAFE_REG(pra305x->rx0.rx_base+RAFE_RX_CALC_IDX),
				RAFE_REG(pra305x->rx0.rx_base+RAFE_RX_DRX_IDX));
	}
}

#if defined (CONFIG_RT7620_ASIC)
void dump_phy_reg(int port_no, int from, int to, int is_local, int page_no)
{
        unsigned int i=0;
        unsigned int temp=0;
        unsigned int r31=0;


        if(is_local==0) {

            diag_printf("\n\nGlobal Register Page %d\n",page_no);
            diag_printf("===============");
            r31 |= 0 << 15; //global
            r31 |= ((page_no&0x7) << 12); //page no
            ra305x_mii_writereg(1, 31, r31); //select global page x
            for(i=16;i<32;i++) {
                if(i%8==0) {
                    diag_printf("\n");
                }
                temp = ra305x_mii_readreg(port_no,i);
                diag_printf("%02d: %04X ",i, temp);
            }
        }else {
            diag_printf("\n\nLocal Register Port %d Page %d\n",port_no, page_no);
            diag_printf("===============");
            r31 |= 1 << 15; //local
            r31 |= ((page_no&0x7) << 12); //page no
            ra305x_mii_writereg(1, 31, r31); //select local page x
            for(i=16;i<32;i++) {
                if(i%8==0) {
                    diag_printf("\n");
                }
                temp = ra305x_mii_readreg(port_no,i);
                diag_printf("%02d: %04X ",i, temp);
            }
        }
        diag_printf("\n");

}

void dump_phy()
{
        int offset, i, value;

        /* SPEC defined Register 0~15
         * Global Register 16~31 for each page
         * Local Register 16~31 for each page
         */
        diag_printf("SPEC defined Register");
        for(i=0; i<5; i++){
                diag_printf("\n[Port %d]===============",i);
                for(offset=0;offset<16;offset++) {
                    if(offset%8==0) {
                        diag_printf("\n");
                }
                        value = ra305x_mii_readreg(i,offset);
                        diag_printf("%02d: %04X ",offset, value);
                }
        }

        for(offset=0;offset<5;offset++) { //global register  page 0~4
                dump_phy_reg(1, 16, 31, 0, offset);
        }

#if !defined (CONFIG_RAETH_HAS_PORT4)
        for(offset=0;offset<5;offset++) { //local register port 0-port4
#else
        for(offset=0;offset<4;offset++) { //local register port 0-port3
#endif
                dump_phy_reg(offset, 16, 31, 1, 0); //dump local page 0
                dump_phy_reg(offset, 16, 31, 1, 1); //dump local page 1
                dump_phy_reg(offset, 16, 31, 1, 2); //dump local page 2
                dump_phy_reg(offset, 16, 31, 1, 3); //dump local page 3
        }

}
#else
void dump_phy_reg(int port_no, int from, int to, int is_local)
{
        unsigned int i, temp = 0;
        if(is_local==0) {
            diag_printf("Global Register\n");
            diag_printf("===============");
            ra305x_mii_writereg(0, 31, 0); //select global register
            for(i=from;i<=to;i++) {
                if(i%8==0) {
                    diag_printf("\n");
                }
                temp = ra305x_mii_readreg(port_no,i);
                diag_printf("%02d: %04X ",i, temp);
            }
        } else {
            ra305x_mii_writereg(0, 31, 0x8000); //select local register
                diag_printf("\n\nLocal Register Port %d\n",port_no);
                diag_printf("===============");
                for(i=from;i<=to;i++) {
                    if(i%8==0) {
                        diag_printf("\n");
                    }
                    temp = ra305x_mii_readreg(port_no,i);
                    diag_printf("%02d: %04X ",i, temp);
                }
        }
        diag_printf("\n");



}

void dump_phy()
{
        int offset;
        dump_phy_reg(0, 0, 31, 0); //dump global register
        for(offset=0;offset<5;offset++) {
                dump_phy_reg(offset, 0, 31, 1); //dump local register
        }
}
#endif


// EOF if_ra305x.c
	
