/*	$KAME: if.c,v 1.6 2005/09/16 11:30:15 suz Exp $	*/

/*
 * Copyright (C) 2002 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <net/if.h>
#include <netinet/in.h>
#ifdef __KAME__
#include <net/if_dl.h>
#endif

#include <syslog.h>
typedef unsigned int wchar_t;
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <errno.h>

#include <dhcp6.h>
#include <config.h>
#include <common.h>

extern int errno;

struct dhcp6_if *dhcp6_if_s;
struct dhcp6_if *dhcp6_if_c;
 extern void clear_iaconf(struct ia_conflist *ialist);

struct dhcp6_if *
ifinit(ifname,mode)
	char *ifname;int mode;
{
	struct dhcp6_if *ifp;
	
	struct dhcp6_if **dhcp6_if;


	if (mode ==  DHCP6_MODE_SERVER)
		dhcp6_if = &dhcp6_if_s;
	else if (mode ==  DHCP6_MODE_CLIENT)
		dhcp6_if = &dhcp6_if_c;
	
	if ((ifp = find_ifconfbyname(ifname,mode)) != NULL) {
		dprintf(LOG_NOTICE, FNAME, "duplicated interface: %s", ifname);
		return (NULL);
	}

	if ((ifp = malloc(sizeof(*ifp))) == NULL) {
		dprintf(LOG_ERR, FNAME, "malloc failed");
		goto fail;
	}
	memset(ifp, 0, sizeof(*ifp));

	TAILQ_INIT(&ifp->event_list);

	if ((ifp->ifname = strdup(ifname)) == NULL) {
		dprintf(LOG_ERR, FNAME, "failed to copy ifname");
		goto fail;
	}

	if (ifreset(ifp))
		goto fail;

	TAILQ_INIT(&ifp->reqopt_list);
	TAILQ_INIT(&ifp->iaconf_list);

	ifp->authproto = DHCP6_AUTHPROTO_UNDEF;
	ifp->authalgorithm = DHCP6_AUTHALG_UNDEF;
	ifp->authrdm = DHCP6_AUTHRDM_UNDEF;

	{
		struct ifaddrs *ifa, *ifap;
		struct sockaddr_in6 *sin6;

		if (getifaddrs(&ifap) < 0) {
			dprintf(LOG_ERR, FNAME, "getifaddrs failed: %s",
			    strerror(errno));
			goto fail;
		}

		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {

			if (strcmp(ifa->ifa_name, ifname) != 0)
				continue;
			if (ifa->ifa_addr == NULL)
				continue;
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;

			sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
			dprintf(LOG_DEBUG, FNAME, "sin6->sin6_addr: %s\n",in6addr2str(&sin6->sin6_addr, 0));
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				continue;

			ifp->addr = sin6->sin6_addr;
		}

		freeifaddrs(ifap);
	}
		ifp->next = *dhcp6_if;
		*dhcp6_if = ifp;
	return (ifp);

  fail:
	if (ifp->ifname != NULL)
		free(ifp->ifname);
	free(ifp);
	return (NULL);
}

int
ifreset(ifp)
	struct dhcp6_if *ifp;
{
	unsigned int ifid;
	u_int32_t linkid;

	if ((ifid = if_nametoindex(ifp->ifname)) == 0) {
		dprintf(LOG_ERR, FNAME, "invalid interface(%s): %s",
			ifp->ifname, strerror(errno));
		return (-1);
	}

#ifdef HAVE_SCOPELIB
	if (inet_zoneid(AF_INET6, 2, ifname, &linkid)) {
		dprintf(LOG_ERR, FNAME, "failed to get link ID for %s",
		    ifname);
		return (-1);
	}
#else
	linkid = ifid;		/* XXX: assume 1to1 mapping IFs and links */
#endif

	ifp->ifid = ifid;
	ifp->linkid = linkid;

	return (0);
}

struct dhcp6_if *
find_ifconfbyname(ifname,mode)
	char *ifname;int mode;
{
	struct dhcp6_if *ifp;
	struct dhcp6_if *dhcp6_if;


	if (mode ==  DHCP6_MODE_SERVER)
		dhcp6_if = dhcp6_if_s;
	else if (mode ==  DHCP6_MODE_CLIENT)
		dhcp6_if = dhcp6_if_c;
	for (ifp = dhcp6_if; ifp; ifp = ifp->next) {
		if (strcmp(ifp->ifname, ifname) == 0)
			return (ifp);
	}

	return (NULL);
}

struct dhcp6_if *
find_ifconfbyid(id,mode)
	unsigned int id;int mode;
{
	struct dhcp6_if *ifp;
	struct dhcp6_if *dhcp6_if;


	if (mode ==  DHCP6_MODE_SERVER)
		dhcp6_if = dhcp6_if_s;
	else if (mode ==  DHCP6_MODE_CLIENT)
		dhcp6_if = dhcp6_if_c;

	for (ifp = dhcp6_if; ifp; ifp = ifp->next) {
		if (ifp->ifid == id)
			return (ifp);
	}

	return (NULL);
}

void ifclear(struct dhcp6_if *ifp)
{
	dprintf(LOG_INFO,FNAME,"Entry ====>");
	
	free(ifp->ifname);
	dhcp6_clear_list(&ifp->reqopt_list);
	clear_iaconf(&ifp->iaconf_list);
	if (ifp->scriptpath != NULL)
		free(ifp->scriptpath);
	free(ifp);

}
