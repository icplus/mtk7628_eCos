/*
 * auth.c - PPP authentication and phase control.
 *
 * Copyright (c) 1993 The Australian National University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Australian National University.  The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys_inc.h>
#include <pppd.h>
#include <pppSecret.h>
#include <cfg_net.h>
#include <cfg_def.h>

#undef malloc
#undef free

/* Bits in auth_pending[] */
#define UPAP_WITHPEER   1
#define UPAP_PEER       2
#define CHAP_WITHPEER   4
#define CHAP_PEER       8

/* Prototypes */

static int  null_login          (PPP_IF_VAR_T *pPppIf);
static int  get_upap_passwd     (PPP_IF_VAR_T *pPppIf);
static int  have_upap_secret    (PPP_IF_VAR_T *pPppIf);
static int  have_chap_secret    (char *, char *);
static void free_wordlist       (struct wordlist *);

FUNCPTR pppCryptRtn = NULL;     /* PPP cryptographic authentication routine */

/*
 * An Open on LCP has requested a change from Dead to Establish phase.
 * Do what's necessary to bring the physical layer up.
 */
void
link_required(fsm *f)
{
}

/*
 * LCP has terminated the link; go to the Dead phase and take the
 * physical layer down.
 */
void
link_terminated(fsm *f)
{
    f->pPppIf->phase = PHASE_DEAD;
    AUTHDEBUG("Connection terminated.");
}

/*
 * LCP has gone down; it will either die or try to re-establish.
 */
void
link_down(fsm *f)
{
    f->pPppIf->phase = PHASE_TERMINATE;
}

/*
 * The link is established.
 * Proceed to the Dead, Authenticate or Network phase as appropriate.
 */
void
link_established(fsm *f)
{
    int auth;
    PPP_IF_VAR_T *pPppIf = f->pPppIf;
    lcp_options *wo = &pPppIf->lcp_wantoptions;
    lcp_options *go = &pPppIf->lcp_gotoptions;
    lcp_options *ho = &pPppIf->lcp_hisoptions;

    if (f->pPppIf->auth_required && !(go->neg_chap || go->neg_upap))
    {
        /*
         * We wanted the peer to authenticate itself, and it refused:
         * treat it as though it authenticated with PAP using a username
         * of "" and a password of "".  If that's not OK, boot it out.
         *
         * NB:
         * Changed this "if" statement to look for requiring CHAP and
         * not PAP (covered by !wo->neg_upap), so that peer cannot simply
         * refuse CHAP and get away with it...
         */
        if (!wo->neg_upap || !null_login(pPppIf))
        {
            AUTHDEBUG("peer refused to authenticate");
            die(f->pPppIf, 1);
        }
    }

    pPppIf->phase = PHASE_AUTHENTICATE;
    auth = 0;
    if (go->neg_chap)
    {
        ChapAuthPeer(pPppIf, pPppIf->our_name, go->chap_mdtype);
        auth |= CHAP_PEER;
    }
    else if (go->neg_upap)
    {
        upap_authpeer(pPppIf);
        auth |= UPAP_PEER;
    }

    if (ho->neg_chap)
    {
        ChapAuthWithPeer(pPppIf, pPppIf->our_name, ho->chap_mdtype);
        auth |= CHAP_WITHPEER;
    }
    else if (ho->neg_upap)
    {
        upap_authwithpeer(pPppIf, pPppIf->user, pPppIf->passwd);
        auth |= UPAP_WITHPEER;
    }
	
    pPppIf->auth_pending = auth;
	
	if (!auth)
	{
		//int val,ipmode;
		pPppIf->phase = PHASE_NETWORK;
	
#ifdef MPPE        
		if (pPppIf->mppe == 1)
			ccp_open(pPppIf);
#endif
		ipcp_open(pPppIf);
	}
}

/*
 * The peer has failed to authenticate himself using `protocol'.
 */
void
auth_peer_fail(PPP_IF_VAR_T *pPppIf, int protocol)
{
    /*
     * Authentication failure: take the link down
     */
    die(pPppIf, 1);
}

/*
 * The peer has been successfully authenticated using `protocol'.
 */
void
auth_peer_success(PPP_IF_VAR_T *pPppIf, int protocol)
{
    int bit;

    switch (protocol)
    {
    case CHAP:
        bit = CHAP_PEER;
        break;
    case UPAP:
        bit = UPAP_PEER;
        break;
    default:
        AUTHDEBUG("auth_peer_success: unknown protocol %x", protocol);
        return;
    }

    /*
     * If there is no more authentication still to be done,
     * proceed to the network phase.
     */
    if ((pPppIf->auth_pending &= ~bit) == 0)
    {
		//int val,ipmode;
		pPppIf->phase = PHASE_NETWORK;
#ifdef MPPE         
		if (pPppIf->mppe == 1)
			ccp_open(pPppIf);
#endif            
        ipcp_open(pPppIf);
    }
}

/*
 * We have failed to authenticate ourselves to the peer using `protocol'.
 */
void
auth_withpeer_fail(PPP_IF_VAR_T *pPppIf, int protocol)
{
    /*
     * We've failed to authenticate ourselves to our peer.
     * He'll probably take the link down, and there's not much
     * we can do except wait for that.
     */
#ifdef CONFIG_POE_STOP_DIAL_IF_AUTH_FAIL     
    pPppIf->hungup = 1;
#endif
    pPppIf->authfail = 1;
    lcp_close(pPppIf); 
}

/*
 * We have successfully authenticated ourselves with the peer using `protocol'.
 */
void
auth_withpeer_success(PPP_IF_VAR_T *pPppIf, int protocol)
{
    int bit;

    switch (protocol)
    {
    case CHAP:
        bit = CHAP_WITHPEER;
        break;
    case UPAP:
        bit = UPAP_WITHPEER;
        break;
    default:
        AUTHDEBUG("auth_peer_success: unknown protocol %x", protocol);
        bit = 0;
    }
    
    /*
    * If there is no more authentication still being done,
    * proceed to the network phase.
    */
    if ((pPppIf->auth_pending &= ~bit) == 0)
    {
		//int val,ipmode;
		pPppIf->phase = PHASE_NETWORK;
#ifdef MPPE        
		if (pPppIf->mppe == 1)
			ccp_open(pPppIf);
#endif            
        ipcp_open(pPppIf);
    }
}

/*
 * check_auth_options - called to check authentication options.
 */
void
check_auth_options(PPP_IF_VAR_T *pPppIf)
{
    lcp_options *wo = &pPppIf->lcp_wantoptions;
    lcp_options *ao = &pPppIf->lcp_allowoptions;

    /* Default our_name to hostname, and user to our_name */
    if (pPppIf->our_name[0] == 0)
        strcpy(pPppIf->our_name, pPppIf->_hostname);

    if (pPppIf->user[0] == 0)
        strcpy(pPppIf->user, pPppIf->our_name);
    
    /* If authentication is required, ask peer for CHAP or PAP. */
    if (pPppIf->auth_required && !wo->neg_chap && !wo->neg_upap)
    {
        wo->neg_chap = 1;
        wo->neg_upap = 1;
    }
    
    /*
    * Check whether we have appropriate secrets to use
    * to authenticate ourselves and/or the peer.
    */
    if (ao->neg_upap && pPppIf->passwd[0] == 0 && !get_upap_passwd(pPppIf))
        ao->neg_upap = 0;
    
    if (wo->neg_upap && !pPppIf->uselogin && !have_upap_secret(pPppIf))
        wo->neg_upap = 0;
    
    if (ao->neg_chap && !have_chap_secret(pPppIf->our_name, pPppIf->remote_name))
        ao->neg_chap = 0;
    
    if (wo->neg_chap && !have_chap_secret(pPppIf->remote_name, pPppIf->our_name))
        wo->neg_chap = 0;
    
    if (pPppIf->auth_required && !wo->neg_chap && !wo->neg_upap)
    {
        AUTHDEBUG("peer authentication required but no authentication files accessible");
        die(pPppIf, 1);
    }
}


/*
 * check_passwd - Check the user name and passwd against the PAP secrets
 * file.  If requested, also check against the system password database,
 * and login the user if OK.
 *
 * returns:
 *	UPAP_AUTHNAK: Authentication failed.
 *	UPAP_AUTHACK: Authentication succeeded.
 * In either case, msg points to an appropriate message.
 */
int
check_passwd(pPppIf, auser, userlen, apasswd, passwdlen, msg, msglen)
    PPP_IF_VAR_T *pPppIf;
    char *auser;
    int userlen;
    char *apasswd;
    int passwdlen;
    char **msg;
    int *msglen;
{
    int ret = UPAP_AUTHACK;
    struct wordlist *addrs = NULL;
    char passwd[256], user[256];
    char secret[MAXWORDLEN];
    
    /*
    * Make copies of apasswd and auser, then null-terminate them.
    */
    BCOPY(apasswd, passwd, passwdlen);
    passwd[passwdlen] = '\0';
    BCOPY(auser, user, userlen);
    user[userlen] = '\0';
   if (AUTHSearchSecret (user, pPppIf->our_name, secret, &addrs) < 0 ||
        (secret[0] != 0 && strcmp(passwd, secret) != 0 &&
        ((pppCryptRtn == NULL) ||
           strcmp((char *)(*pppCryptRtn)(secret, passwd), passwd) != 0)))
    {
        ret = UPAP_AUTHNAK;
    }
    
    if (ret == UPAP_AUTHNAK)
    {
        *msg = "Login incorrect";
        *msglen = strlen(*msg);
        if (addrs != NULL)
            free_wordlist(addrs);
        
    }
    else
    {
        *msg = "Login ok";
        *msglen = strlen(*msg);
        if (pPppIf->addresses != NULL)
            free_wordlist(pPppIf->addresses);
        pPppIf->addresses = addrs;
    }
    
    return ret;
}


/*
 * upap_login - Check the user name and password against the system
 * password database, and login the user if OK.
 *
 * returns:
 *	UPAP_AUTHNAK: Login failed.
 *	UPAP_AUTHACK: Login succeeded.
 * In either case, msg points to an appropriate message.
 */


/*
 * null_login - Check if a username of "" and a password of "" are
 * acceptable, and iff so, set the list of acceptable IP addresses
 * and return 1.
 */
static int
null_login(PPP_IF_VAR_T *pPppIf)
{
    return 0;
}


/*
 * get_upap_passwd - get a password for authenticating ourselves with
 * our peer using PAP.  Returns 1 on success, 0 if no suitable password
 * could be found.
 */
static int
get_upap_passwd(PPP_IF_VAR_T *pPppIf)
{
    char secret[MAXWORDLEN];

    if (AUTHSearchSecret (pPppIf->user, pPppIf->remote_name,
        secret, NULL) < 0)
    {
        return 0;
    }

    strncpy(pPppIf->passwd, secret, MAXSECRETLEN);
    pPppIf->passwd[MAXSECRETLEN-1] = 0;
 
    return 1;
}


/*
 * have_upap_secret - check whether we have a PAP file with any
 * secrets that we could possibly use for authenticating the peer.
 */
static int
have_upap_secret(PPP_IF_VAR_T *pPppIf)
{
    if (pPppIf->uselogin)
        return 1;
    
    if (AUTHSearchSecret (NULL, pPppIf->our_name, NULL, NULL) < 0)
    {
        return 0;
    }
    
    return 1;
}


/*
 * have_chap_secret - check whether we have a CHAP file with a
 * secret that we could possibly use for authenticating `client'
 * on `server'.  Either can be the null string, meaning we don't
 * know the identity yet.
 */
static int
have_chap_secret(client, server)
    char *client;
    char *server;
{

    if (client[0] == 0)
        client = NULL;
    else if (server[0] == 0)
        server = NULL;

    if (AUTHSearchSecret (client, server, NULL, NULL) < 0)
    {
        return 0;
    }

    return 1;
}


/*
 * get_secret - open the CHAP secret file and return the secret
 * for authenticating the given client on the given server.
 * (We could be either client or server).
 */
int
get_secret(pPppIf, client, server, secret, secret_len, save_addrs)
    PPP_IF_VAR_T *pPppIf;
    char *client;
    char *server;
    char *secret;
    int *secret_len;
    int save_addrs;
{
    int len;
    struct wordlist *addrs = NULL;
    char secbuf[MAXWORDLEN];

    secbuf[0] = 0;
    if (AUTHSearchSecret (client, server, secbuf, &addrs) < 0)
    {
    	if (addrs )
    		{
			 free_wordlist(addrs);
			 addrs =NULL;
    		}
        return 0;
    }
    
    if (save_addrs)
    {
        if (pPppIf->addresses != NULL)
            free_wordlist(pPppIf->addresses);
        pPppIf->addresses = addrs;
    } else
	{
    	if (addrs )
    		{
			 free_wordlist(addrs);
			 addrs=NULL;
    		}
		
	}
    
    len = strlen(secbuf);
    if (len > MAXSECRETLEN)
    {
        AUTHDEBUG("Secret for %s on %s is too long", client, server);
        len = MAXSECRETLEN;
    }
    BCOPY(secbuf, secret, len);
    *secret_len = len;

    return 1;
}

#if 0
/*
 * auth_ip_addr - check whether the peer is authorized to use
 * a given IP address.  Returns 1 if authorized, 0 otherwise.
 */
int
auth_ip_addr(PPP_IF_VAR_T *pPppIf, u_long addr)
{
    u_long a;
    struct wordlist *addrs;

    /* don't allow loopback or multicast address */
    if (bad_ip_adrs(addr))
        return 0;

    if ((addrs = pPppIf->addresses) == NULL)
        return 1;       /* no restriction */
    
    for (; addrs != NULL; addrs = addrs->next)
    {
        /* "-" means no addresses authorized */
        if (strcmp(addrs->word, "-") == 0)
            break;

        if ((a = inet_addr(addrs->word)) == -1)
        {
            if ((a = HostTbl_GetByName (addrs->word)) == 0)
            {
                AUTHDEBUG("unknown host %s in auth. address list", addrs->word);
                continue;
            }
        }

        if (addr == a)
            return 1;
    }

    return 0;           /* not in list => can't have it */
}
#endif

/*
 * bad_ip_adrs - return 1 if the IP address is one we don't want
 * to use, such as an address in the loopback net or a multicast address.
 * addr is in network byte order.
 */
int
bad_ip_adrs(addr)
    u_long addr;
{
    //addr = ntohl(addr);  //yfchou find bug!!
//#if CPU==SIMSPARCSOLARIS
//    return IN_MULTICAST(addr) || IN_BADCLASS(addr);
//#else
    return (addr >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET
            || IN_MULTICAST(addr) || IN_BADCLASS(addr);
//#endif
}


/*
 * free_wordlist - release memory allocated for a wordlist.
 */
static void
free_wordlist(struct wordlist *wp)
{
    struct wordlist *next;

    while (wp != NULL)
    {
        next = wp->next;
        free(wp);
        wp = next;
    }
}
/*---------------------------------------------------------------------------
*      Secret table utilities
-----------------------------------------------------------------------------*/
/* globals */

PPP_SECRET *	pppSecretHead = NULL;		/* head of table linked list */
//static PPP_SECRET 	PppSecretArray[10];
//static unsigned     PppSecretIndex;

/* locals */
static int pppSemId = 0;
static cyg_mutex_t pppSem_obj;


/*--------------------------------------------------------------
* ROUTINE NAME - AUTHInitSecurityInfo
*---------------------------------------------------------------
* FUNCTION: This routine initialize the PPP authentication 
*           secrets table facility  .
*
* INPUT:    None 
* RETURN:  OK or ERROR
---------------------------------------------------------------*/
int AUTHInitSecurityInfo (void)
{
	//yfchou added 01/27/2004
	//PppSecretIndex = 0;
    if (pppSemId == 0)       /* already initialized? */
    {
        cyg_mutex_init(&pppSem_obj);
        pppSemId = 1;
    }

    return (OK);
}


/*--------------------------------------------------------------
* ROUTINE NAME - AUTHInsertSecret
*---------------------------------------------------------------
* FUNCTION: This routine adds a secret to the Point-to-Point 
*           Protocol (PPP) authentication secrets table.  This 
*           table may be used by the Password Authentication 
*           Protocol (PAP) and Challenge-Handshake Authentication 
*           Protocol (CHAP) user authentication protocols.
*
*           When a PPP link is established, a "server" may require 
*           a "client" to authenticate itself using a "secret".  
*           Clients and servers obtain authentication secrets 
*           by searching secrets files, or by searching the secrets 
*           table constructed by this routine.  Clients and servers
*           search the secrets table by matching client and server 
*           names with table entries, and retrieving the associated 
*           secret.
*
*           Client and server names in the table consisting of "*" 
*           are considered wildcards; they serve as matches for 
*           any client and/or server name if an exact match cannot 
*           be found.
*
*           If <secret> starts with "@", <secret> is assumed to be 
*           the name of a file, wherein the actual secret can be read.
*
*           If <addrs> is not NULL, it should contain a list of 
*           acceptable client IP addresses.   When a server is 
*           authenticating a client and the client's IP address 
*           is not contained in the list of acceptable addresses,
*           the link is terminated.  Any IP address will be 
*           considered acceptable if <addrs> is NULL.  If this 
*           parameter is "-", all IP addresses are disallowed.
*
* INPUT:    
*       client -- client being authenticated 
*       server -- server performing authentication
*       secret -- secret used for authentication
*       addrs  -- acceptable client IP addresses 
*
* RETURN:  OK, or ERROR if the secret cannot be added to the table.
---------------------------------------------------------------*/
STATUS AUTHInsertSecret(char *client, char *server, char *secret, char *addrs)
{
    PPP_SECRET *pSecret;        /* pointer to new secret */
    
    if (pppSemId == 0)
    {
        return (ERROR);
    }
#if 1    
    if ((pSecret = (PPP_SECRET *) calloc (1, sizeof (struct ppp_secret))) == NULL)
        return (ERROR);
#else
	//yfchou added 01/27/2004
	pSecret = &PppSecretArray[PppSecretIndex++];
#endif	
	memset(pSecret,0,sizeof(PPP_SECRET));

    /* copy secret information */
    if (client)
        strcpy (pSecret->client, client);
    if (server)
        strcpy (pSecret->server, server);		
    if (secret)
        strcpy (pSecret->secret, secret);
    if (addrs)
        strcpy (pSecret->addrs, addrs);

    /* hook into secret list */
    cyg_mutex_lock(&pppSem_obj);
    pSecret->secretNext = pppSecretHead;    /* put secret on front */
    pppSecretHead = pSecret;
    cyg_mutex_unlock(&pppSem_obj);
    
    return (OK);
}

/*--------------------------------------------------------------
* ROUTINE NAME - AUTHDeleteSecretEntry
*---------------------------------------------------------------
* FUNCTION: This routine deletes a secret from the Point-to-Point 
*           Protocol (PPP) authentication secrets table.  When 
*           searching for a secret to delete from the table, 
*           the wildcard substitution (using "*") is not performed for
*           client and/or server names.  The <client>, <server>, 
*           and <secret> strings must match the table entry exactly
*           in order to be deleted.
*
* INPUT:    
*       client -- client being authenticated 
*       server -- server performing authentication
*       secret -- secret used for authentication
* 
* RETURN:  OK, or ERROR if the table entry being deleted is not found.
---------------------------------------------------------------*/
int AUTHDeleteSecretEntry(char *client, char *server, char *secret)
{
    PPP_SECRET *pSecret;
    PPP_SECRET **ppPrev;                    /* list trailer */
	
    if (pppSemId == 0)
    {
        return (ERROR);
    }
    cyg_mutex_lock(&pppSem_obj);

    /* find secret */
    ppPrev = &pppSecretHead;
    for (pSecret = pppSecretHead; pSecret != NULL; pSecret =
        pSecret->secretNext)
    {
        if ((!strcmp (client, pSecret->client)) &&
            (!strcmp (server, pSecret->server)) &&
            (!strcmp (secret, pSecret->secret)))
            break;

        ppPrev = &pSecret->secretNext;  /* update list trailer */
    }
    
    if (pSecret != NULL)
        *ppPrev = pSecret->secretNext;

    /* unhook from secret list */
    cyg_mutex_unlock(&pppSem_obj);
    if (pSecret == NULL)            /* secret found ? */
    {
        return (ERROR);
    }
    //yfchou added 01/27/2004
#if 1
    free (pSecret);                 /* free secret */
#else
	PppSecretIndex--;
#endif    
    return (OK);
}

/*--------------------------------------------------------------
* ROUTINE NAME - AUTHSearchSecret
*---------------------------------------------------------------
* FUNCTION: This routine searches the PPP authentication secrets
*           table for a suitable secret to authenticate the 
*           given <client> and <server>.  The secret is returned 
*           in the <secret> parameter.  The list of authorized 
*           client IP addresses are returned in the <ppAddrs> 
*           parameter.  If the secret starts with "@", the 
*           secret is taken to be a filename, wherein which the 
*           actual secret is read.
*
* INPUT:     
*       client -- client being authenticated 
*       server -- server performing authentication
*       secret -- secret used for authentication
*
* RETURN:   flag determining strength of the secret, or ERROR if no
*           suitable secret could be found.
---------------------------------------------------------------*/
int AUTHSearchSecret(char *client, char *server, char *secret, 
                   struct wordlist **ppAddrs)
{
    PPP_SECRET *pSecret = NULL;
    PPP_SECRET *pEntry;
    //char atfile [MAXWORDLEN];
    char word [MAXWORDLEN];
    char *pWord;
    char *separators = {" \t"};
    char *pAddr;
    char *pLast = NULL;
    int  got_flag = 0;
    int  best_flag = -1;
    //int  xxx;
    struct wordlist *addr_list = NULL;
    struct wordlist *addr_last = NULL;
    struct wordlist *ap;
	
    if (pppSemId == 0)
    {
        return (ERROR);
    }
    cyg_mutex_lock(&pppSem_obj);

    /* find best secret entry */
    for (pEntry = pppSecretHead; pEntry != NULL; pEntry = pEntry->secretNext)
    {
        /* check for a match */

        if (((client != NULL) && client[0] && strcmp (client, pEntry->client)
            && !ISWILD (pEntry->client)) ||
            ((server != NULL) && server[0] && strcmp (server, pEntry->server)
            && !ISWILD (pEntry->server)))
            continue;
        
        if (!ISWILD (pEntry->client))
            got_flag = NONWILD_CLIENT;
        if (!ISWILD (pEntry->server))
            got_flag |= NONWILD_SERVER;
        
        if (got_flag > best_flag)
        {
            pSecret = pEntry;           /* update best entry */
            best_flag = got_flag;
        }
    }
    cyg_mutex_unlock(&pppSem_obj);
    
    if (pSecret == NULL)                /* secret found ? */
    {
        return (ERROR);
    }
    
    /* check for special syntax: @filename means read secret from file */
    if (secret != NULL)
    {
        strcpy (secret, pSecret->secret);   /* stuff secret for return */
    }

    /* read address authorization info and make a wordlist */
    if (ppAddrs != NULL)
    {
        *ppAddrs = NULL;                        /* tie off in case error */
        strcpy (word, pSecret->addrs);
        pWord = word;
        
        while ((pAddr = strtok_r (pWord, separators, &pLast)) != NULL)
        {
            pWord = NULL;
            
            if ((ap = (struct wordlist *) malloc (sizeof (struct wordlist)
                + strlen (pAddr))) == NULL)
            {
                diag_printf("authorized addresses");
                return ERROR;
            }
            ap->next = NULL;
            strcpy (ap->word, pAddr);       /* stuff word */
            
            if (addr_list == NULL)
                addr_list = ap;             /* first word */
            else
                addr_last->next = ap;       /* tie in subsequent words */

            addr_last = ap;                 /* bump current word pointer */
        }
        *ppAddrs = addr_list;               /* hook wordlist for return */
    }

    return (best_flag);
}

