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
 ***************************************************************************/

//==============================================================================
//                                INCLUDE FILES
//==============================================================================

#include <network.h>
#include <sys/mbuf.h>
#include <netinet/if_ether.h>

//==============================================================================
//                                    MACROS
//==============================================================================

//==============================================================================
//                               TYPE DEFINITIONS
//==============================================================================

static char my_clone_mac_addr[6];
char * net_clone_mac_addr=&my_clone_mac_addr[0];
//char net_clone_mac_addr[6];
struct ifnet * net_clone_if;
int net_clone_stat;
extern int (*clnmac_detect)(struct mbuf *);
static char oldmac[6];
//==============================================================================
//                               LOCAL VARIABLES
//==============================================================================

//==============================================================================
//                          LOCAL FUNCTION PROTOTYPES
//==============================================================================

static int net_clone_mac_detect(struct mbuf * m, struct ether_header *eh);
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
int net_clone_mac_init(char *inf)
{
	struct ifnet *ifp;
	
	ifp = ifunit(inf);

	diag_printf("CLN MAC init,if %s=%x\n", inf, ifp);

    net_clone_stat=1;
    net_clone_if=ifp;
    net_clone_mac_addr[5]=0xaa;
	clnmac_detect=&net_clone_mac_detect;
	
	return 0;
}

int net_clone_mac_del(char *inf)
{
	//struct ifnet *ifp;
	//ifp = ifunit(inf);

	net_clone_stat=0;
	clnmac_detect=0;
	diag_printf("CLN MAC DEL\n");
	
	return 0;
}
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
int net_clone_mac_get(char *mac)
{
    int rc;

	
    if (net_clone_stat==2)
    {

        if (!memcmp(oldmac, net_clone_mac_addr, 6 ))
            rc=0;   // same as old
        else
        {
            rc=1;
            diag_printf("CLN MAC NEW!\n");
        }
        memcpy(oldmac, net_clone_mac_addr, 6 );
        memcpy(mac, net_clone_mac_addr, 6 );

    }
    else
    {
        rc=-1;
    }
    return rc;
}

extern struct ifnet * mat_lfp, *mat_efp;

int net_clone_bridge_init(char *l_inf, char *e_inf)
{
#ifdef  MAT 
   mat_lfp = ifunit(l_inf);
	mat_efp = ifunit(e_inf);

    ip2mac_init();
//diag_printf("net_clone_bridge_init: net_clone_mac_addr=%s\n", ether_ntoa(net_clone_mac_addr) );

	diag_printf("cloneing bridge init, local if=%x, ext if=%x\n", mat_lfp, mat_efp );
#endif
	return 0;
}

#if 0
static int net_clone_mac_detect(struct mbuf * m)
{
	struct ether_arp *ea;
	ea = mtod(m, struct ether_arp *);
	char *p=(char *)&ea->arp_sha[0];

    if (net_clone_stat==1)
    {
        if (net_clone_if==m->m_pkthdr.rcvif)
        {
            memcpy(net_clone_mac_addr,p, 6);
            diag_printf("target if=%x EA: %s\n" , m->m_pkthdr.rcvif, ether_ntoa(p));
            net_clone_stat++;
            mon_snd_cmd(13);
        }
    }
}
#else

static int net_clone_mac_detect(struct mbuf * m, struct ether_header *eh)
{
	char *p;
    if (net_clone_stat==1)
    {
        if (net_clone_if!=m->m_pkthdr.rcvif ) return 0;
		//if (eh->ether_type != htons(ETHERTYPE_ARP)) return 0;
		p=(char *)&eh->ether_shost[0];
        memcpy(net_clone_mac_addr, p, 6);
        diag_printf("CLN MAC: %s\n" , ether_ntoa(p));
        net_clone_stat++;
        mon_snd_cmd(13);
    }

	return 0;
}
#endif

