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

#ifndef EVENT_H
#define EVENT_H

/* socket and timeout event dispatcher */
#include <sys/time.h>

/* Add a new event to the list, calling "function(state)" at "firetime". */
extern event_t *event_add(struct timeval *firetime, event_fn_t function, void *state);

/* Cancel a previously requested event; return TRUE if successful, FALSE if the
 * event wasn't found. */
extern bool_t event_cancel(event_t *event);

/* Capture the current thread, and run event and IO handlers forever.
 * Does not return. */
extern void event_mainloop(void);

#endif /* EVENT_H */
