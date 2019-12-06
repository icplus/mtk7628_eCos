/*
 * LICENSE NOTICE.
 *
 * Use of the Microsoft Windows Rally Development Kit is covered under
 * the Microsoft Windows Rally Development Kit License Agreement,
 * which is provided within the Microsoft Windows Rally Development
 * Kit or at http://www.microsoft.com/whdc/rally/rallykit.mspx. If you
 * want a license from Microsoft to use the software in the Microsoft
 * Windows Rally Development Kit, you must (1) complete the designated
 * "licensee" information in the Windows Rally Development Kit License
 * Agreement, and (2) sign and return the Agreement AS IS to Microsoft
 * at the address provided in the Agreement.
 */

/*
 * Copyright (c) Microsoft Corporation 2005.  All rights reserved.
 * This software is provided with NO WARRANTY.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "globals.h"
#include <sys/select.h>

/* Static list of event_t's, sorted by their firing times */
static event_t *Events=NULL;
static cyg_flag_t lltd_flag;

/* Limited to just a handful of IO handler for ease: */
#define NIOS 2


/* Add a new event to the list, calling "function(state)" at "firetime". */
event_t *
event_add(struct timeval *firetime, event_fn_t function, void *state)
{
    event_t *prev = NULL;
    event_t *here = Events;

    event_t *ne = xmalloc(sizeof(event_t));
    ne->ev_firetime = *firetime;
    ne->ev_function = function;
    ne->ev_state    = state;
    ne->ev_next     = NULL;

    while (here != NULL && timerle(&here->ev_firetime, &ne->ev_firetime))
    {
	prev = here;
	here = here->ev_next;
    }

    ne->ev_next = here;
    if (!prev)
	Events = ne;
    else
	prev->ev_next = ne;

    return ne;
}


/* Cancel a previously requested event; return TRUE if successful, FALSE if the
 * event wasn't found. */
bool_t
event_cancel(event_t *event)
{
    event_t *prev = NULL;
    event_t *here = Events;

    while (here != NULL)
    {
	if (here == event)
	{
	    if (prev)
		prev->ev_next = here->ev_next;
	    else
		Events = here->ev_next;
	    xfree(here);
	    return TRUE;
	}

	prev = here;
	here = here->ev_next;
    }

    return FALSE;
}


/* Capture the current thread, and run event and IO handlers forever.
 * Does not return. */
extern int lltd_msgbox_id;

void
event_mainloop(void)
{
    struct timeval now;
    int ret;

#ifdef ADJUSTING_FOR_EARLY_WAKEUP
    /* If your OS can wake you before the full timeout has elapsed, we
     * end up looping through and not running any event functions,
     * then setting a really short timeout (~10us!) to get back on
     * track.  We avoid this by adding wake_lead_time to the timeout
     * which we calculate dynamically based on how often we get
     * spurious wakeups.  We reduce the lead time if its been correct
     * for a while. */
	int wake_lead_time = 0;
	int wake_time_correct = 0;
	DBGPRINT(RT_DEBUG_TRACE,"3.Head=%d.....\n",lltdfree_head);        // Create mail box

#endif

    while (1)
    {
        if (Events != NULL)
        {
	        	/* run event functions */
			gettimeofday(&now, NULL);
			while (Events != NULL && timercmp(&now, &Events->ev_firetime, >=))
			{
				event_t *e = Events;
				Events = Events->ev_next;
				e->ev_function(e->ev_state);
				xfree(e);
				gettimeofday(&now, NULL);
	            }
        }

	if (lltd_msgbox_id != -1) {	
		LLTD_CMD_STRUCT      *msg;
	       cyg_tick_count_t start_time = cyg_current_time();

		msg = cyg_mbox_timed_get((cyg_handle_t)lltd_msgbox_id, start_time + 10);
		if (msg)
		{
      			lltd_packetio_recv_handler(msg->ifp,msg->inpkt,msg->len);
			free(msg);
		}
	}

    }

}

