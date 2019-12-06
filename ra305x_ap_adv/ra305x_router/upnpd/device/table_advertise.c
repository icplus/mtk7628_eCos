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

UPNP_ADVERTISE upnp_advertise_table [] =
{       /* name                   , contrlURL                         , uuid,                                  root_device_xml,           , domain,                   type*/
	{"LANHostConfigManagement", "/control?LANHostConfigManagement", "774a7c25-ac61-424e-8968-e42c32ad6c71","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_SERVICE},
	{"LANDevice",               "",                                 "774a7c25-ac61-424e-8968-e42c32ad6c71","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_DEVICE},
	{"WANIPConnection",         "/control?WANIPConnection",         "17079cb3-4e6b-4c94-80c5-02f7ae10d9f8","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_SERVICE},
	{"WANConnectionDevice",     "",                                 "17079cb3-4e6b-4c94-80c5-02f7ae10d9f8","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_DEVICE},
	{"WANCommonInterfaceConfig","/control?WANCommonInterfaceConfig","692db16a-3120-429a-b565-caac16c76e86","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_SERVICE},
	{"WANDevice",               "",                                 "692db16a-3120-429a-b565-caac16c76e86","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_DEVICE},
	{"Layer3Forwarding",        "/control?Layer3Forwarding",        "7fff6211-b0fa-4075-a15c-0b624d01bfb4","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_SERVICE},
	{"InternetGatewayDevice",   "",                                 "7fff6211-b0fa-4075-a15c-0b624d01bfb4","InternetGatewayDevice.xml","schemas-upnp-org",        ADVERTISE_ROOTDEVICE},
#ifdef CONFIG_WPS
	{"WFAWLANConfig",           "control",                          ((char *)wps_uuidvalue)+5,             "WFADeviceDesc.xml",        "schemas-wifialliance-org",ADVERTISE_SERVICE},
	{"WFADevice",               "",                                 ((char *)wps_uuidvalue)+5,             "WFADeviceDesc.xml",        "schemas-wifialliance-org",ADVERTISE_DEVICE},
#ifdef SECOND_WIFI
	{"InternetGatewayDevice2WLAN",   "",                            "7fff6211-b0fa-7540-5ca1-0b624d01b4bf",   "InternetGatewayDevice2WLAN.xml",  "schemas-upnp-org",         ADVERTISE_ROOTDEVICE},
	{"WFAWLANConfig",              "/control?5G",                 ((char *)wps_5G_uuidvalue)+5,             "WFA5GDeviceDesc.xml",      	     "schemas-wifialliance-org", ADVERTISE_SERVICE},
	{"WFADevice",                  "",                            ((char *)wps_5G_uuidvalue)+5,             "WFA5GDeviceDesc.xml",             "schemas-wifialliance-org", ADVERTISE_DEVICE},	
#endif//SECOND_WIFI
#endif /* CONFIG_WPS */	
	{0,                         0,                                  0,                                              0}
};

