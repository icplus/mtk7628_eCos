
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#ifdef __ECOS
#include <sys/select.h>
#include <cyg/kernel/kapi.h>
#endif

#include "eloop.h"

bool ralink_1x_enable = 1;

#define	timercmp(tvp, uvp, cmp)						\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	    ((tvp)->tv_usec cmp (uvp)->tv_usec) :			\
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define	timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */

struct eloop_sock {
	int sock;
	void *eloop_data;
	void *user_data;
	void (*handler)(int sock, void *eloop_ctx, void *sock_ctx);
};

struct eloop_timeout {
	struct timeval time;
	void *eloop_data;
	void *user_data;
	void (*handler)(void *eloop_ctx, void *sock_ctx);
	struct eloop_timeout *next;
};

struct eloop_signal {
	int sig;
	void *user_data;
	void (*handler)(int sig, void *eloop_ctx, void *signal_ctx);
};

struct eloop_data {
	int is_start;
	char* packet_tmp;
	int is_cached;
	int cache_len;
	void *user_data;

	int max_sock, reader_count;
	struct eloop_sock *readers;

	struct eloop_timeout *timeout;

	int signal_count;
	struct eloop_signal *signals;

	int terminate;
    int reload;
};

static struct eloop_data eloop;


void eloop_init(void *user_data)
{
	memset(&eloop, 0, sizeof(eloop));
	eloop.user_data = user_data;
}


int eloop_register_read_sock(int sock, void (*handler)(int sock, void *eloop_ctx, void *sock_ctx),
                             void *eloop_data, void *user_data)
{
	struct eloop_sock *tmp;

	tmp = (struct eloop_sock *)	realloc(eloop.readers, (eloop.reader_count + 1) * sizeof(struct eloop_sock));
	if (tmp == NULL)
		return -1;

	tmp[eloop.reader_count].sock = sock;
	tmp[eloop.reader_count].eloop_data = eloop_data;
	tmp[eloop.reader_count].user_data = user_data;
	tmp[eloop.reader_count].handler = handler;
	eloop.reader_count++;
	eloop.readers = tmp;

	if (sock > eloop.max_sock)
		eloop.max_sock = sock;

	return 0;
}

int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   void (*handler)(void *eloop_ctx, void *timeout_ctx),
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *tmp, *prev;

	timeout = (struct eloop_timeout *) malloc(sizeof(*timeout));
	if (timeout == NULL)
		return -1;

	gettimeofday(&timeout->time, NULL);
	timeout->time.tv_sec += secs;
	timeout->time.tv_usec += usecs;

	while (timeout->time.tv_usec >= 1000000)
    {
		timeout->time.tv_sec++;
		timeout->time.tv_usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	timeout->next = NULL;

	if (eloop.timeout == NULL) {
		eloop.timeout = timeout;
		return 0;
	}

	prev = NULL;
	tmp = eloop.timeout;
	while (tmp != NULL)
    {
		if (timercmp(&timeout->time, &tmp->time, <))
			break;
		prev = tmp;
		tmp = tmp->next;
	}

	if (prev == NULL)
    {
		timeout->next = eloop.timeout;
		eloop.timeout = timeout;
	}
    else
    {
		timeout->next = prev->next;
		prev->next = timeout;
	}

	return 0;
}

int eloop_cancel_timeout(void (*handler)(void *eloop_ctx, void *sock_ctx),
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout, *prev, *next;
	int removed = 0;

	prev = NULL;
	timeout = eloop.timeout;
	while (timeout != NULL)
    {
		next = timeout->next;

		if (timeout->handler == handler && (timeout->eloop_data == eloop_data || eloop_data == ELOOP_ALL_CTX) 
            && (timeout->user_data == user_data || user_data == ELOOP_ALL_CTX))
        {
			if (prev == NULL)
				eloop.timeout = next;
			else
				prev->next = next;
			free(timeout);
			removed++;
		} else
			prev = timeout;

		timeout = next;
	}

	return removed;
}

static void eloop_handle_signal(int sig)
{
	int i;
	for (i = 0; i < eloop.signal_count; i++)
    {
		if (eloop.signals[i].sig == sig)
        {
			eloop.signals[i].handler(eloop.signals[i].sig, eloop.user_data, eloop.signals[i].user_data);
			break;
		}
	}
}

int eloop_register_signal(int sig, void (*handler)(int sig, void *eloop_ctx, void *signal_ctx), void *user_data)
{
	struct eloop_signal *tmp;

	tmp = (struct eloop_signal *) realloc(eloop.signals, (eloop.signal_count + 1) *	sizeof(struct eloop_signal));
	if (tmp == NULL)
		return -1;

	tmp[eloop.signal_count].sig = sig;
	tmp[eloop.signal_count].user_data = user_data;
	tmp[eloop.signal_count].handler = handler;
	eloop.signal_count++;
	eloop.signals = tmp;
	signal(sig, eloop_handle_signal);

	return 0;
}

void eloop_run(void)
{
	fd_set rfds;
	int i, res;
	struct timeval tv, now;
	int j=0;
	eloop.is_start=1;
	while(!eloop.terminate && (eloop.timeout || eloop.reader_count > 0))
    	{

		if (eloop.timeout)
        	{
           		 gettimeofday(&now, NULL);
			if (timercmp(&now, &eloop.timeout->time, >=))
				tv.tv_sec = tv.tv_usec = 0;
			else
				timersub(&eloop.timeout->time, &now, &tv);
		} else {
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
                }              

		FD_ZERO(&rfds);
		for (i = 0; i < eloop.reader_count; i++)
		{
			FD_SET(eloop.readers[i].sock, &rfds);
		}

		res = select(eloop.max_sock + 1, &rfds, NULL, NULL, &tv);

		if (res < 0 && errno != EINTR)
        	{
			perror("select");
                        eloop.is_start=0;
			return ;
		}

		/* check if some registered timeouts have occurred */
		if (eloop.timeout)
        	{

			struct eloop_timeout *tmp;

			gettimeofday(&now, NULL);
			if (timercmp(&now, &eloop.timeout->time, >=))
            		{
				tmp = eloop.timeout;
				eloop.timeout = eloop.timeout->next;
				tmp->handler(tmp->eloop_data, tmp->user_data);
				free(tmp);
			}

		}  
        
		if (res <= 0)
		{
			continue;
		}
		for (i = 0; i < eloop.reader_count; i++)
        	{
			if (FD_ISSET(eloop.readers[i].sock, &rfds))
            		{
				eloop.readers[i].handler(eloop.readers[i].sock,eloop.readers[i].eloop_data, eloop.readers[i].user_data);
			}
		}
	}

        eloop.is_start=0;
}


void eloop_terminate(void)
{
	eloop.terminate = 1;
}

void eloop_reload(void)
{
	eloop.reload = 1;
}

void eloop_destroy(void)
{
	struct eloop_timeout *timeout, *prev;

	timeout = eloop.timeout;
	while (timeout != NULL)
        {
		prev = timeout;
		timeout = timeout->next;
		free(prev);
	}
	free(eloop.readers);
	free(eloop.signals);
}

int eloop_terminated(void)
{
	return eloop.terminate;
}

void Dot1xNetReceive(struct ifnet *ifp, unsigned char *inpkt, int len)
{
	int i=0;
	unsigned char* tmp_buf=NULL;
	diag_printf("===>Dot1xNetReceive len=%d\n",len);
	/* 
	 * Check packet len 
	 */
	if (len < (ETH_HLEN /* sizeof(struct racfghdr) */)) 
	{
		diag_printf("Dot1xNetReceive packet len is too short failed!!\n");
		return;
	}
	if(!eloop.is_start)
	{
		diag_printf("Dot1xNetReceive (!eloop.is_start)\n");
		return;
        }

	tmp_buf=malloc(len);
	if(tmp_buf)
	{
	memcpy(tmp_buf,inpkt,len);
	Ecos_Handle_read(eloop.readers[i].sock,eloop.readers[i].eloop_data, eloop.readers[i].user_data, tmp_buf,len);
	free(tmp_buf);
	}
	else
		diag_printf("Dot1xNetReceive malloc tmp_buf fail\n");

}



