/*
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
#include <errno.h>

#ifndef GIDSET_TYPE
#define GIDSET_TYPE     int
#endif

/*
 * Prototypes
 */

static int setdebug         (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setnodebug       (PPP_IF_VAR_T *);
#endif
static int setkdebug        (PPP_IF_VAR_T *);
static int setpassive       (PPP_IF_VAR_T *);
static int setsilent        (PPP_IF_VAR_T *);
static int noopt            (PPP_IF_VAR_T *);
static int setnovj          (PPP_IF_VAR_T *);
static int setnovjccomp     (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setvjslots       (PPP_IF_VAR_T *, char *);
#endif
static int nopap            (PPP_IF_VAR_T *);
static int nochap           (PPP_IF_VAR_T *);
static int reqpap           (PPP_IF_VAR_T *);
static int reqchap          (PPP_IF_VAR_T *);
// static int setchapfile      (PPP_IF_VAR_T *, char *);
// static int setupapfile      (PPP_IF_VAR_T *, char *);
// static int setspeed         (PPP_IF_VAR_T *, int, int);
static int noaccomp         (PPP_IF_VAR_T *);
static int noasyncmap       (PPP_IF_VAR_T *);
static int noipaddr         (PPP_IF_VAR_T *);
static int nomagicnumber    (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setasyncmap      (PPP_IF_VAR_T *, char *);
static int setescape        (PPP_IF_VAR_T *, char *);
static int setmru           (PPP_IF_VAR_T *, char *);
static int setmtu           (PPP_IF_VAR_T *, char *);
#endif
static int nomru            (PPP_IF_VAR_T *);
static int nopcomp          (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setname          (PPP_IF_VAR_T *, char *);
static int setuser          (PPP_IF_VAR_T *, char *);
static int setpasswd        (PPP_IF_VAR_T *, char *);
static int setremote        (PPP_IF_VAR_T *, char *);
#endif
static int setdefaultroute  (PPP_IF_VAR_T *);
static int setproxyarp      (PPP_IF_VAR_T *);
static int setdologin       (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setlcptimeout    (PPP_IF_VAR_T *, char *);
static int setlcpterm       (PPP_IF_VAR_T *, char *);
static int setlcpconf       (PPP_IF_VAR_T *, char *);
static int setlcpfails      (PPP_IF_VAR_T *, char *);
static int setipcptimeout   (PPP_IF_VAR_T *, char *);
static int setipcpterm      (PPP_IF_VAR_T *, char *);
static int setipcpconf      (PPP_IF_VAR_T *, char *);
static int setipcpfails     (PPP_IF_VAR_T *, char *);
static int setpaptimeout    (PPP_IF_VAR_T *, char *);
static int setpapreqs       (PPP_IF_VAR_T *, char *);
static int setchaptimeout   (PPP_IF_VAR_T *, char *);
static int setchapchal      (PPP_IF_VAR_T *, char *);
static int setchapintv      (PPP_IF_VAR_T *, char *);
#endif
static int setipcpaccl      (PPP_IF_VAR_T *);
static int setipcpaccr      (PPP_IF_VAR_T *);
#ifdef PPP_OPTION_CMD
static int setlcpechointv   (PPP_IF_VAR_T *, char *);
static int setlcpechofails  (PPP_IF_VAR_T *, char *);
#endif

static int number_option    (char *, long *, int);
// static int readable         (int fd);
#ifdef PPP_OPTION_CMD
/*
 * Valid arguments.
 */
static struct cmd {
    char *cmd_name;
    int num_args;
    int (*cmd_func)(PPP_IF_VAR_T *);
} cmds[] = {
    {"no_all", 0, noopt},                       /* Don't request/allow any options */
    {"no_acc", 0, noaccomp},                    /* Disable Address/Control compress */
    {"no_asyncmap", 0, noasyncmap},             /* Disable asyncmap negotiation */
    {"debug", 0, setdebug},                     /* Enable the daemon debug mode */
    {"nodebug", 0, setnodebug},                 /* Disable the daemon debug mode */
    {"driver_debug", 1, setkdebug},             /* Enable driver-level debugging */
    {"no_ip", 0, noipaddr},                     /* Disable IP address negotiation */
    {"no_mn", 0, nomagicnumber},                /* Disable magic number negotiation */
    {"no_mru", 0, nomru},                       /* Disable mru negotiation */
    {"passive_mode", 0, setpassive},            /* Set passive mode */
    {"no_pc", 0, nopcomp},                      /* Disable protocol field compress */
    {"no_pap", 0, nopap},                       /* Don't allow UPAP authentication with peer */
    {"no_chap", 0, nochap},                     /* Don't allow CHAP authentication with peer */
    {"require_pap", 0, reqpap},                 /* Require PAP auth from peer */
    {"require_chap", 0, reqchap},               /* Require CHAP auth from peer */
    {"no_vj", 0, setnovj},                      /* disable VJ compression */
    {"no_vjccomp", 0, setnovjccomp},            /* disable VJ connection-ID compression */
    {"silent_mode", 0, setsilent},              /* Set silent mode */
    {"defaultroute", 0, setdefaultroute},       /* Add default route */
    {"proxyarp", 0, setproxyarp},               /* Add proxy ARP entry */
    {"login", 0, setdologin},                   /* Use system password database for UPAP */
    {"ipcp_accept_local", 0, setipcpaccl},      /* Accept peer's address for us */
    {"ipcp_accept_remote", 0, setipcpaccr},     /* Accept peer's address for it */
    {"asyncmap", 1, setasyncmap},               /* set the desired async map */
    {"vj_max_slots", 1, setvjslots},            /* Set maximum VJ header slots */
    {"escape_chars", 1, setescape},             /* set chars to escape on transmission */
#if 0
    {"pap_file", 1, setupapfile},               /* Get PAP user and password from file */
    {"chap_file", 1, setchapfile},              /* Get CHAP info */
#endif
    {"mru", 1, setmru},                         /* Set MRU value for negotiation */
    {"mtu", 1, setmtu},                         /* Set our MTU */
    {"netmask", 1, setnetmask},                 /* Set netmask */
    {"local_auth_name", 1, setname},            /* Set local name for authentication */
    {"pap_user_name", 1, setuser},              /* Set username for PAP auth with peer */
    {"pap_passwd", 1, setpasswd},               /* Set password for PAP auth with peer */
    {"remote_auth_name", 1, setremote},         /* Set remote name for authentication */
    {"lcp_echo_failure", 1, setlcpechofails},   /* consecutive echo failures */
    {"lcp_echo_interval", 1, setlcpechointv},   /* time for lcp echo events */
    {"lcp_restart", 1, setlcptimeout},          /* Set timeout for LCP */
    {"lcp_max_terminate", 1, setlcpterm},       /* Set max #xmits for term-reqs */
    {"lcp_max_configure", 1, setlcpconf},       /* Set max #xmits for conf-reqs */
    {"lcp_max_failure", 1, setlcpfails},        /* Set max #conf-naks for LCP */
    {"ipcp_restart", 1, setipcptimeout},        /* Set timeout for IPCP */
    {"ipcp_max_terminate", 1, setipcpterm},     /* Set max #xmits for term-reqs */
    {"ipcp_max_configure", 1, setipcpconf},     /* Set max #xmits for conf-reqs */
    {"ipcp_max_failure", 1, setipcpfails},      /* Set max #conf-naks for IPCP */
    {"pap_restart", 1, setpaptimeout},          /* Set timeout for UPAP */
    {"pap_max_authreq", 1, setpapreqs},         /* Set max #xmits for auth-reqs */
    {"chap_restart", 1, setchaptimeout},        /* Set timeout for CHAP */
    {"chap_max_challenge", 1, setchapchal},     /* Set max #xmits for challenge */
    {"chap_interval", 1, setchapintv},          /* Set interval for rechallenge */
    {NULL, 0, NULL}
};
#endif

#ifdef  notyet

#ifndef IMPLEMENTATION
#define IMPLEMENTATION ""
#endif

static char *usage_string = "\
pppd version %s patch level %d%s\n\
Usage: %s [ arguments ], where arguments are:\n\
        <device>        Communicate over the named device\n\
        <speed>         Set the baud rate to <speed>\n\
        <loc>:<rem>     Set the local and/or remote interface IP\n\
                        addresses.  Either one may be omitted.\n\
        asyncmap <n>    Set the desired async map to hex <n>\n\
        auth            Require authentication from peer\n\
        connect <p>     Invoke shell command <p> to set up the serial line\n\
        defaultroute    Add default route through interface\n\
        file <f>        Take options from file <f>\n\
        modem           Use modem control lines\n\
        mru <n>         Set MRU value to <n> for negotiation\n\
        netmask <n>     Set interface netmask to <n>\n\
See pppd(8) for more options.\n\
";
#endif	/* notyet */


/*
 * parse_options - parse a flag of option
 */
int
parse_options(PPP_IF_VAR_T *pPppIf)
{
    if (pPppIf->flags & OPT_NO_ALL)
        noopt(pPppIf);
    
    if (pPppIf->flags & OPT_NO_ACC)
        noaccomp(pPppIf);
    
    if (pPppIf->flags & OPT_NO_ASYNCMAP)
        noasyncmap(pPppIf);
    
    if (pPppIf->flags & OPT_DEBUG)
        setdebug(pPppIf);
    
    if (pPppIf->flags & OPT_DRIVER_DEBUG)
        setkdebug(pPppIf);
    
    if (pPppIf->flags & OPT_NO_IP)
        noipaddr(pPppIf);
    
    if (pPppIf->flags & OPT_NO_MN)
        nomagicnumber(pPppIf);
    
    if (pPppIf->flags & OPT_NO_MRU)
        nomru(pPppIf);
    
    if (pPppIf->flags & OPT_PASSIVE_MODE)
        setpassive(pPppIf);
    
    if (pPppIf->flags & OPT_NO_PC)
        nopcomp(pPppIf);
    
    if (pPppIf->flags & OPT_NO_PAP)
        nopap(pPppIf);
    
    if (pPppIf->flags & OPT_NO_CHAP)
        nochap(pPppIf);
    
    if (pPppIf->flags & OPT_REQUIRE_PAP)
        reqpap(pPppIf);
    
    if (pPppIf->flags & OPT_REQUIRE_CHAP)
        reqchap(pPppIf);
    
    if (pPppIf->flags & OPT_NO_VJ)
        setnovj(pPppIf);
    
    if (pPppIf->flags & OPT_NO_VJCCOMP)
        setnovjccomp(pPppIf);
    
    if (pPppIf->flags & OPT_SILENT_MODE)
        setsilent(pPppIf);
    
    if (pPppIf->flags & OPT_DEFAULTROUTE)
        setdefaultroute(pPppIf);
    
    if (pPppIf->flags & OPT_PROXYARP)
        setproxyarp(pPppIf);
    
    if (pPppIf->flags & OPT_LOGIN)
        setdologin(pPppIf);
    
    if (pPppIf->flags & OPT_IPCP_ACCEPT_LOCAL)
        setipcpaccl(pPppIf);
    
    if (pPppIf->flags & OPT_IPCP_ACCEPT_REMOTE)
        setipcpaccr(pPppIf);

    return 1;
}



/*
 * usage - print out a message telling how to use the program.
 */

#ifdef	notyet
static void
usage()
{
    logMsg(usage_string, VERSION, PATCHLEVEL, IMPLEMENTATION,
           "pppInit");
}
#endif	/* notyet */


#if 0
/*
 * readable - check if a file is readable by the real user.
 */
static int
readable(fd)
    int fd;
{
#ifdef	notyet
    uid_t uid;
    int ngroups, i;
    struct stat sbuf;
    GIDSET_TYPE groups[NGROUPS_MAX];

    uid = getuid();
    if (uid == 0)
        return 1;
    if (fstat(fd, &sbuf) != 0)
        return 0;
    if (sbuf.st_uid == uid)
        return sbuf.st_mode & S_IRUSR;
    if (sbuf.st_gid == getgid())
        return sbuf.st_mode & S_IRGRP;
    ngroups = getgroups(NGROUPS_MAX, groups);
    for (i = 0; i < ngroups; ++i)
    {
        if (sbuf.st_gid == groups[i])
            return sbuf.st_mode & S_IRGRP;
    }

    return sbuf.st_mode & S_IROTH;
#else	/* notyet */
    return 1;
#endif	/* notyet */
}
#endif

#ifdef PPP_OPTION_CMD
/*
 * number_option - parse a numeric parameter for an option
 */
static int
number_option(str, valp, base)
    char *str;
    long *valp;
    int base;
{
    char *ptr;

    *valp = strtoul(str, &ptr, base);
    if (errno == ERANGE)
    {
        MAINDEBUG("invalid number: %s", str);
        return 0;
    }
    return 1;
}



/*
 * int_option - like number_option, but valp is int *,
 * the base is assumed to be 0, and *valp is not changed
 * if there is an error.
 */
static int
int_option(str, valp)
    char *str;
    int *valp;
{
    long v;

    if (!number_option(str, &v, 0))
        return 0;

    *valp = (int) v;
    return 1;
}
#endif

/*
 * The following procedures execute commands.
 */

/*
 * setdebug - Set debug (command line argument).
 */
static int
setdebug(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->debug = 1;
    return (1);
}

#ifdef PPP_OPTION_CMD
/*
 * setnodebug - Disable debug (command line argument).
 */
static int
setnodebug(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->debug = 0;
    return (1);
}
#endif

/*
 * setkdebug - Set kernel debugging level.
 */
static int
setkdebug(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->kdebugflag = 1;
    return (1);
}

/*
 * noopt - Disable all options.
 */
static int
noopt(PPP_IF_VAR_T *pPppIf)
{
    BZERO((char *) &pPppIf->lcp_wantoptions,
          sizeof (struct lcp_options));
    BZERO((char *) &pPppIf->lcp_allowoptions,
          sizeof (struct lcp_options));
    BZERO((char *) &pPppIf->ipcp_wantoptions,
          sizeof (struct ipcp_options));
    BZERO((char *) &pPppIf->ipcp_allowoptions,
          sizeof (struct ipcp_options));
    return (1);
}

/*
 * noaccomp - Disable Address/Control field compression negotiation.
 */
static int
noaccomp(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_accompression = 0;
    pPppIf->lcp_allowoptions.neg_accompression = 0;
    return (1);
}


/*
 * noasyncmap - Disable async map negotiation.
 */
static int
noasyncmap(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_asyncmap = 0;
    pPppIf->lcp_allowoptions.neg_asyncmap = 0;
    return (1);
}


/*
 * noipaddr - Disable IP address negotiation.
 */
static int
noipaddr(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.neg_addr = 0;
    pPppIf->ipcp_allowoptions.neg_addr = 0;
    return (1);
}


/*
 * nomagicnumber - Disable magic number negotiation.
 */
static int
nomagicnumber(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_magicnumber = 0;
    pPppIf->lcp_allowoptions.neg_magicnumber = 0;
    return (1);
}


/*
 * nomru - Disable mru negotiation.
 */
static int
nomru(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_mru = 0;
    pPppIf->lcp_allowoptions.neg_mru = 0;
    return (1);
}

#ifdef PPP_OPTION_CMD
/*
 * setmru - Set MRU for negotiation.
 */
static int
setmru(PPP_IF_VAR_T *pPppIf, char *argv)
{
    long mru;

    if (!number_option(argv, &mru, 0))
        return 0;
    pPppIf->lcp_wantoptions.mru = mru;
    pPppIf->lcp_wantoptions.neg_mru = 1;
    return (1);
}


/*
 * setmtu - Set the largest MTU we'll use.
 */
static int
setmtu(PPP_IF_VAR_T *pPppIf, char *argv)
{
    long mtu;

    if (!number_option(argv, &mtu, 0))
        return 0;
    if (mtu < MINMRU || mtu > MAXMRU)
    {
        MAINDEBUG("mtu option value of %d is too %s", mtu, (mtu < MINMRU? "small": "large"));
        return 0;
    }
    pPppIf->lcp_allowoptions.mru = mtu;
    return (1);
}
#endif

/*
 * nopcomp - Disable Protocol field compression negotiation.
 */
static int
nopcomp(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_pcompression = 0;
    pPppIf->lcp_allowoptions.neg_pcompression = 0;
    return (1);
}


/*
 * setpassive - Set passive mode (don't give up if we time out sending
 * LCP configure-requests).
 */
static int
setpassive(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.passive = 1;
    return (1);
}


/*
 * setsilent - Set silent mode (don't start sending LCP configure-requests
 * until we get one from the peer).
 */
static int
setsilent(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.silent = 1;
    return (1);
}


/*
 * nopap - Disable PAP authentication with peer.
 */
static int
nopap(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_allowoptions.neg_upap = 0;
    return (1);
}

/*
 * reqpap - Require PAP authentication from peer.
 */
static int
reqpap(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_upap = 1;
    pPppIf->auth_required = 1;
    return (1);
}



/*
 * nochap - Disable CHAP authentication with peer.
 */
static int
nochap(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_allowoptions.neg_chap = 0;
    return (1);
}


/*
 * reqchap - Require CHAP authentication from peer.
 */
static int
reqchap(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->lcp_wantoptions.neg_chap = 1;
    pPppIf->auth_required = 1;
    return (1);
}

/*
 * setnovj - disable vj compression
 */
static int
setnovj(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.neg_vj = 0;
    pPppIf->ipcp_allowoptions.neg_vj = 0;
    return (1);
}


/*
 * setnovjccomp - disable VJ connection-ID compression
 */
static int
setnovjccomp(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.cflag = 0;
    pPppIf->ipcp_allowoptions.cflag = 0;
    return (1);
}


#ifdef PPP_OPTION_CMD
/*
 * setvjslots - set maximum number of connection slots for VJ compression
 */
static int
setvjslots(PPP_IF_VAR_T *pPppIf, char *argv)
{
    int value;

    if (!int_option(argv, &value))
        return 0;
    if (value < 2 || value > 16)
    {
        MAINDEBUG("pppd: vj-max-slots value must be between 2 and 16");
        return 0;
    }

    pPppIf->ipcp_wantoptions.maxslotindex =
    pPppIf->ipcp_allowoptions.maxslotindex = value - 1;
    return 1;
}



/*
 * setasyncmap - add bits to asyncmap (what we request peer to escape).
 */
static int
setasyncmap(PPP_IF_VAR_T *pPppIf, char *argv)
{
    long asyncmap;

    if (!number_option(argv, &asyncmap, 16))
        return 0;

    pPppIf->lcp_wantoptions.asyncmap |= asyncmap;
    pPppIf->lcp_wantoptions.neg_asyncmap = 1;
    return (1);
}



/*
 * setescape - add chars to the set we escape on transmission.
 */
static int
setescape(PPP_IF_VAR_T *pPppIf, char *argv)
{
    int n, ret;
    char *p, *endp;

    p = argv;
    ret = 1;
    while (*p)
    {
        n = strtoul(p, &endp, 16);
        if (p == endp)
        {
            MAINDEBUG("invalid hex number: %s", p);
            return 0;
        }

        p = endp;
        if (n < 0 || (0x20 <= n && n <= 0x3F) || n == 0x5E || n > 0xFF)
        {
            MAINDEBUG("can't escape character 0x%x", n);
            ret = 0;
        }
        else
            pPppIf->xmit_accm[n >> 5] |= 1 << (n & 0x1F);

        while (*p == ',' || *p == ' ')
            ++p;
    }

    return ret;
}
#endif

#if 0
/*
 * setspeed - Set the speed.
 */
static int
setspeed(PPP_IF_VAR_T *pPppIf, int arg, int is_string)
{
    char *ptr;
    int spd;

    if (is_string)
    {
        spd = strtol((char *)arg, &ptr, 0);
        if (ptr == (char *)arg || *ptr != 0 || spd == 0)
            return 0;
        pPppIf->inspeed = spd;
    }
    else 
        pPppIf->inspeed = arg;

    return 1;
}
#endif

# define	MAXPATHLEN	NET_IFNAME_LEN
/*
 * setdevname - Set the device name.
 */
int
setdevname(PPP_IF_VAR_T *pPppIf, char *cp)
{
    (void) strncpy(pPppIf->ifname, cp, NET_IFNAME_LEN);
    pPppIf->ifname[NET_IFNAME_LEN-1] = 0;
  
    return 1;
}


/*
 * setipaddr - Set the IP address
 *
 */
int
setipaddr(PPP_IF_VAR_T *pPppIf, u_long local_addr, u_long remote_addr)
{
    ipcp_options *wo = &pPppIf->ipcp_wantoptions;

    if (local_addr != 0)
        wo->ouraddr = local_addr;
    
    if (remote_addr != 0)
        wo->hisaddr = remote_addr;

    return 1;
}

/*
 * setipcpaccl - accept peer's idea of our address
 */
static int
setipcpaccl(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.accept_local = 1;
    return 1;
}

/*
 * setipcpaccr - accept peer's idea of its address
 */
static int
setipcpaccr(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.accept_remote = 1;
    return 1;
}



/*
 * setnetmask - set the netmask to be used on the interface.
 */
int setnetmask(PPP_IF_VAR_T *pPppIf, u_long mask)
{
    pPppIf->netmask = mask;
    return (1);
}

/*
 * Return user specified netmask. A value of zero means no netmask has
 * been set.
 */
/* ARGSUSED */
u_long
GetMask(PPP_IF_VAR_T *pPppIf, u_long addr)
{
    return(pPppIf->netmask);
}

#ifdef PPP_OPTION_CMD
static int
setname(PPP_IF_VAR_T *pPppIf, char *argv)
{
    if (pPppIf->our_name[0] == 0)
    {
        strncpy(pPppIf->our_name, argv, MAXNAMELEN);
        pPppIf->our_name[MAXNAMELEN-1] = 0;
    }
    return 1;
}

static int
setuser(PPP_IF_VAR_T *pPppIf, char *argv)
{
    strncpy(pPppIf->user, argv, MAXNAMELEN);
    pPppIf->user[MAXNAMELEN-1] = 0;
    return 1;
}

static int
setpasswd(PPP_IF_VAR_T *pPppIf, char *argv)
{
    strncpy(pPppIf->passwd, argv, MAXSECRETLEN);
    pPppIf->passwd[MAXSECRETLEN-1] = 0;
    return 1;
}

static int
setremote(PPP_IF_VAR_T *pPppIf, char *argv)
{
    strncpy(pPppIf->remote_name, argv, MAXNAMELEN);
    pPppIf->remote_name[MAXNAMELEN-1] = 0;
    return 1;
}
#endif


static int
setdefaultroute(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.default_route = 1;
    return 1;
}

static int
setproxyarp(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->ipcp_wantoptions.proxy_arp = 1;
    return 1;
}

static int
setdologin(PPP_IF_VAR_T *pPppIf)
{
    pPppIf->uselogin = 1;
    return 1;
}

#ifdef PPP_OPTION_CMD
/*
 * Functions to set the echo interval for modem-less monitors
 */

static int
setlcpechointv(PPP_IF_VAR_T *pPppIf, char *argv)
{
    int_option(argv, &pPppIf->lcp_echo_interval);
}

static int
setlcpechofails(PPP_IF_VAR_T *pPppIf, char *argv)
{
    int_option(argv, &pPppIf->lcp_echo_fails);
}

/*
 * Functions to set timeouts, max transmits, etc.
 */
static int
setlcptimeout(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->lcp_fsm.timeouttime);
}

static int setlcpterm(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->lcp_fsm.maxtermtransmits);
}

static int setlcpconf(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->lcp_fsm.maxconfreqtransmits);
}

static int setlcpfails(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->lcp_fsm.maxnakloops);
}

static int setipcptimeout(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->ipcp_fsm.timeouttime);
}

static int setipcpterm(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->ipcp_fsm.maxtermtransmits);
}

static int setipcpconf(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->ipcp_fsm.maxconfreqtransmits);
}

static int setipcpfails(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->ipcp_fsm.maxnakloops);
}

static int setpaptimeout(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->upap.us_timeouttime);
}

static int setpapreqs(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->upap.us_maxtransmits);
}

static int setchaptimeout(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->chap.timeouttime);
}

static int setchapchal(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->chap.max_transmits);
}

static int setchapintv(PPP_IF_VAR_T *pPppIf, char *argv)
{
    return int_option(argv, &pPppIf->chap.chal_interval);
}
#endif

