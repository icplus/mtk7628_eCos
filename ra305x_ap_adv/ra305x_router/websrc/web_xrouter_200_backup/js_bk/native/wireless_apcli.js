/*REAL
<%
CGI_MAP(apcli_ssid, CFG_WLN_ApCliSsid);
CGI_MAP(apcli_bssid, CFG_WLN_ApCliBssid);
CGI_MAP(apcli_enable, CFG_WLN_ApCliEnable);
CGI_MAP(apcli_channel, CFG_WLN_Channel);
CGI_MAP(apcli_autoconnect, CFG_WLN_ApCliAutoConnect);
CGI_MAP(apcli_wep_key_method, CFG_WLN_ApCliKey1Type);
CGI_MAP(ApCliKey2Type2, CFG_WLN_ApCliKey2Type);
CGI_MAP(ApCliKey3Type2, CFG_WLN_ApCliKey3Type);
CGI_MAP(ApCliKey4Type2, CFG_WLN_ApCliKey4Type);
CGI_MAP(apcli_wep_key_1, CFG_WLN_ApCliKey1Str);
CGI_MAP(apcli_wep_key_2, CFG_WLN_ApCliKey2Str);
CGI_MAP(apcli_wep_key_3, CFG_WLN_ApCliKey3Str);
CGI_MAP(apcli_wep_key_4, CFG_WLN_ApCliKey4Str);
CGI_MAP(apcli_passphrase, CFG_WLN_ApCliWPAPSK);
CGI_MAP(apcliencryptype2, CFG_WLN_ApCliEncrypType);
CGI_MAP(apcliauth2, CFG_WLN_ApCliAuthMode);
CGI_MAP(ApCliDefaultKeyID2, CFG_WLN_ApCliDefaultKeyID);
CGI_MAP(opmode, CFG_SYS_OPMODE);
%>
REAL*/

var apclissid = "<% CGI_CFG_GET(CFG_WLN_ApCliSsid); %>";
var apclienable = "<% CGI_CFG_GET(CFG_WLN_ApCliEnable); %>"; 
var apclibssid = "<% CGI_CFG_GET(CFG_WLN_ApCliBssid); %>"; 
var apclichn_index  = "<% CGI_WLN_Channel();%>";
var apcliautoconnect  = "<% CGI_CFG_GET(CFG_WLN_ApCliAutoConnect); %>";
var apcliauth = "<% CGI_CFG_GET(CFG_WLN_ApCliAuthMode); %>"; 
var apcliencryptype = "<% CGI_CFG_GET(CFG_WLN_ApCliEncrypType); %>";
var apcliwepkeytype = "<% CGI_CFG_GET(CFG_WLN_ApCliKey1Type); %>";
var apcliwepkey1 = "<% CGI_CFG_GET(CFG_WLN_ApCliKey1Str); %>"; 
var apcliwepkey2 = "<% CGI_CFG_GET(CFG_WLN_ApCliKey2Str); %>"; 
var apcliwepkey3 = "<% CGI_CFG_GET(CFG_WLN_ApCliKey3Str); %>"; 
var apcliwepkey4 = "<% CGI_CFG_GET(CFG_WLN_ApCliKey4Str); %>"; 
var apcliwepdefaultKey = "<% CGI_CFG_GET(CFG_WLN_ApCliDefaultKeyID); %>"; 
var apcliwpakey = "<% CGI_CFG_GET(CFG_WLN_ApCliWPAPSK); %>";
