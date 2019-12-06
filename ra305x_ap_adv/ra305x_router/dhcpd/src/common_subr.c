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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <netinet/ip_var.h>
#include <dhcpd.h>
#include <common_subr.h>
#include <leases.h>
#include <packet.h>


// supported options are easily added here
struct dhcps_option offer_options[] =
{
    {DHCP_SUBNET,       4},
    {DHCP_ROUTER,       4},
    {DHCP_DNS_SERVER,   4},
    {DHCP_DOMAIN_NAME,  0},
    {0, 0}
};

void dhcpd_attach_embed_option(unsigned char code, void *data)
{
    struct dhcps_option *opt;
    unsigned char len;
    unsigned char *ptr;

    for (opt=offer_options; opt->code; opt++)
    {
        if (opt->code == code)
        {
            if (opt->option)
            {
                len = opt->option[OPT_LEN] + 2;
                ptr = opt->option = (char *)realloc(opt->option, len + opt->len);
                if (ptr != 0)
                {
                    memcpy(&ptr[len], data, opt->len);
                    ptr[OPT_LEN] += opt->len;
                }
            }
            else
            {
                len = opt->len;
                if (len == 0)
                    len = strlen((char *)data);

                ptr = opt->option = malloc(len + 2);
                if (ptr)
                {
                    ptr[OPT_CODE] = code;
                    ptr[OPT_LEN] = len;
                    memcpy(&ptr[OPT_DATA], data, len);
                }
            }

            return;
        }
    }
}

void dhcpd_flush_embed_options(void)
{
    int i;

    for (i=0; offer_options[i].code; i++)
    {
        if (offer_options[i].option)
        {
            free(offer_options[i].option);
            offer_options[i].option = 0;
        }
    }
}

int dhcpd_add_option(unsigned char *ptr, unsigned char code, unsigned char len, void *data)
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

void dhcpd_add_offsers(unsigned char *ptr)
{
    int i;
    int len;

    // Search DHCP_END
    while (*ptr != DHCP_END)
    {
        if (*ptr == DHCP_PADDING)
            ptr++;
        else
            ptr += ptr[OPT_LEN] + 2;
    }

    // Copy embed offer to options
    for (i=0; offer_options[i].code; i++)
    {
        if (offer_options[i].option)
        {
            len = offer_options[i].option[OPT_LEN];

            memcpy(ptr, offer_options[i].option, len+2);
            ptr += (len+2);
        }
    }

    *ptr = DHCP_END;
    return;
}

unsigned char *dhcpd_pickup_opt(struct dhcpd *packet, int code, int dest_len, void *dest)
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
                if (curr == OPTION_FIELD && over & FILE_FIELD)
                {
                    ptr = packet->file;
                    i = 0;
                    length = 128;
                    curr = FILE_FIELD;
                }
                else if (curr == FILE_FIELD && over & SNAME_FIELD)
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
        }
    }

    return 0;
}

