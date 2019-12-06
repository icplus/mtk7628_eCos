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

#include <unistd.h>
#include <network.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dhcpc.h>
#include <options.h>
#include <eventlog.h>

#define DHCPD_DEBUG
#define DHCPD_DIAG(c...)    SysLog(SYS_LOG_INFO|LOGM_DHCPD, ##c)
#define DHCPD_LOG(c...)     SysLog(LOG_USER|SYS_LOG_INFO|LOGM_DHCPD, ##c)


/* get an option with bounds checking (warning, not aligned). */
unsigned char *dhcpc_get_option(struct dhcpc *packet, int code, int dest_len, void *dest)
{
    int i, length;
    unsigned char *ptr;
    int over = 0, done = 0, curr = OPTION_FIELD;
    unsigned char len;

    ptr = packet->options;
    i = 0;
    length = 308;
    while (!done)
    {
        if (i >= length)
        {
            DHCPD_DIAG( "Option fields too long.");
            return 0;
        }

        if (ptr[i + OPT_CODE] == code)
        {
            if (i + 1 + ptr[i + OPT_LEN] >= length)
                return 0;

            if (dest)
            {
                len = ptr[i + OPT_LEN];
                if (len > dest_len)
                {
                    DHCPD_DIAG( "Option fields too long to fit in dest.");
                    return 0;
                }

                memcpy(dest, &ptr[i + OPT_DATA], (int)len);
            }

            return &ptr[i + OPT_DATA];
        }

        switch (ptr[i + OPT_CODE])
        {
            case DHCP_PADDING:
                i++;
                break;

            case DHCP_OPTION_OVER:
                if (i + 1 + ptr[i + OPT_LEN] >= length)
                    return 0;

                over = ptr[i + 3];
                i += ptr[OPT_LEN] + 2;
                break;

            case DHCP_END:
                if (curr == OPTION_FIELD && (over & FILE_FIELD))
                {
                    ptr = packet->file;
                    i = 0;
                    length = 128;
                    curr = FILE_FIELD;
                }
                else if (curr == FILE_FIELD && (over & SNAME_FIELD))
                {
                    ptr = packet->sname;
                    i = 0;
                    length = 64;
                    curr = SNAME_FIELD;
                }
                else
                    done = 1;
                break;

            default:
                i += ptr[OPT_LEN + i] + 2;
                break;
        }
    }

    return 0;
}


int dhcpc_add_option(unsigned char *ptr, unsigned char code, unsigned char len, void *data)
{
    int end;

    // Search DHCP_END
    end = 0;
    while (ptr[end] != DHCP_END)
    {
        if (ptr[end] == DHCP_PADDING)
            end++;
        else
            end += ptr[end + OPT_LEN] + 2;
    }

    if (end + len + 2 + 1 >= 308)
    {
        DHCPD_DIAG( "Option 0x%02x cannot not fit into the packet!", code);
        return 0;
    }

    ptr += end;

    ptr[OPT_CODE] = code;
    ptr[OPT_LEN] = len;
    memcpy(&ptr[OPT_DATA], data, len);

    // Reassign DHCP_END
    ptr += (len+2);
    *ptr = DHCP_END;
    return (len + 2);
}

