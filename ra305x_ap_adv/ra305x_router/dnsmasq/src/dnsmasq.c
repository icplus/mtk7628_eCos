#include "dnsmasq.h"
#include <cfg_def.h> 
#include <cfg_net.h>
#include <cfg_dns.h>
#include <sys_status.h>

#define DNS_SIGHUP      1
#define DNS_SHUTDOWN    2

#define DNS_PRIORITY    9
#define DNS_STACKSIZE 6*1024//16*1024	//chang DNS_STACKSIZE 16*1024->6*1024
static char dns_stack[DNS_STACKSIZE];
cyg_handle_t dns_handle ;
cyg_thread dns_thread ;

static cyg_mbox dns_mbox_obj;
static cyg_handle_t dns_mbox_id;
static int dns_cmd;

IFENTRY recv_infaces[MAXRECVINFACE];
int recv_infaces_num = 0;

DNSSERVER dns_servers[MAXDNSSERVER];
int dns_servers_num = 0;

static DNSSERVER *activeserver;
static int dns_running = 0;

void DNS_daemon(cyg_addrword_t data)
{
	DNSSERVER *new_active_srv;
	int port = NAMESERVER_PORT;
	int first_loop	= 1;
	int recv_len	= 0;
	int i, cmd, dns_sleep, ret;
	struct timeval	tv = {1, 0};
	time_t	now;
	char	packet[PACKETSZ+MAXDNAME+RRFIXEDSZ];

        DNS_DBGPRINT(DNS_DEBUG_OFF, "%s\n", __FUNCTION__);
	ret = dnsMasqstart(port);
	if (ret < 0)
	{
		DNS_DBGPRINT(DNS_DEBUG_OFF, "%s(): Line%d failed to start up\n", __FUNCTION__, __LINE__);
		return;
	}

  	activeserver = &dns_servers[0];
  	dns_running = 1;
  	dns_sleep = 0;
  	while (dns_running)
  	{
                int ready, maxfd = 0;
                fd_set rset;
                DNSHEADER *header;
   		
   		cyg_thread_delay(10);
   		if ((cmd = (int )cyg_mbox_tryget(dns_mbox_id)) != 0)
                {
    	switch(cmd)
			{
	  		case DNS_SIGHUP:
				ret = dnsMasqstart(port);
				if (ret < 0)
				{
					DNS_DBGPRINT(DNS_DEBUG_OFF, "%s(): Line%d failed to start up\n", __FUNCTION__, __LINE__);
					dns_running = 0;
					return;
				}
				dns_sleep = 0;
				break;

			case DNS_SHUTDOWN: // do we need to return ??
				dnsMasqstop();
				dns_sleep = 1;
				break;

			default:
				break;
			}
		}

		if (dns_sleep)
		{
			cyg_thread_delay(100);
			continue;
		}
		
		/* do init stuff only first time round. */
   		if (first_loop)
		{
			first_loop = 0;
		  	ready = 0;
		}
   		else
		{
			FD_ZERO(&rset);
		        maxfd = 0;
			for (i=0; i<dns_servers_num; i++)
			{
				if (dns_servers[i].fd > 0)
				{
					FD_SET(dns_servers[i].fd, &rset);
					if (dns_servers[i].fd > maxfd)
					maxfd = dns_servers[i].fd;
				}
			}

			for (i=0; i<recv_infaces_num; i++)
			{
				if (recv_infaces[i].fd > 0)
				{
					FD_SET(recv_infaces[i].fd, &rset);
					if (recv_infaces[i].fd > maxfd)
					maxfd = recv_infaces[i].fd;
				}
			}

			ready = select(maxfd+1, &rset, NULL, NULL, &tv); /* NONBLOCKING */
			if(ready <= 0)
			{
			        if (DNSDebugLevel > DNS_DEBUG_OFF) {
                                        DNS_DBGPRINT(DNS_DEBUG_ERROR, "%s(): select() return %d\n", __FUNCTION__, ready);
        				cyg_thread_delay(100);
                                } else
        				cyg_thread_delay(2);
                                
				continue;
			}  
		}

                now = time(NULL);
                if (ready == 0)
			continue; /* no sockets ready */


		// check dns query from querist
		for (i=0; i<recv_infaces_num; i++)
		{
			if (recv_infaces[i].fd > 0)
			{
				if (FD_ISSET(recv_infaces[i].fd, &rset))
				{
					// request packet, deal with query
					MYSOCKADDR queryaddr;
					size_t queryaddrlen = sizeof(queryaddr);

					FD_CLR(recv_infaces[i].fd, &rset);
					recv_len = recvfrom(recv_infaces[i].fd, packet, PACKETSZ, 0, &queryaddr.sa, &queryaddrlen); 
					queryaddr.sa.sa_family = recv_infaces[i].addr.sa.sa_family;
					header = (DNSHEADER *)packet;
					if (recv_len >= (int)sizeof(DNSHEADER) && !header->qr)
					{
						new_active_srv = dnsMasqprocessquery(recv_infaces[i].fd, &queryaddr, header, recv_len, 
															 activeserver, now);
						if (new_active_srv != NULL)
							activeserver = new_active_srv;
					}	      
	    		        }
			}
		}

		// check dns reply from dns server
		for (i=0; i<dns_servers_num; i++)
		{
			if (dns_servers[i].fd > 0)
			{
				if (FD_ISSET(dns_servers[i].fd, &rset))
				{
					FD_CLR(dns_servers[i].fd, &rset);
					recv_len = recvfrom(dns_servers[i].fd, packet, PACKETSZ, 0, NULL, NULL); 
					new_active_srv = dnsMasqprocessreply(dns_servers[i].fd, packet, recv_len, now);
					if (new_active_srv != NULL)
						activeserver = new_active_srv;
				}
			}
		}
        }
}

void DNS_init(void) 
{
	int val=0;

	CFG_get(CFG_DNS_EN, &val);
	if(val)
	{
		cyg_mbox_create( &dns_mbox_id, &dns_mbox_obj );	
		cyg_thread_create(DNS_PRIORITY, &DNS_daemon, 0, "DNS_daemon",
                     &dns_stack, DNS_STACKSIZE,
                     &dns_handle, &dns_thread);
		cyg_thread_resume(dns_handle);	
	}
   	
}

void DNS_update(void)
{
	int val=0;

	CFG_get(CFG_DNS_EN, &val);
	if(val)
	{
		if(dns_running)
			dns_cmd = DNS_SIGHUP;
		else
		{
			DNS_init();
			return;
		}
	}
	else
	{
		if(dns_running)
			dns_cmd = DNS_SHUTDOWN;
		else
			return;
	}
	cyg_mbox_tryput(dns_mbox_id, (void*)dns_cmd);
}

void DNS_shutdown(void)
{
	if(dns_running)
                dns_cmd = DNS_SHUTDOWN;
        else
                return;
	cyg_mbox_tryput(dns_mbox_id, (void*)dns_cmd);
}

void DNS_cache_dump(void)
{
        dnsMasqforwarddump();
}

void DNS_forward_dump(void)
{
        dnsMasqforwarddump();
}

