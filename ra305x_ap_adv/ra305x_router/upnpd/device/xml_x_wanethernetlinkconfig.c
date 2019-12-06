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


#include <porting.h>
#include <upnp.h>

int xml_x_wanethernetlinkconfig
(
    HTTP_CONTEXT * context
)
{
    description_send(
    context,
	"<?xml version=\"1.0\"?>\r\n"
	"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\r\n"
	"\t<specVersion>\r\n"
	"\t\t<major>1</major>\r\n"
	"\t\t<minor>0</minor>\r\n"
	"\t</specVersion>\r\n"
	"\t<actionList>\r\n"
	"\t\t<action>\r\n"
	"\t\t\t<name>GetEthernetLinkStatus</name>\r\n"
	"\t\t\t\t<argumentList>\r\n"
	"\t\t\t\t\t<argument>\r\n"
	"\t\t\t\t\t\t<name>NewEthernetLinkStatus</name>\r\n"
	"\t\t\t\t\t\t<direction>out</direction>\r\n"
	"\t\t\t\t\t\t<relatedStateVariable>EthernetLinkStatus</relatedStateVariable>\r\n"
	"\t\t\t\t\t</argument>\r\n"
	"\t\t\t\t</argumentList>\r\n"
	"\t\t</action>\r\n"
	"\t</actionList>\r\n"
	"\t<serviceStateTable>\r\n"
	"\t\t<stateVariable sendEvents=\"yes\">\r\n"
	"\t\t\t<name>EthernetLinkStatus</name>\r\n"
	"\t\t\t<dataType>string</dataType>\r\n"
	"\t\t\t<allowedValueList>\r\n"
	"\t\t\t\t<allowedValue>Up</allowedValue>\r\n"
	"\t\t\t\t<allowedValue>Down</allowedValue>\r\n"
	"\t\t\t\t<allowedValue>Unavailable</allowedValue>\r\n"
	"\t\t\t</allowedValueList>\r\n"
	"\t\t</stateVariable>\r\n"
	"\t</serviceStateTable>\r\n"
	"</scpd>\r\n"
	"\r\n"
    );

    return 0;
}


