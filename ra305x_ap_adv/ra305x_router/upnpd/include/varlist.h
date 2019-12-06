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


#ifndef __VARLIST_H__
#define __VARLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct var_field 
{
    char *name;
    char *value;
    struct var_field *prev;     /* pointer to previous variable */
    struct var_field *next;     /* pointer to next variable */
}
VAR_FIELD;

typedef struct var_list
{
    VAR_FIELD    *head;     /* list head */
    VAR_FIELD    *tail;     /* list tail */
}
VAR_LIST;

/*
 * Functions
 */
void    init_varlist(VAR_LIST *);
int     free_varlist(VAR_LIST *);
int     set_var(VAR_LIST *, char *, char *);
int     unset_var(VAR_LIST *, char *);
char    *get_var(VAR_LIST *, char *);

#ifdef __cplusplus
}
#endif

#endif /* __VARLIST_H__ */


