/*
 * magic.c - PPP Magic Number routines.
 *
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

#include "stdio.h"
#include "stdlib.h"

typedef unsigned long u_long;
#include "magic.h"


static u_long next;         /* Next value to return */


/*
 * magic_init - Initialize the magic number generator.
 *
 * Computes first magic number and seed for random number generator.
 * Attempts to compute a random number seed which will not repeat.
 * The current method uses the current hostid and current time.
 */
void magic_init()
{
    //next = cyg_current_time();
    //srand((int) next);
	next = cyg_arc4random();
}


/*
 * magic - Returns the next magic number.
 */
u_long magic()
{
    u_long m;

    m = next;
    //#if 0
    //next = (u_long) random();
    //#else
    //next = (u_long) rand();
    //#endif
	next = cyg_arc4random();
	
    return (m);
}


