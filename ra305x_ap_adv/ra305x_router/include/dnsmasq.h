
#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>

#include <sys/types.h>
#include "eventlog.h"
#include <arpa/nameser.h>
#include <time.h>

#if defined(INET_ADDRSTRLEN)
#  define ADDRSTRLEN INET_ADDRSTRLEN
#else
#  define ADDRSTRLEN 16 /* 4*3 + 3 dots + NULL */
#endif

#define FTABSIZ 150 /* max number of outstanding requests */
#define TIMEOUT 40 /* drop queries after TIMEOUT seconds */
#define LOGRATE 120 /* log table overflows every LOGRATE seconds */

#define SNAMELEN	40

#define PACKETSZ	512		/* maximum packet size */
#define RRFIXEDSZ	10		/* #/bytes of fixed data in r record */

#define MAXRECVINFACE	6
#define MAXDNSSERVER	6

#define MAXHOSTSENTRY	16

#define MAX_DNS_ARGS	10

#ifndef INADDRSZ
#define INADDRSZ 4
#endif

#ifndef T_SRV
#define T_SRV		33		/* Server selection */
#endif

#define FLAG_NOERR				0x0
#define FLAG_NEG				0x1
#define FLAG_IPV4				0x2
#define FLAG_QUERY				0x4

#define SERV_NO_ADDR			0x1  /* no server, this domain is local only */
#define SERV_LITERAL_ADDRESS	0x2  /* addr is the answer, not the server */ 

typedef HEADER DNSHEADER;

typedef struct myAddr {
	union
	{
		struct in_addr	addr4;
	} addr;
}MYADDR;

typedef union mySockaddr {
	struct sockaddr		sa;
	struct sockaddr_in	in;
}MYSOCKADDR;

typedef struct hostsEntry {
  MYADDR	addr;
  char		name[SNAMELEN];
}HOSTSENTRY;

typedef struct dnsServer {
  MYSOCKADDR	addr, source_addr;
  int			fd;
  int			flags;
  char			*domain; /* set if this server only handles a domain. */ 
}DNSSERVER;

/* linked list of all the interfaces in the system and 
   the sockets we have bound to each one. */
typedef struct ifEntry {
  MYSOCKADDR addr;
  int fd;
  int valid;
}IFENTRY;

typedef struct forwardEntry {
	DNSSERVER			*forwardto;
	MYSOCKADDR			queryaddr;
	time_t				time;
	int					fd;
	char				dnsname[256];
	unsigned short		oid, nid;
	struct forwardEntry	*next;
} FORWARDENTRY;

extern int DNSDebugLevel;
#define DNS_DBGPRINT(Level, fmt, args...)   \
{                                           \
    if (Level <= DNSDebugLevel)                      \
    {                                       \
		diag_printf( fmt, ## args);         \
    }                                       \
}

//export functions
int dnsMasqstart(int port);
int dnsMasqstop(void);
void dnsMasqforwarddump(void);
DNSSERVER *dnsMasqprocessreply(int fd, char *packet, int plen, time_t now);
DNSSERVER *dnsMasqprocessquery(int queryfd, MYSOCKADDR *queryaddr, DNSHEADER *dnsheader, 
							int plen, DNSSERVER *activeserver, time_t now);
