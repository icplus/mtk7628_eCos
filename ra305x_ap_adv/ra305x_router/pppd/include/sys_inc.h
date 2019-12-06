#ifdef __cplusplus
extern "C" {
#endif

#include <sys/param.h>
//#include <sys/proc.h>
//#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/if_ether.h>

#ifdef __cplusplus
}
#endif
#define NET_IFNAME_LEN  16
#define OK		0
#define ERROR		(-1)
#define TRUE  1
#define FALSE 0

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif


