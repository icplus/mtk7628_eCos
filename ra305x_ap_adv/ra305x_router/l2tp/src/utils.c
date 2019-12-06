
/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>
#include <l2tp.h>
#include <l2tp_ppp.h>
//#include <sys/random.h>

#define MAX_ERRMSG_LEN 512
int Sock = -1;
static int random_fd = -1;

static char errmsg[MAX_ERRMSG_LEN];

struct sd_handler {
    l2tp_shutdown_func f;
    void *data;
};

static L2tpHandler *NetworkReadHandler = NULL;
static void network_readable(L2tpSelector *es, int fd, unsigned int flags, void *data);
static void DestroySelector(L2tpSelector *es);
static void DestroyHandler(L2tpHandler *eh);
void l2tp_PendingChanges(L2tpSelector *es);

char L2tp_Hostname[MAX_HOSTNAME];


char const *
l2tp_get_errmsg(void)
{
    return errmsg;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_set_errmsg(char const *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(errmsg, MAX_ERRMSG_LEN, fmt, ap);
    va_end(ap);
    errmsg[MAX_ERRMSG_LEN-1] = 0;
    fprintf(stderr, "Error: %s\n", errmsg);
}

/*--------------------------------------------------------------
* ROUTINE NAME -   l2tp_random_init       
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void
l2tp_random_init(void)
{
    srand(time(NULL));
}

/*--------------------------------------------------------------
* ROUTINE NAME - bad_random_fill         
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void bad_random_fill(void *ptr, size_t size)
{
    unsigned char *buf = ptr;
    while(size--) 
	{
		*buf++ = random() & 0xFF;
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME -  l2tp_random_fill        
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void
l2tp_random_fill(void *ptr, size_t size)
{
    int n;
    int ndone = 0;
    int nleft = size;
    unsigned char *buf = ptr;

    if (random_fd < 0) 
	{
		bad_random_fill(ptr, size);
		return;
    }

    while(nleft) 
	{
		n = read(random_fd, buf+ndone, nleft);
		if (n <= 0) 
		{
			close(random_fd);
			random_fd = -1;
			bad_random_fill(buf+ndone, nleft);
			return;
		}
		nleft -= n;
		ndone += n;
    }
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int
l2tp_NetworkInit(L2tpSelector *es)
{
    struct sockaddr_in me;
    struct timeval tv = {0, 200000};

    gethostname(L2tp_Hostname, sizeof(L2tp_Hostname));
    L2tp_Hostname[sizeof(L2tp_Hostname)-1] = 0;

    if (Sock >= 0) 
	{
		if (NetworkReadHandler) 
		{
			l2tp_DelHandler(es, NetworkReadHandler);
			NetworkReadHandler = NULL;
		}
		close(Sock);
		Sock = -1;
    }
    Sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (Sock < 0) 
	{
		//l2tp_set_errmsg("network_init: socket: %s", strerror(errno));
		diag_printf("network_init: socket error\n");
		return -1;
    }
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons((uint16_t) 1701);
    if (bind(Sock, (struct sockaddr *) &me, sizeof(me)) < 0) 
	{
		diag_printf("network_init: bind error\n");
		close(Sock);
		Sock = -1;
		return -1;
    }

	setsockopt(Sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	
    /* Set up the network read handler */
    l2tp_AddHandler(es, Sock, EVENT_FLAG_READABLE,
		     network_readable, NULL);
    return Sock;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*	Called when a packet arrives on the UDP socket.
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
static void network_readable(L2tpSelector *es,
		 int fd,
		 unsigned int flags,
		 void *data)
{
    l2tp_dgram *dgram;

    struct sockaddr_in from;
	
    #ifdef CONFIG_L2TP_BOOST_MODE
      dgram = lf_TakeFromBSD_BoostMode(&from);
    #else
      dgram = lf_TakeFromBSD(&from);
	#endif
    if (!dgram) return;

    /* It's a control packet if we get here */
    lt_HandleReceivedCtlDatagram(dgram, es, &from);
    lf_Free(dgram);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_TunnelCtl(L2tpSelector *es, int fd)
{
	int maxfd = -1;
	int r;
	fd_set readfds;
	struct timeval tv = {0, 20000};
	
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	
	maxfd = fd;
	r = select(maxfd+1, &readfds, NULL, NULL, &tv);
	if (r <= 0){
	  diag_printf("Enter %s, no response for tunnel request, error!\n", __func__);
	  return ;
	}else{	
	  network_readable(es, 0, 0, NULL);
	}	
	
	return;	
}

/*--------------------------------------------------------------
* ROUTINE NAME -  l2tp_AuthGenResponse        
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_AuthGenResponse(uint16_t msg_type,
		  char const *secret,
		  unsigned char const *challenge,
		  size_t chal_len,
		  unsigned char buf[16])
{
    MD5_CTX ctx;
    unsigned char id = (unsigned char) msg_type;

    MD5Init(&ctx);
    MD5Update(&ctx, &id, 1);
    MD5Update(&ctx, (unsigned char *) secret, strlen(secret));
    MD5Update(&ctx, challenge, chal_len);
    MD5Final(buf, &ctx);
    DBG(ld_Debug(DBG_AUTH, "auth_gen_response(secret=%s) -> %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n", secret,
	   buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
	   buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]));
}


/*--------------------------------------------------------------
* ROUTINE NAME - l2tp_AddHandler
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
L2tpHandler *l2tp_AddHandler(L2tpSelector *es,
		 int fd,
		 unsigned int flags,
		 CallbackFunc fn,
		 void *data)
{
    L2tpHandler *eh;
	
    /* Specifically disable timer and deleted flags */
    flags &= (~(EVENT_TIMER_BITS | EVENT_FLAG_DELETED));
	
    /* Bad file descriptor */
    if (fd < 0) 
	{
		errno = EBADF;
		return NULL;
    }
    eh = malloc(sizeof(L2tpHandler));
    if (!eh) return NULL;
	
    eh->fd = fd;
    eh->flags = flags;
    eh->tmout.tv_usec = 0;
    eh->tmout.tv_sec = 0;
    eh->fn = fn;
    eh->data = data;
	
    /* Add immediately.  This is safe even if we are in a handler. */
    eh->next = es->handlers;
    es->handlers = eh;
	
    return eh;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
L2tpHandler *l2tp_AddHandlerWithTimeout(L2tpSelector *es,
			    int fd,
			    unsigned int flags,
			    struct timeval t,
			    CallbackFunc fn,
			    void *data)
{
    L2tpHandler *eh;
    struct timeval now;
	
    /* If timeout is negative, just do normal non-timing-out event */
    if (t.tv_sec < 0 || t.tv_usec < 0) 
	{
		return l2tp_AddHandler(es, fd, flags, fn, data);
    }
	
    /* Specifically disable timer and deleted flags */
    flags &= (~(EVENT_FLAG_TIMER | EVENT_FLAG_DELETED));
    flags |= EVENT_FLAG_TIMEOUT;
	
    /* Bad file descriptor? */
    if (fd < 0) 
	{
		errno = EBADF;
		return NULL;
    }
	
    /* Bad timeout? */
    if (t.tv_usec >= 1000000) 
	{
		errno = EINVAL;
		return NULL;
    }
	
    eh = malloc(sizeof(L2tpHandler));
    if (!eh) return NULL;
	
    /* Convert time interval to absolute time */
    gettimeofday(&now, NULL);
	
    t.tv_sec += now.tv_sec;
    t.tv_usec += now.tv_usec;
    if (t.tv_usec >= 1000000) 
	{
		t.tv_usec -= 1000000;
		t.tv_sec++;
    }
	
    eh->fd = fd;
    eh->flags = flags;
    eh->tmout = t;
    eh->fn = fn;
    eh->data = data;
	
    /* Add immediately.  This is safe even if we are in a handler. */
    eh->next = es->handlers;
    es->handlers = eh;
	
    return eh;
}


/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
L2tpHandler *l2tp_AddTimerHandler(L2tpSelector *es,
		      struct timeval t,
		      CallbackFunc fn,
		      void *data)
{
    L2tpHandler *eh;
    struct timeval now;
	
    /* Check time interval for validity */
    if (t.tv_sec < 0 || t.tv_usec < 0 || t.tv_usec >= 1000000) 
	{
		errno = EINVAL;
		return NULL;
    }
	
    eh = malloc(sizeof(L2tpHandler));
    if (!eh) return NULL;
	
    /* Convert time interval to absolute time */
    gettimeofday(&now, NULL);
	
    t.tv_sec += now.tv_sec;
    t.tv_usec += now.tv_usec;
    if (t.tv_usec >= 1000000) 
	{
		t.tv_usec -= 1000000;
		t.tv_sec++;
    }
	
    eh->fd = -1;
    eh->flags = EVENT_FLAG_TIMER;
    eh->tmout = t;
    eh->fn = fn;
    eh->data = data;
	
    /* Add immediately.  This is safe even if we are in a handler. */
    eh->next = es->handlers;
    es->handlers = eh;
    return eh;
}
/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
int
l2tp_DelHandler(L2tpSelector *es,
		 L2tpHandler *eh)
{
    /* Scan the handlers list */
    L2tpHandler *cur, *prev;

    for (cur=es->handlers, prev=NULL; cur; prev=cur, cur=cur->next) 
	{
		if (cur == eh) 
		{
			if (es->nestLevel) 
			{
				eh->flags |= EVENT_FLAG_DELETED;
				es->opsPending = 1;
				return 0;
			} 
			else 
			{
				if (prev) prev->next = cur->next;
				else      es->handlers = cur->next;
				
				DestroyHandler(cur);
				return 0;
			}
		}
    }
	
    /* Handler not found */
    return 1;
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void
DestroySelector(L2tpSelector *es)
{
    L2tpHandler *cur, *next;
    for (cur=es->handlers; cur; cur=next) 
	{
		next = cur->next;
		DestroyHandler(cur);
    }

    free(es);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void
DestroyHandler(L2tpHandler *eh)
{
    free(eh);
}

/*--------------------------------------------------------------
* ROUTINE NAME -          
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_PendingChanges(L2tpSelector *es)
{
    L2tpHandler *cur, *prev, *next;
	
    es->opsPending = 0;
	
    /* If selector is to be deleted, do it and skip everything else */
    if (es->destroyPending) 
	{
		DestroySelector(es);
		return;
    }
	
    /* Do deletions */
    cur = es->handlers;
    prev = NULL;
    while(cur) 
	{
		if (!(cur->flags & EVENT_FLAG_DELETED)) 
		{
			prev = cur;
			cur = cur->next;
			continue;
		}
		
		/* Unlink from list */
		if (prev) {
			prev->next = cur->next;
		} else {
			es->handlers = cur->next;
		}
		next = cur->next;
		DestroyHandler(cur);
		cur = next;
    }
}

/*--------------------------------------------------------------
* ROUTINE NAME -l2tp_ChangeTimeout
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None 
* OUTPUT:   None 
* RETURN:   None                                               
* NOTE:     
*---------------------------------------------------------------*/
void l2tp_ChangeTimeout(L2tpHandler *h, struct timeval t)
{
    struct timeval now;
	
    /* Check time interval for validity */
    if (t.tv_sec < 0 || t.tv_usec < 0 || t.tv_usec >= 1000000) 
	{
		return;
    }
    /* Convert time interval to absolute time */
    gettimeofday(&now, NULL);
	
    t.tv_sec += now.tv_sec;
    t.tv_usec += now.tv_usec;
    if (t.tv_usec >= 1000000) 
	{
		t.tv_usec -= 1000000;
		t.tv_sec++;
    }
	
    h->tmout = t;
}

