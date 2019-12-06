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

/* #USERCODE */
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_WPS
#ifdef SECOND_WIFI
/*static char xml_template1[] =
(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
        "<specVersion>\r\n"
        "<major>1</major>\r\n"
        "<minor>0</minor>\r\n"
        "</specVersion>\r\n"
        "<device>\r\n"
        "<deviceType>urn:schemas-wifialliance-org:device:WFA5GDevice:1</deviceType>\r\n"
        "<friendlyName>RalinkAP5G</friendlyName>\r\n"
        "<manufacturer>Ralink Technology, Corp.</manufacturer>\r\n"
        "<manufacturerURL>http://www.ralinktech.com.tw</manufacturerURL>\r\n"
        "<modelDescription>Ralink AP 5G WSC device</modelDescription>\r\n"
        "<modelName>Ralink Wireless 5G Access Point</modelName>\r\n"
        "<modelNumber>RalinkAP5G</modelNumber>\r\n"
        "<modelURL>http://www.ralinktech.com.tw</modelURL>\r\n"
        "<serialNumber>87654321</serialNumber>\r\n"
);*/

static char xml_template2[] =
(
        "<serviceList>\r\n"
        "<service>\r\n"
        "<serviceType>urn:schemas-wifialliance-org:service:WFAWLANConfig:1</serviceType>\r\n"
        "<serviceId>urn:wifialliance-org:serviceId:WFAWLANConfig1</serviceId>\r\n"
        "<SCPDURL>WFA5GWLANConfigSCPD.xml</SCPDURL>\r\n"
        "<controlURL>/control?5G</controlURL>\r\n"
        "<eventSubURL>/event?5G</eventSubURL>\r\n"
        "</service>\r\n"
        "</serviceList>\r\n"
        "</device>\r\n"
        "</root>\r\n"
        "\r\n"
);

int xml_WFA5GDeviceDesc(UPNP_CONTEXT * context)
{
	int len;			
	char url[128];
	char buf[255];			
	
	char* pInternetGatewayDevice=NULL;
	int result=0;
	
	result = description_readXML(XML_INDEX_x_wfadevicedesc,&pInternetGatewayDevice,&len);
	
	if(pInternetGatewayDevice==NULL || result==0) return 1;
	
	CONTEXT_SEND(pInternetGatewayDevice, len);
	
	if(pInternetGatewayDevice!=NULL) 
	{	
		free(pInternetGatewayDevice);
		pInternetGatewayDevice=NULL;
	}

                       
        len = sprintf(buf,"<UDN>%s</UDN>\r\n", wps_5G_uuidvalue);
        CONTEXT_SEND(buf, len);

        CONTEXT_SEND(xml_template2, sizeof(xml_template2) - 1);

    return 0;
}
#endif
#endif /* CONFIG_WPS */

