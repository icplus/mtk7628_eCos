#ifndef SYSLOG_H
#define SYSLOG_H

#include <stdio.h>

#define	SYS_LOG_EMERG	0	/* system is unusable */
#define	SYS_LOG_ALERT	1	/* action must be taken immediately */
#define	SYS_LOG_CRIT	2	/* critical conditions */
#define	SYS_LOG_ERR	3	/* error conditions */
#define	SYS_LOG_WARNING	4	/* warning conditions */
#define	SYS_LOG_NOTICE	5	/* normal but significant condition */
#define	SYS_LOG_INFO	6	/* informational */
#define	SYS_LOG_DEBUG	7	/* debug-level messages */

static inline void syslog(int pri, const char *fmt, ...)
{
    char buf[256];
    va_list ap;  // for variable args
	va_start(ap, fmt); // init specifying last non-var arg
    vsprintf(buf, fmt, ap);
	//printf(fmt, ap);
    diag_printf("%s\n", buf);
    va_end(ap); // end var args
}

#endif /* SYSLOG_H */


