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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <varlist.h>

#define MAX_FIELD_LEN   4096

/*==============================================================================
 * Function:    init_varlist()
 *------------------------------------------------------------------------------
 * Description: Initialize a variable list
 *
 *
 *============================================================================*/
void init_varlist
(
    VAR_LIST *var_list
)
{
    var_list->head = NULL;
    var_list->tail = NULL;
}

/*==============================================================================
 * Function:    free_varlist()
 *------------------------------------------------------------------------------
 * Description: Free memory allocated for a variable list.
 *
 *============================================================================*/
int free_varlist
(
    VAR_LIST *var_list
)
{
    VAR_FIELD *field;
    VAR_FIELD *temp;
	
    field = var_list->head;
    while (field) 
    {
        temp = field;

        field = field->next;
        if (temp->name)       
            free(temp->name);  // system

        if (temp->value)
            free(temp->value);  // system

        free(temp);  // system
    }

    return 0;
}

/*==============================================================================
 * Function:    set_var()
 *------------------------------------------------------------------------------
 * Description: Allocate and save a (name, value) tuple.
 *
 *============================================================================*/
int set_var
(
    VAR_LIST *var_list,
    char *name,
    char *value
)
{
    VAR_FIELD *field;

    /* check for strings too long */
    if ((strlen(name) >= MAX_FIELD_LEN) || (strlen(value) >= MAX_FIELD_LEN))
        return -1;

    /* allocate a new field */
    field = (VAR_FIELD *)calloc(sizeof(VAR_FIELD), 1);
    if (!field)
        return -1;

    if (strlen(name))
    {
        /* allocate memory for the field name  */
        field->name = (char *)calloc(strlen(name) + 1, 1);
    }

    if (!field->name)
    {
        free(field);
        return -1;
    }

    /* allocate memory for the field value */
    field->value = (char *)calloc(strlen(value) + 1, 1);
    if (!field->value)
    {
        free(field->name);
        free(field);
        return -1;
    }

    strcpy(field->name, name);
    strcpy(field->value, value);

    /* append to the variable list */
    if (var_list->head == NULL)
    {
        var_list->head = field;
        field->prev = NULL;
    }
    else
    {
        field->prev = var_list->tail;
        var_list->tail->next = field;
    }

    var_list->tail = field;
    field->next = NULL;

    return 0;
}

/*==============================================================================
 * Function:    get_var()
 *------------------------------------------------------------------------------
 * Description: Retrive the field value by name
 *
 *============================================================================*/
char *get_var
(
    VAR_LIST *var_list,
    char *name
)
{
    VAR_FIELD *field;

    field = var_list->head;
    while (field)
    {
        if (strcmp(field->name, name) == 0)
        {
            /* got it */
            return field->value;
        }

        field = field->next;   
    }

    /* not found */
    return NULL;
}

/*==============================================================================
 * Function:    unset_var()
 *------------------------------------------------------------------------------
 * Description: Remove a field from the variable list
 *
 *============================================================================*/
int unset_var
(
    VAR_LIST *var_list, 
    char *name
)
{
    VAR_FIELD *field;

    field = var_list->head;
    while (field)
    {
        if (strcmp(field->name, name) == 0)
        {
            /* got it */
            if (field->prev)
                field->prev->next = field->next;

            if (field->next) 
                field->next->prev = field->prev;

            if (var_list->head == field) 
                var_list->head = field->next;

            if (var_list->tail == field)
                var_list->tail = field->prev;
 
            /* free memory */
            free(field->name);
            free(field->value);           
            free(field);
			
            return 0;
        }

        field = field->next;   
    }

    return -1;
}

