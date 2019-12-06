#ifndef _PKTFTR_H_
#define _PKTFTR_H_

typedef struct mbuf * (*pktftr_infun_t)(struct ifnet *ifp, struct ether_header *eh, struct mbuf *m);
typedef struct mbuf * (*pktftr_outfun_t)(struct ifnet *ifp, struct mbuf *m);
typedef struct pktftr_s {
	int hdl;
	struct pktftr_s *next;
	pktftr_infun_t in_filter;
	pktftr_outfun_t out_filter;
} pktftr_t;

#endif

