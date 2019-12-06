/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#if !defined(__SVR4) && !defined(__svr4__)
//#include <strings.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
//#include <syslog.h>
#include "facpri.h"

#ifndef __STDC__
# define	const
#endif

#if !defined(lint)
static const char rcsid[] = "@(#)$Id: facpri.c,v 1.1 2010/01/28 11:09:57 dennis Exp $";
#endif

typedef	struct	table	{
	char	*name;
	int	value;
} table_t;

#if 0 //yfchou
table_t	facs[] = {
	{ "kern", LOG_KERN },	{ "user", LOG_USER },
	{ "mail", LOG_MAIL },	{ "daemon", LOG_DAEMON },
	{ "auth", LOG_AUTH },	{ "syslog", LOG_SYSLOG },
	{ "lpr", LOG_LPR },	{ "news", LOG_NEWS },
	{ "uucp", LOG_UUCP },
#if LOG_CRON == LOG_CRON2 
	{ "cron2", LOG_CRON1 },
#else
	{ "cron", LOG_CRON1 },
#endif
#ifdef	LOG_FTP
	{ "ftp", LOG_FTP },
#endif
#ifdef	LOG_AUTHPRIV
	{ "authpriv", LOG_AUTHPRIV },
#endif
#ifdef	LOG_AUDIT
	{ "audit", LOG_AUDIT },
#endif
#ifdef	LOG_LFMT
	{ "logalert", LOG_LFMT },
#endif
#if LOG_CRON == LOG_CRON1
	{ "cron", LOG_CRON2 },
#else
	{ "cron2", LOG_CRON2 },
#endif
#ifdef	LOG_SECURITY
	{ "security", LOG_SECURITY },
#endif
	{ "local0", LOG_LOCAL0 },	{ "local1", LOG_LOCAL1 },
	{ "local2", LOG_LOCAL2 },	{ "local3", LOG_LOCAL3 },
	{ "local4", LOG_LOCAL4 },	{ "local5", LOG_LOCAL5 },
	{ "local6", LOG_LOCAL6 },	{ "local7", LOG_LOCAL7 },
	{ NULL, 0 }
};
#endif

/*
 * map a facility number to its name
 */
char *
fac_toname(facpri)
	int facpri;
{
#if 0 //yfchou

	int	i, j, fac;
	fac = facpri & LOG_FACMASK;
	j = fac >> 3;
	if (j < 24) {
		if (facs[j].value == fac)
			return facs[j].name;
		for (i = 0; facs[i].name; i++)
			if (fac == facs[i].value)
				return facs[i].name;
	}
#endif
	return NULL;
}


/*
 * map a facility name to its number
 */
int
fac_findname(name)
	char *name;
{
#if 0  //yfchou

	int	i;
	for (i = 0; facs[i].name; i++)
		if (!strcmp(facs[i].name, name))
			return facs[i].value;
#endif			
	return -1;
}

//yfchou add
#define	SYS_LOG_EMERG	0	/* system is unusable */
#define	SYS_LOG_ALERT	1	/* action must be taken immediately */
#define	SYS_LOG_CRIT	2	/* critical conditions */
#define	SYS_LOG_ERR		3	/* error conditions */
#define	SYS_LOG_WARNING	4	/* warning conditions */
#define	SYS_LOG_NOTICE	5	/* normal but significant condition */
#define	SYS_LOG_INFO	6	/* informational */
#define	SYS_LOG_DEBUG	7	/* debug-level messages */
#define	SYS_LOG_PRIMASK	0x07	/* mask to extract priority part (internal) */

table_t	pris[] = {
	{ "emerg", SYS_LOG_EMERG },
        { "alert", SYS_LOG_ALERT  },
	{ "crit", SYS_LOG_CRIT },
	{ "err", SYS_LOG_ERR  },
	{ "warn", SYS_LOG_WARNING },
	{ "notice", SYS_LOG_NOTICE  },
	{ "info", SYS_LOG_INFO },
	{ "debug", SYS_LOG_DEBUG  },
	{ NULL, 0 }
};


/*
 * map a priority name to its number
 */
int
pri_findname(name)
	char *name;
{
	int	i;

	for (i = 0; pris[i].name; i++)
		if (!strcmp(pris[i].name, name))
			return pris[i].value;
	return -1;
}


/*
 * map a priority number to its name
 */
char *
pri_toname(facpri)
	int facpri;
{
	int	i, pri;

	pri = facpri & SYS_LOG_PRIMASK;
	if (pris[pri].value == pri)
		return pris[pri].name;
	for (i = 0; pris[i].name; i++)
		if (pri == pris[i].value)
			return pris[i].name;
	return NULL;
}


