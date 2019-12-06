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
 ***************************************************************************

    Module Name:
    io_util.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include <stdio.h>
#include <cyg/io/io.h>
#include <cli_prompt.h>

#define STRING_BUFFER_LENGTH 100

char getch(void)
{	char rc;
	char line[STRING_BUFFER_LENGTH];
	
	rc = (char) getchar();
	if (rc != 0x00 && rc != 0x0a && rc != 0x0d)
	{	
		gets(line); // to get ride of garbage character 0x00.
	}

	return rc;
}


int getss(char *buf)
{	int i;

	memset(buf,0x00,STRING_BUFFER_LENGTH);
	gets(buf);
	for (i=0;i<STRING_BUFFER_LENGTH;i++)
		if (buf[i] == 0x00) break;

	return i;
}


char * long2ipstr(unsigned long ip)
{
static char ipstr[15];

#ifdef	BIG_ENDIAN
	sprintf(ipstr,"%d.%d.%d.%d",ip>>24,(ip>>16)&0xff,(ip>>8)&0xff,ip&0xff);
#else
	sprintf(ipstr,"%d.%d.%d.%d",ip&0xff,(ip>>8)&0xff,(ip>>16)&0xff,ip>>24);
#endif

    return ipstr;
}

unsigned long str2ip(char * str)
{
    unsigned int ipa[4];
    unsigned long the_ip;
    int i;

    if ((sscanf(str, "%d.%d.%d.%d", ipa,ipa+1,ipa+2,ipa+3)) != 4 )
        return 0;
            
    for (i=0; i <3; i++)
    {
        if (ipa[i] > 255)
            return 0;
    }

#ifdef	BIG_ENDIAN
    the_ip = (ipa[0] << 24)|(ipa[1] << 16)|(ipa[2] << 8)| ipa[3];
#else
    the_ip = (ipa[3] << 24)|(ipa[2] << 16)|(ipa[1] << 8)| ipa[0];
#endif

    return the_ip;
}


int prompt(char *string, int ptype, void *parm_ptr, int show_value)
{
    unsigned char rc;
    char line[STRING_BUFFER_LENGTH+1];
    unsigned long temp;
	int tmp=0;
    unsigned int ipa[6];
    int i;
    unsigned char * pch;
	int wrong_char;

    printf(string);
	memset(line,0x00,sizeof(line));
    switch (ptype & 0xff )
    {
    case QFLAG:
        rc = (*((unsigned char *)parm_ptr) ? 'Y' : 'N') ;
                if (show_value) 
                        printf(" [%c] ", rc);
                rc=getch();

        switch (rc)
        {
	        case 'Y':
    	    case 'y':	        	
        	    *((unsigned char *)parm_ptr) = 1;
            	break;
               
	        case 'N':
    	    case 'n':	        	
        	    *((unsigned char *)parm_ptr) = 0;
            	break;

	        case '\n':
    	    case '\r':
        	    break;
                
	        default:
    	        printf("\nInvalid Input !\007\n");
        	    return -1;
        }
        break;

    case QIP:
        if (show_value) printf(" [%s] :", long2ipstr( *((unsigned long *)parm_ptr) ));
        if (getss(line))
        {
            if ((line[0]=='\n')|(line[0]=='\r'))
                break;
            temp = str2ip(line) ;
            if (temp)
                *((unsigned long *)parm_ptr) = temp;
            else
                return -1;
        }
        break;

    case QHEX:
                if (show_value) 
                        printf(" [%X] :", *((unsigned int *)parm_ptr));
        if (getss(line))
        {
            if (sscanf(line,"%x",&tmp)>0)
                *((unsigned long *)parm_ptr) = tmp;
            else
                return -1;
        }
        break;

    case QDECIMAL:
                if (show_value) 
                        printf(" [%d] :", *((unsigned int *)parm_ptr));
        if (getss(line))
        {			
            if (sscanf(line,"%d",&tmp)>0)
                *((unsigned int *)parm_ptr) = tmp;
            else
                return -1;
        }
        break;

    case QCHAR:
                if (show_value) 
                        printf(" [%c] :", *((unsigned char *)parm_ptr));
                rc=getch();
                if ((rc!=0x0d)&&(rc!=0x0a))
                    *((unsigned char *)parm_ptr) = rc;
        break;

    case QSTRING:
                if (show_value) 
                        printf(" [%s] :", (unsigned char *)parm_ptr);
        if ((i=getss(line))!=0)
            memcpy((void *)parm_ptr,line,i+1);
        break;

	case QWEPSTRING:
                if (show_value) 
                        printf(" [%s] :", (unsigned char *)parm_ptr);
		wrong_char=0;
                if (getss(line))
		{			
		for (i=0;i<26 && line[i] != 0x00;i++)
		{
			if (!((line[i] >= '0' && line[i] <= '9') || (line[i] >= 'a' && line[i] <= 'f') || (line[i] >= 'A' && line[i] <= 'F')))
			{
				wrong_char=1;
				break;
			}
		}

                        if(wrong_char) 
                                printf("\007Input Character out of range : [0~9], [A~F]\n");
		else
		{
			for (i=0;i<26;i++)
			{
				*((unsigned char *)parm_ptr+i)=line[i];
			}
		}
		break;
                }
                
    case QMAC:
        pch=(unsigned char *)parm_ptr;
        if (show_value) 
			printf(" [%02X:%02X:%02X:%02X:%02X:%02X] :",
                 pch[0]&0xff,pch[1]&0xff,pch[2]&0xff,
                 pch[3]&0xff,pch[4]&0xff,pch[5]&0xff);
            
        if (getss(line))
        {
            if ((line[0]=='\n')|(line[0]=='\r'))
                break;
            if ((sscanf(line, "%x:%x:%x:%x:%x:%x", ipa,ipa+1,ipa+2,ipa+3,ipa+4,ipa+5)) != 6 )
            {
                printf("Invalid MAC address\n");
                return -1;
            }
            
            for (i=0; i <6; i++)
            {
                if (ipa[i] > 255)
                {
                    printf("Invalid MAC address\n");
                    return -1;
                }
                    
            }
            for (i=0;i<6;i++)
                pch[i]=ipa[i]&0xff;
        }
        break;
    }
    return 0;
}


