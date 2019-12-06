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

extern UPNP_ACTION action_x_lanhostconfigmanagement[];
extern UPNP_ACTION action_x_layer3forwarding[];
extern UPNP_ACTION action_x_wancommoninterfaceconfig[];
extern UPNP_ACTION action_x_wanipconnection[];
#ifdef CONFIG_WPS
extern UPNP_ACTION action_x_wfawlanconfig[];
#endif /* CONFIG_WPS */

extern UPNP_STATE_VAR statevar_x_lanhostconfigmanagement[];
extern UPNP_STATE_VAR statevar_x_layer3forwarding[];
extern UPNP_STATE_VAR statevar_x_wancommoninterfaceconfig[];
extern UPNP_STATE_VAR statevar_x_wanipconnection[];
#ifdef CONFIG_WPS
extern UPNP_STATE_VAR statevar_x_wfawlanconfig[];
#endif /* CONFIG_WPS */


UPNP_SERVICE upnp_service_table [] =
{
	{"/control?LANHostConfigManagement", "/event?LANHostConfigManagement", "774a7c25-ac61-424e-8968-e42c32ad6c71","LANHostConfigManagement", "LANHostCfg1",   action_x_lanhostconfigmanagement, statevar_x_lanhostconfigmanagement},
	{"/control?Layer3Forwarding",        "/event?Layer3Forwarding",        "7fff6211-b0fa-4075-a15c-0b624d01bfb4","Layer3Forwarding",        "L3Forwarding1", action_x_layer3forwarding,        statevar_x_layer3forwarding},
	{"/control?WANCommonInterfaceConfig","/event?WANCommonInterfaceConfig","692db16a-3120-429a-b565-caac16c76e86","WANCommonInterfaceConfig","WANCommonIFC1", action_x_wancommoninterfaceconfig,statevar_x_wancommoninterfaceconfig},
	{"/control?WANIPConnection",         "/event?WANIPConnection",         "17079cb3-4e6b-4c94-80c5-02f7ae10d9f8","WANIPConnection",         "WANIPConn1",    action_x_wanipconnection,         statevar_x_wanipconnection},
#ifdef CONFIG_WPS
	{"/control",                         "/event",                         wps_uuidvalue+5,                       "WFAWLANConfig",           "WFAWLANConfig1",  action_x_wfawlanconfig,           statevar_x_wfawlanconfig},
#ifdef SECOND_WIFI	
	{"/control?5G",                      "/event?5G",                      wps_5G_uuidvalue+5,                       "WFAWLANConfig",         "WFAWLANConfig1",action_x_wfawlanconfig,           statevar_x_wfawlanconfig},	
#endif
#endif /* CONFIG_WPS */
	{0,                                  0,                                0,                                     0,                         0,                                                 0}
};
//>> TABLE END

