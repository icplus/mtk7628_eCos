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
    xml_InternetGatewayDevice.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/
#include <porting.h>
#include <upnp.h>
#include <stdio.h>
#include <string.h>

/*
static char xml_template[] =
(
	"<?xml version=\"1.0\"?>\r\n"
	"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
	"\t<specVersion>\r\n"
	"\t\t<major>1</major>\r\n"
	"\t\t<minor>0</minor>\r\n"
	"\t</specVersion>\r\n"
	"\t<device>\r\n"
	"\t\t<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>\r\n"
	"\t\t<friendlyName>MTKRouter</friendlyName>\r\n"
	"\t\t<manufacturer>Mediatek Technology, Corp.</manufacturer>\r\n"
	"\t\t<manufacturerURL>http://www.mediatek.com/</manufacturerURL>\r\n"
	"\t\t<modelDescription>Mediatek Home Gateway</modelDescription>\r\n"
	"\t\t<modelName>MTKRouter</modelName>\r\n"
	"\t\t<modelURL>http://www.mediatek.com/</modelURL>\r\n"
	"\t\t<UDN>uuid:7fff6211-b0fa-4075-a15c-0b624d01bfb4</UDN>\r\n"
	"\t\t<serviceList>\r\n"
	"\t\t\t<service>\r\n"
	"\t\t\t\t<serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>\r\n"
	"\t\t\t\t<serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>\r\n"
	"\t\t\t\t<SCPDURL>/x_layer3forwarding.xml</SCPDURL>\r\n"
	"\t\t\t\t<controlURL>/control?Layer3Forwarding</controlURL>\r\n"
	"\t\t\t\t<eventSubURL>/event?Layer3Forwarding</eventSubURL>\r\n"
	"\t\t\t</service>\r\n"
	"\t\t</serviceList>\r\n"
	"\t\t<deviceList>\r\n"
	"\t\t\t<device>\r\n"
	"\t\t\t\t<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\r\n"
	"\t\t\t\t<friendlyName>WANDevice</friendlyName>\r\n"
	"\t\t\t\t<manufacturer>Mediatek Technology, Corp.</manufacturer>\r\n"
	"\t\t\t\t<manufacturerURL>http://www.mediatek.com/</manufacturerURL>\r\n"
	"\t\t\t\t<modelDescription>Mediatek Home Gateway</modelDescription>\r\n"
	"\t\t\t\t<modelName>MTKRouter</modelName>\r\n"
	"\t\t\t\t<modelURL>http://www.mediatek.com/</modelURL>\r\n"
	"\t\t\t\t<UDN>uuid:692db16a-3120-429a-b565-caac16c76e86</UDN>\r\n"
	"\t\t\t\t<serviceList>\r\n"
	"\t\t\t\t\t<service>\r\n"
	"\t\t\t\t\t\t<serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>\r\n"
	"\t\t\t\t\t\t<serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>\r\n"
	"\t\t\t\t\t\t<SCPDURL>/x_wancommoninterfaceconfig.xml</SCPDURL>\r\n"
	"\t\t\t\t\t\t<controlURL>/control?WANCommonInterfaceConfig</controlURL>\r\n"
	"\t\t\t\t\t\t<eventSubURL>/event?WANCommonInterfaceConfig</eventSubURL>\r\n"
	"\t\t\t\t\t</service>\r\n"
	"\t\t\t\t</serviceList>\r\n"
	"\t\t\t\t<deviceList>\r\n"
	"\t\t\t\t\t<device>\r\n"
	"\t\t\t\t\t\t<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>\r\n"
	"\t\t\t\t\t\t<friendlyName>WAN Connection Device</friendlyName>\r\n"
	"\t\t\t\t\t\t<manufacturer>Mediatek Technology, Corp.</manufacturer>\r\n"
	"\t\t\t\t\t\t<manufacturerURL>http://www.mediatek.com/</manufacturerURL>\r\n"
	"\t\t\t\t\t\t<modelDescription>Mediatek Home Gateway</modelDescription>\r\n"
	"\t\t\t\t\t\t<modelName>MTKRouter</modelName>\r\n"
	"\t\t\t\t\t\t<modelURL>http://www.mediatek.com/</modelURL>\r\n"
	"\t\t\t\t\t\t<UDN>uuid:17079cb3-4e6b-4c94-80c5-02f7ae10d9f8</UDN>\r\n"
	"\t\t\t\t\t\t<serviceList>\r\n"
	"\t\t\t\t\t\t\t<service>\r\n"
	"\t\t\t\t\t\t\t\t<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>\r\n"
	"\t\t\t\t\t\t\t\t<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>\r\n"
	"\t\t\t\t\t\t\t\t<SCPDURL>/x_wanipconnection.xml</SCPDURL>\r\n"
	"\t\t\t\t\t\t\t\t<controlURL>/control?WANIPConnection</controlURL>\r\n"
	"\t\t\t\t\t\t\t\t<eventSubURL>/event?WANIPConnection</eventSubURL>\r\n"
	"\t\t\t\t\t\t\t</service>\r\n"
	"\t\t\t\t\t\t</serviceList>\r\n"
	"\t\t\t\t\t</device>\r\n"
	"\t\t\t\t</deviceList>\r\n"
	"\t\t\t</device>\r\n"

	"\t\t\t<device>\r\n"
	"\t\t\t\t<deviceType>urn:schemas-upnp-org:device:LANDevice:1</deviceType>\r\n"
	"\t\t\t\t<friendlyName>LANDevice</friendlyName>\r\n"
	"\t\t\t\t<manufacturer>Mediatek Technology, Corp.</manufacturer>\r\n"
	"\t\t\t\t<manufacturerURL>http://www.mediatek.com/</manufacturerURL>\r\n"
	"\t\t\t\t<modelDescription>Mediatek Home Gateway</modelDescription>\r\n"
	"\t\t\t\t<modelName>MTKRouter</modelName>\r\n"
	"\t\t\t\t<modelURL>http://www.mediatek.com/</modelURL>\r\n"
	"\t\t\t\t<UDN>uuid:774a7c25-ac61-424e-8968-e42c32ad6c71</UDN>\r\n"
	"\t\t\t\t<serviceList>\r\n"
	"\t\t\t\t\t<service>\r\n"
	"\t\t\t\t\t\t<serviceType>urn:schemas-upnp-org:service:LANHostConfigManagement:1</serviceType>\r\n"
	"\t\t\t\t\t\t<serviceId>urn:upnp-org:serviceId:LANHostCfg1</serviceId>\r\n"
	"\t\t\t\t\t\t<SCPDURL>/x_lanhostconfigmanagement.xml</SCPDURL>\r\n"
	"\t\t\t\t\t\t<controlURL>/control?LANHostConfigManagement</controlURL>\r\n"
	"\t\t\t\t\t\t<eventSubURL>/event?LANHostConfigManagement</eventSubURL>\r\n"
	"\t\t\t\t\t</service>\r\n"
	"\t\t\t\t</serviceList>\r\n"
	"\t\t\t</device>\r\n"
#ifdef CONFIG_WPS
        "<device>\r\n"
        "<deviceType>urn:schemas-wifialliance-org:device:WFADevice:1</deviceType>\r\n"
        "<friendlyName>MTKAP</friendlyName>\r\n"
        "<manufacturer>Mediatek Technology, Corp.</manufacturer>\r\n"
        "<manufacturerURL>http://www.mediatek.com/</manufacturerURL>\r\n"
        "<modelDescription>Mediatek AP WSC device</modelDescription>\r\n"
        "<modelName>Mediatek Wireless Access Point</modelName>\r\n"
        "<modelNumber>MTKAP</modelNumber>\r\n"
        "<modelURL>http://www.mediatek.com/</modelURL>\r\n"
        "<serialNumber>12345678</serialNumber>\r\n"
#endif
);*/

//#ifdef SECOND_WIFI


static char xml_template2[] =
(
#ifdef CONFIG_WPS
        "<serviceList>\r\n"
        "<service>\r\n"
        "<serviceType>urn:schemas-wifialliance-org:service:WFAWLANConfig:1</serviceType>\r\n"
        "<serviceId>urn:wifialliance-org:serviceId:WFAWLANConfig1</serviceId>\r\n"
        "<SCPDURL>WFAWLANConfigSCPD.xml</SCPDURL>\r\n"
        "<controlURL>control</controlURL>\r\n"
        "<eventSubURL>event</eventSubURL>\r\n"
        "</service>\r\n"
        "</serviceList>\r\n"
        "</device>\r\n"
#endif
	"\t\t</deviceList>\r\n"
);

//#endif//SECOND_WIFI

static void PresentURL(UPNP_CONTEXT *context, char *buf)
{
	char lanIP[16];
	
	CFG_get_str(CFG_LAN_IP, lanIP) ; // get LAN_IPADDR dynamically            
	
	sprintf(buf, "http://%s", lanIP);
}
//#ifdef SECOND_WIFI	

int xml_InternetGatewayDevice(UPNP_CONTEXT * context)
{
    int len;        	
	char url[128];
	char buf[255];
	char* pInternetGatewayDevice=NULL;
	int result=0;

	result = description_readXML(XML_INDEX_InternetGatewayDevice,&pInternetGatewayDevice,&len);
	
	if(pInternetGatewayDevice==NULL || result==0) return 1;
	
    CONTEXT_SEND(pInternetGatewayDevice, len);
	
	if(pInternetGatewayDevice!=NULL) 
	{	
		free(pInternetGatewayDevice);
		pInternetGatewayDevice=NULL;
	}

#ifdef CONFIG_WPS
        len = sprintf(buf,"<UDN>%s</UDN>\r\n", wps_uuidvalue);
        CONTEXT_SEND(buf, len);
#endif
        CONTEXT_SEND(xml_template2, sizeof(xml_template2) - 1);

	PresentURL(context, url);
	len = sprintf(buf,"\t\t<presentationURL>%s</presentationURL>\r\n", url);
						
	CONTEXT_SEND(buf, len);

        CONTEXT_SEND("\t</device>\r\n</root>\r\n\r\n", 23);

    return 0;
}

//#endif//SECOND_WIFI

