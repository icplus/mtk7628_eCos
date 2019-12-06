/* ppp_auth.h - PPP Authentication header */


#ifndef	__INCppp_authh
#define	__INCppp_authh

#ifdef	__cplusplus
extern "C" {
#endif

/* includes */
/* defines */

/* Bits in scan_authfile return value */

#define NONWILD_SERVER  1
#define NONWILD_CLIENT  2
#define NONWILD		(NONWILD_SERVER | NONWILD_CLIENT)
 
#define ISWILD(word)    (word[0] == '*' && word[1] == 0)

//extern FUNCPTR	pppCryptRtn;

/* function declarations */
 
void	link_required (fsm *f);
void	link_terminated (fsm *f);
void	link_down (fsm *f);
void	link_established (fsm *f);
void	auth_peer_fail (struct ppp_if_var *pPppIf, int protocol);
void	auth_peer_success (struct ppp_if_var *pPppIf, int protocol);
void	auth_withpeer_fail (struct ppp_if_var *pPppIf, int protocol);
void	auth_withpeer_success (struct ppp_if_var *pPppIf, int protocol);
void	check_auth_options (struct ppp_if_var *pPppIf);
int	    check_passwd (struct ppp_if_var *pPppIf, char *auser, int userlen, char *apasswd,
                    int passwdlen, char **msg, int *msglen);
int	    get_secret (struct ppp_if_var *pPppIf, char *client, char *server, char *secret,
                        int *secret_len, int save_addrs);
int	    auth_ip_addr (struct ppp_if_var *pPppIf, u_long addr);
int	    bad_ip_adrs (u_long addr);


#ifdef	__cplusplus
}
#endif

#endif	/* __INCppp_authh */


