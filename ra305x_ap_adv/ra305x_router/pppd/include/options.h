/* options.h - PPP option processing header */
#ifndef __INCoptionsh
#define __INCoptionsh

#ifdef  __cplusplus
extern "C" {
#endif

/* options flags */

#define PPP_S_NO_ALL                0   /* Don't allow any options */
#define PPP_S_PASSIVE_MODE          1     /* Set passive mode */
#define PPP_S_SILENT_MODE           2     /* Set silent mode */
#define PPP_S_DEFAULTROUTE          3     /* Add default route */
#define PPP_S_PROXYARP              4     /* Add proxy ARP entry */
#define PPP_S_IPCP_ACCEPT_LOCAL     5     /* Acpt peer's IP addr for us */
#define PPP_S_IPCP_ACCEPT_REMOTE    6     /* Acpt peer's IP addr for it */
#define PPP_S_NO_IP                 7     /* Disable IP addr negot. */
#define PPP_S_NO_ACC                8     /* Disable addr/control compr */
#define PPP_S_NO_PC                 9     /* Disable proto field compr */
#define PPP_S_NO_VJ                 10    /* Disable VJ compression */
#define PPP_S_NO_VJCCOMP            11    /* Disable VJ conct-ID compr */
#define PPP_S_NO_ASYNCMAP           12    /* Disable async map negot. */
#define PPP_S_NO_MN                 13    /* Disable magic num negot. */
#define PPP_S_NO_MRU                14    /* Disable MRU negotiation */
#define PPP_S_NO_PAP                15    /* Don't allow PAP auth */
#define PPP_S_NO_CHAP               16    /* Don't allow CHAP auth */
#define PPP_S_REQUIRE_PAP           17    /* Require PAP auth */
#define PPP_S_REQUIRE_CHAP          18    /* Require CHAP auth */
#define PPP_S_LOGIN                 19    /* Use login dbase for PAP */
#define PPP_S_DEBUG                 20    /* Enable daemon debug mode */
#define PPP_S_DRIVER_DEBUG          21    /* Enable driver debug mode */

/* PPP options flag bitfield values */

#define OPT_NO_ALL                  (1 << PPP_S_NO_ALL)
#define OPT_PASSIVE_MODE            (1 << PPP_S_PASSIVE_MODE)
#define OPT_SILENT_MODE             (1 << PPP_S_SILENT_MODE)
#define OPT_DEFAULTROUTE            (1 << PPP_S_DEFAULTROUTE)
#define OPT_PROXYARP                (1 << PPP_S_PROXYARP)
#define OPT_IPCP_ACCEPT_LOCAL       (1 << PPP_S_IPCP_ACCEPT_LOCAL)
#define OPT_IPCP_ACCEPT_REMOTE      (1 << PPP_S_IPCP_ACCEPT_REMOTE)
#define OPT_NO_IP                   (1 << PPP_S_NO_IP)
#define OPT_NO_ACC                  (1 << PPP_S_NO_ACC)
#define OPT_NO_PC                   (1 << PPP_S_NO_PC)
#define OPT_NO_VJ                   (1 << PPP_S_NO_VJ)
#define OPT_NO_VJCCOMP              (1 << PPP_S_NO_VJCCOMP)
#define OPT_NO_ASYNCMAP             (1 << PPP_S_NO_ASYNCMAP)
#define OPT_NO_MN                   (1 << PPP_S_NO_MN)
#define OPT_NO_MRU                  (1 << PPP_S_NO_MRU)
#define OPT_NO_PAP                  (1 << PPP_S_NO_PAP)
#define OPT_NO_CHAP                 (1 << PPP_S_NO_CHAP)
#define OPT_REQUIRE_PAP             (1 << PPP_S_REQUIRE_PAP)
#define OPT_REQUIRE_CHAP            (1 << PPP_S_REQUIRE_CHAP)
#define OPT_LOGIN                   (1 << PPP_S_LOGIN)
#define OPT_DEBUG                   (1 << PPP_S_DEBUG)
#define OPT_DRIVER_DEBUG            (1 << PPP_S_DRIVER_DEBUG)

/* PPP configuration options */

typedef struct ppp_options
{
    int flags;                      /* Flag options */
    char *asyncmap;                 /* Set the desired async map */
    char *escape_chars;             /* Set chars to escape on transmission */
    char *vj_max_slots;             /* Set maximum VJ compression header slots */
    char *netmask;                  /* Set netmask value for negotiation */
    char *mru;                      /* Set MRU value for negotiation */
    char *mtu;                      /* Set MTU value for negotiation */
    char *lcp_echo_failure;         /* Set max # consecutive LCP echo failures */
    char *lcp_echo_interval;        /* Set time for LCP echo requests */
    char *lcp_restart;              /* Set timeout for LCP */
    char *lcp_max_terminate;        /* Set max # xmits for LCP term-reqs */
    char *lcp_max_configure;        /* Set max # xmits for LCP conf-reqs */
    char *lcp_max_failure;          /* Set max # conf-naks for LCP */
    char *ipcp_restart;             /* Set timeout for IPCP */
    char *ipcp_max_terminate;       /* Set max # xmits for IPCP term-reqs */
    char *ipcp_max_configure;       /* Set max # xmits for IPCP conf-reqs */
    char *ipcp_max_failure;         /* Set max # conf-naks for IPCP */
    char *local_auth_name;          /* Set local name for authentication */
    char *remote_auth_name;         /* Set remote name for authentication */
    char *pap_file;                 /* Set the PAP secrets file */
    char *pap_user_name;            /* Set username for PAP auth with peer */
    char *pap_passwd;               /* Set password for PAP auth with peer */
    char *pap_restart;              /* Set timeout for PAP */
    char *pap_max_authreq;          /* Set max # xmits for PAP auth-reqs */
    char *chap_file;                /* Set the CHAP secrets file */
    char *chap_restart;             /* Set timeout for CHAP */
    char *chap_interval;            /* Set interval for CHAP rechallenge */
    char *chap_max_challenge;       /* Set max # xmits for CHAP challenge */
}
 PPP_OPTIONS;

/* function declarations */

extern int      parse_args (int unit, char *devname, char *local_addr,
                            char *remote_addr, int baud, PPP_OPTIONS *options,
                            char *fileName);

extern int      setdevname (PPP_IF_VAR_T *pPppIf, char *cp);
extern int      setipaddr(PPP_IF_VAR_T *pPppIf, u_long local_addr, u_long remote_addr);
extern int      setnetmask (PPP_IF_VAR_T *, u_long netmask);

extern u_long   GetMask (PPP_IF_VAR_T *pPppIf, u_long addr);

// Simon 2005/12/19
extern int parse_options(PPP_IF_VAR_T *pPppIf);


#ifdef  __cplusplus
}
#endif

#endif  /* __INCoptionsh */



