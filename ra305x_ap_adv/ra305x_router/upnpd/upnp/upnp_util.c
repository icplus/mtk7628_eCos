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

#include <upnp.h>


/*==============================================================================
 * Function:    gmt_time()
 *------------------------------------------------------------------------------
 * Description: Get current GMT time
 *
 *============================================================================*/
int gmt_time
(
	char *time_str
)
{
	struct tm btime;  
	time_t curr_time;
	
	static char *day_name[] = 
	{
		"Sun",  "Mon",  "Tue",  "Wed",  "Thu",  "Fri",  "Sat"
	};
	
	static char *mon_name[] = 
	{
		"Jan",  "Feb",  "Mar",  "Apr",  "May",  "Jun", 
		"Jul",  "Aug",  "Sep",  "Oct",  "Nov",  "Dec"
	};
	
	upnp_curr_time(&curr_time);
	gmtime_r(&curr_time, &btime);
	
	sprintf(time_str, "%.3s, %.2d %.3s %d %.2d:%.2d:%.2d GMT",
			day_name[btime.tm_wday],
			btime.tm_mday,
			mon_name[btime.tm_mon],
			1900 + btime.tm_year,
			btime.tm_hour,
			btime.tm_min,
			btime.tm_sec);
	
	return 0;	    
}


/*==============================================================================
 * Function:    translate_value()
 *------------------------------------------------------------------------------
 * Description: Translate value to string according to data type
 *
 *============================================================================*/
void translate_value
(
    int type, 
    UPNP_VALUE *value
)
{
	char *buf = value->str;
	
	switch (type)
	{
	case UPNP_TYPE_STR:
#ifdef CONFIG_WPS
	case UPNP_TYPE_BIN_BASE64:
#endif /* CONFIG_WPS */
		break;
		 
	case UPNP_TYPE_BOOL:
		value->bool = (value->bool ? 1 : 0);
		sprintf(buf, "%d", value->bool);
		break;
		 
	case UPNP_TYPE_I1:
		sprintf(buf, "%d", value->i1); 
		break;
	
	case UPNP_TYPE_I2:
		sprintf(buf, "%d", value->i2); 
		break;
		 
	case UPNP_TYPE_I4:
		sprintf(buf, "%ld", value->i4); 
		break;
		 
	case UPNP_TYPE_UI1:
		sprintf(buf, "%u", value->ui1); 
		break;
		 
	case UPNP_TYPE_UI2:
		sprintf(buf, "%u", value->ui2); 
		break;
	
	case UPNP_TYPE_UI4: 
		sprintf(buf, "%lu", value->ui4); 
		break;
	
	default:
		/* should not be reached */
		*buf = '\0'; 
		break;
	}
	
	return;
}


/*==============================================================================
 * Function:    convert_value()
 *------------------------------------------------------------------------------
 * Description: Convert value from string according to data type
 *
 *============================================================================*/
int convert_value
(
	IN_ARGUMENT *arg
)
{
	int ival;
	unsigned int uval;
	
	switch (arg->type)
	{
	case UPNP_TYPE_STR:
#ifdef CONFIG_WPS
	case UPNP_TYPE_BIN_BASE64:
#endif /* CONFIG_WPS */
		break;
		
	case UPNP_TYPE_BOOL:
		/* 0, false, no for false; 1, true, yes for true */
		if (strcmp(arg->value.str, "0") == 0 ||
			strcmp(arg->value.str, "false") == 0 ||
			strcmp(arg->value.str, "no") == 0)
		{
			arg->value.bool = 0;
		}
		else if (strcmp(arg->value.str, "1") == 0 ||
				 strcmp(arg->value.str, "true") == 0 ||
				 strcmp(arg->value.str, "yes") == 0)
		{
			arg->value.bool = 1;
		}
		else
		{
			return -1;
		}
		break;
		 
	case UPNP_TYPE_I1:
		ival = atoi(arg->value.str);
		if ((ival & 0xffffff00) != 0)
			return -1;
		
		arg->value.i1 = ival;
		break;
	
	case UPNP_TYPE_I2:
		ival = atoi(arg->value.str);
		if ((ival & 0xffff0000) != 0)
			return -1;
			
		arg->value.i2 = ival;
		break;
		 
	case UPNP_TYPE_I4:
		ival = atoi(arg->value.str);
		arg->value.i4 = ival;
		break;
		 
	case UPNP_TYPE_UI1:
		uval = strtoul(arg->value.str, NULL, 10);
		if (uval > 0xff)
			return -1;
		
		arg->value.ui1 = uval;
		break;
		 
	case UPNP_TYPE_UI2:
		uval = strtoul(arg->value.str, NULL, 10);
		if (uval > 0xffff)
			return -1;
		
		arg->value.ui2 = uval;
		break;
	
	case UPNP_TYPE_UI4:
		uval = strtoul(arg->value.str, NULL, 10);
		arg->value.ui4 = uval;
		break;
	
	default:
		break;
	}
	
	return 0;
}


/*==============================================================================
 * Function:    upnp_get_in_value()
 *------------------------------------------------------------------------------
 * Description: Search input argument list for a arguement 
 * and return its value
 *
 *============================================================================*/
UPNP_INPUT	*upnp_get_in_value
(
	IN_ARGUMENT *in_arguments,
	char *arg_name
)
{
	while (in_arguments != NULL)
	{
		if (strcmp(in_arguments->name, arg_name) == 0)
		{
			return &in_arguments->value;
		}
		
		in_arguments = in_arguments->next;   
	} 
	
	return 0;
}


/*==============================================================================
 * Function:    upnp_get_out_value()
 *------------------------------------------------------------------------------
 * Description: Search input argument list for a arguement 
 * and return its value
 *
 *============================================================================*/
UPNP_VALUE	*upnp_get_out_value
(
	OUT_ARGUMENT *out_arguments,
	char *arg_name
)
{
	while (out_arguments != NULL)
	{
		if (strcmp(out_arguments->name, arg_name) == 0)
		{
			return &out_arguments->value;
		}
		
		out_arguments = out_arguments->next;   
	} 
	
	return 0;
}

