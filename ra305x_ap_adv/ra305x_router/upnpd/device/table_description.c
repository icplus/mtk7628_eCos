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


//<< TABLE BEGIN
/***************************************************
  WARNNING: DON'T MODIFY THE FOLLOWING TABLES
  AND DON'T REMOVE TAG :
           "<< TABLE BEGIN"
           ">> TABLE END"
*****************************************************/
extern int xml_InternetGatewayDevice(UPNP_CONTEXT *context);
extern int xml_x_lanhostconfigmanagement(UPNP_CONTEXT *context);
extern int xml_x_layer3forwarding(UPNP_CONTEXT *context);
extern int xml_x_wancommoninterfaceconfig(UPNP_CONTEXT *context);
extern int xml_x_wanipconnection(UPNP_CONTEXT *context);
#ifdef CONFIG_WPS
extern int xml_WFADeviceDesc(UPNP_CONTEXT *context);
extern int xml_x_wfawlanconfigscpd(UPNP_CONTEXT *context);
#ifdef SECOND_WIFI
extern int xml_InternetGatewayDevice2WLAN(UPNP_CONTEXT *context);
extern int xml_WFA5GDeviceDesc(UPNP_CONTEXT *context);
extern int xml_x_wfa5Gwlanconfigscpd(UPNP_CONTEXT *context);
#endif
#endif /* CONFIG_WPS */

UPNP_DESCRIPTION upnp_description_table [] =
{
	{"/InternetGatewayDevice.xml",              xml_InternetGatewayDevice},
	{"/x_lanhostconfigmanagement.xml",          xml_x_lanhostconfigmanagement},
	{"/x_layer3forwarding.xml",                 xml_x_layer3forwarding},
	{"/x_wancommoninterfaceconfig.xml",         xml_x_wancommoninterfaceconfig},
	{"/x_wanipconnection.xml",                  xml_x_wanipconnection},
#ifdef CONFIG_WPS
	{"/WFADeviceDesc.xml",                      xml_WFADeviceDesc},
	{"/WFAWLANConfigSCPD.xml",	            	xml_x_wfawlanconfigscpd},
#ifdef SECOND_WIFI	
	{"/InternetGatewayDevice2WLAN.xml",			xml_InternetGatewayDevice2WLAN},
	{"/WFA5GDeviceDesc.xml",                    xml_WFA5GDeviceDesc},
	{"/WFA5GWLANConfigSCPD.xml",	            xml_x_wfa5Gwlanconfigscpd},	
#endif	
#endif /* CONFIG_WPS */
	{0,                                         0}
};
//>> TABLE END

