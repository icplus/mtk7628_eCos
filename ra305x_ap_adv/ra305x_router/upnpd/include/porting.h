#ifndef __UPNP_PORTING_H_
#define __UPNP_PORTING_H_

#ifdef __ECOS

//************************
// ECOS platform
//************************
#include <network.h>
#include <sys_status.h>
#include <cyg/kernel/kapi.h>
#include <cfg_def.h>
#include <cfg_net.h>


#define BSD_SOCK
#undef	BIND_TO_INADDR_ANY

#define upnp_syslog(args...)

#define	upnp_pid()				((unsigned long)cyg_thread_self())
#define	upnp_thread_exit()		return
#define	upnp_thread_sleep(n)	cyg_thread_delay(n)

// I will merge DHCPD/CHCPClient raw send together
#define	ssdp_lpf_send(context)	ssdp_send(context)

#else	/* Linux Platform */

//***************************
// Linux Platform
//***************************
#include <cgi_file.h>

#undef	BSD_SOCK
#define	BIND_TO_INADDR_ANY

#include <syslog.h>
#define upnp_syslog			syslog

#define	upnp_pid()				((unsigned long)getpid())
#define	upnp_thread_exit()		pthread_exit(NULL)
#define	upnp_thread_sleep(n)	usleep((n)*10000)

#endif

// Function should be port to per platform
int 	upnp_read_config(void *);
int		upnp_if_hwaddr(void *);

#include <sys/time.h>
time_t	upnp_curr_time(time_t *now);

#ifdef CONFIG_WPS
int		wps_upnp_cmdq_create(void);
int		wps_upnp_cmdq_send(int qid, int op, int fd);
int		wps_upnp_cmdq_rcv(int qid, int *op, int *fd);
int		wps_upnp_cmdq_delete(int qid);

int		wps_upnp_thread_create(void);
#endif // CONFIG_WPS //
int		upnp_cmdq_create(void);
int		upnp_cmdq_send(int qid, int op, int fd);
int		upnp_cmdq_rcv(int qid, int *op, int *fd);
int		upnp_cmdq_delete(int qid);

int		upnp_thread_create(void);
void 	upnp_timer_init(void);
void	upnp_timer_shutdown(void);

#endif	/* __UPNP_PORTING_H_ */


