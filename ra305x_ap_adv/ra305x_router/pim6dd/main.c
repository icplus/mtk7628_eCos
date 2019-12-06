/*
 *  Copyright (c) 1998 by the University of Oregon.
 *  All rights reserved.
 *
 *  Permission to use, copy, modify, and distribute this software and
 *  its documentation in source and binary forms for lawful
 *  purposes and without fee is hereby granted, provided
 *  that the above copyright notice appear in all copies and that both
 *  the copyright notice and this permission notice appear in supporting
 *  documentation, and that any documentation, advertising materials,
 *  and other materials related to such distribution and use acknowledge
 *  that the software was developed by the University of Oregon.
 *  The name of the University of Oregon may not be used to endorse or 
 *  promote products derived from this software without specific prior 
 *  written permission.
 *
 *  THE UNIVERSITY OF OREGON DOES NOT MAKE ANY REPRESENTATIONS
 *  ABOUT THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  THIS SOFTWARE IS
 *  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, TITLE, AND 
 *  NON-INFRINGEMENT.
 *
 *  IN NO EVENT SHALL UO, OR ANY OTHER CONTRIBUTOR BE LIABLE FOR ANY
 *  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, WHETHER IN CONTRACT,
 *  TORT, OR OTHER FORM OF ACTION, ARISING OUT OF OR IN CONNECTION WITH,
 *  THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  Other copyrights might apply to parts of this software and are so
 *  noted when applicable.
 */
/*
 *  Questions concerning this software should be directed to 
 *  Kurt Windisch (kurtw@antc.uoregon.edu)
 *
 *  $Id: main.c,v 1.4 2000/10/05 22:20:38 itojun Exp $
 */
/*
 * Part of this program has been derived from PIM sparse-mode pimd.
 * The pimd program is covered by the license in the accompanying file
 * named "LICENSE.pimd".
 *  
 * The pimd program is COPYRIGHT 1998 by University of Southern California.
 *
 * Part of this program has been derived from mrouted.
 * The mrouted program is covered by the license in the accompanying file
 * named "LICENSE.mrouted".
 * 
 * The mrouted program is COPYRIGHT 1989 by The Board of Trustees of
 * Leland Stanford Junior University.
 *
 * $FreeBSD$
 */

#include "defs.h"

static int pimd_running = 0;
static int pimd_doing_shutdown = 0;
static cyg_handle_t pimd_thread_h;
static cyg_thread pimd_thread;
static char pimd_stack[4096];

#define NHANDLERS       3

static struct ihandler {
    int fd;			/* File descriptor               */
    ihfunc_t func;		/* Function to call with &fd_set */
} ihandlers[NHANDLERS];
static int nhandlers = 0;


/*
 * Forward declarations.
 */
static void timer __P((void *));

int
register_input_handler(fd, func)
    int fd;
    ihfunc_t func;
{
    if (nhandlers >= NHANDLERS)
	return -1;
    
    ihandlers[nhandlers].fd = fd;
    ihandlers[nhandlers++].func = func;
    
    return 0;
}

void pimd_daemon(cyg_addrword_t data)
{
	struct timeval tv, difftime, curtime, lasttime, *timeout;
	fd_set rfds, readers;
 	int nfds, n, i, secs;
 	char c;
	int tmpd;

	FD_ZERO(&readers);
	FD_SET(mld6_socket, &readers);
	nfds = mld6_socket + 1;
	for (i = 0; i < nhandlers; i++) {
		FD_SET(ihandlers[i].fd, &readers);
		if (ihandlers[i].fd >= nfds)
			nfds = ihandlers[i].fd + 1;
	}
	
	/* schedule first timer interrupt */
	timer_setTimer(TIMER_INTERVAL, timer, NULL);
	
#ifdef HAVE_ROUTING_SOCKETS
	init_routesock();
#endif /* HAVE_ROUTING_SOCKETS */
			
	/*
	 * Main receive loop.
	 */
	difftime.tv_usec = 0;
	gettimeofday(&curtime, NULL);
	lasttime = curtime;
	while(pimd_running) {
		bcopy((char *)&readers, (char *)&rfds, sizeof(rfds));
		secs = timer_nextTimer();
		if (secs == -1)
			timeout = NULL;
		else {
		   timeout = &tv;
		   timeout->tv_sec = secs;
		   timeout->tv_usec = 0;
		}

		if ((n = select(nfds, &rfds, NULL, NULL, timeout)) < 0) {
			if (errno != EINTR) /* SIGALRM is expected */
			syslog(LOG_WARNING,  "select failed");
			continue;
		}
	
		/*
		 * Handle timeout queue.
		 *
		 * If select + packet processing took more than 1 second,
		 * or if there is a timeout pending, age the timeout queue.
		 *
		 * If not, collect usec in difftime to make sure that the
		 * time doesn't drift too badly.
		 *
		 * If the timeout handlers took more than 1 second,
		 * age the timeout queue again.  XXX This introduces the
		 * potential for infinite loops!
		 */
		do {
			/*
			 * If the select timed out, then there's no other
			 * activity to account for and we don't need to
			 * call gettimeofday.
			 */
			if (n == 0) {
				curtime.tv_sec = lasttime.tv_sec + secs;
				curtime.tv_usec = lasttime.tv_usec;
				n = -1; /* don't do this next time through the loop */
			} else
				gettimeofday(&curtime, NULL);
				difftime.tv_sec = curtime.tv_sec - lasttime.tv_sec;
				difftime.tv_usec += curtime.tv_usec - lasttime.tv_usec;
				while (difftime.tv_usec >= 1000000) {
					difftime.tv_sec++;
					difftime.tv_usec -= 1000000;
				}
				if (difftime.tv_usec < 0) {
					difftime.tv_sec--;
					difftime.tv_usec += 1000000;
				}
				lasttime = curtime;
				if (secs == 0 || difftime.tv_sec > 0) {
					age_callout_queue(difftime.tv_sec);
				}
				secs = -1;
		} while (difftime.tv_sec > 0);

		/* Handle sockets */
		if (n > 0) {
			/* TODO: shall check first mld6_socket for better performance? */
			for (i = 0; i < nhandlers; i++) {
				if (FD_ISSET(ihandlers[i].fd, &rfds)) {
					(*ihandlers[i].func)(ihandlers[i].fd, &rfds);
				}
			}
		}
		
	} /* Main loop */
	
	free_all_callouts();
	stop_all_vifs();
	nhandlers=0;
	k_stop_pim(mld6_socket);
	close(mld6_socket);
	close(pim6_socket);
	close(udp_socket);

	pimd_doing_shutdown = 0;
	return;
}

void
pimd_stop()
{
	if (pimd_running == 0)
		return;

	diag_printf("pimd stop\n");
	pimd_doing_shutdown = 1;

	pimd_running = 0; 

	while (pimd_doing_shutdown != 0)
		cyg_thread_delay(5);
	return;
}

int
pimd_start(argc, argv)
    int argc;
    char *argv[];
{	
	if (pimd_running)
		return 1;
    
    pim_callout_init();

    init_mld6();
    init_pim6();

    init_pim6_mrt();
    init_timers();
    
    /* TODO: check the kernel DVMRP/MROUTED/PIM support version */    
    init_vifs();

	pimd_running = 1;
	cyg_thread_create(7, (cyg_thread_entry_t *)pimd_daemon, (cyg_addrword_t) NULL, "pimd_thread",
				  (void *)&pimd_stack[0], sizeof(pimd_stack), &pimd_thread_h, &pimd_thread);
	cyg_thread_resume(pimd_thread_h);

    return 0;
}

/*
 * The 'virtual_time' variable is initialized to a value that will cause the
 * first invocation of timer() to send a probe or route report to all vifs
 * and send group membership queries to all subnets for which this router is
 * querier.  This first invocation occurs approximately TIMER_INTERVAL seconds
 * after the router starts up.   Note that probes for neighbors and queries
 * for group memberships are also sent at start-up time, as part of initial-
 * ization.  This repetition after a short interval is desirable for quickly
 * building up topology and membership information in the presence of possible
 * packet loss.
 *
 * 'virtual_time' advances at a rate that is only a crude approximation of
 * real time, because it does not take into account any time spent processing,
 * and because the timer intervals are sometimes shrunk by a random amount to
 * avoid unwanted synchronization with other routers.
 */

u_long virtual_time = 0;

/*
 * Timer routine. Performs all perodic functions:
 * aging interfaces, quering neighbors and members, etc... The granularity
 * is equal to TIMER_INTERVAL.
 */
static void 
timer(i)
    void *i;
{
    age_vifs();	        /* Timeout neighbors and groups         */
    age_routes();  	/* Timeout routing entries              */
    
    virtual_time += TIMER_INTERVAL;
    timer_setTimer(TIMER_INTERVAL, timer, NULL);
}

