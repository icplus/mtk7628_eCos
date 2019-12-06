/* magic.h - PPP Magic Number header */

/*
 * magic.h - PPP Magic Number definitions.
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

#ifndef __INCmagich
#define __INCmagich

#ifdef  __cplusplus
extern "C" {
#endif

/* function declarations */
 
#if defined(__STDC__) || defined(__cplusplus)
  
extern void     magic_init (void);      /* Init the magic number generator */
extern u_long   magic (void);           /* Returns the next magic number */

#else   /* __STDC__ */

extern void     magic_init ();          /* Init the magic number generator */
extern u_long   magic ();               /* Returns the next magic number */

#endif  /* __STDC__ */

#ifdef  __cplusplus
}
#endif

#endif  /* __INCmagich */


