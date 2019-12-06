#ifndef __PPP_SECRET__H
#define __PPP_SECRET__H

#ifdef __cplusplus
extern "C" {
#endif

/* includes */
#include <sys_inc.h>
#include <pppd.h>

typedef int             STATUS;
/* typedefs */
typedef struct ppp_secret		/* PPP_SECRET */
{
    struct ppp_secret *	secretNext;
    char		client [MAXNAMELEN+33];
    char		server [MAXNAMELEN+33];
    char		secret [MAXSECRETLEN+33];
    char		addrs [MAXNAMELEN+33];
} PPP_SECRET;

/* globals */

extern PPP_SECRET *	pppSecretHead;
/* function declarations */
extern STATUS	AUTHInitSecurityInfo (void);
extern STATUS	AUTHInsertSecret (char * client, char * server, char * secret,
		    char * addrs);
extern STATUS	AUTHDeleteSecretEntry (char * client, char * server, char * secret);
extern int	AUTHSearchSecret (char * client, char * server, char * secret,
		    struct wordlist ** ppAddrs);


#ifdef __cplusplus
}
#endif

#endif /* __PPP_SECRET__H */


