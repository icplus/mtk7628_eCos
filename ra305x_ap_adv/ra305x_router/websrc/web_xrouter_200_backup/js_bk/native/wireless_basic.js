/*REAL
<%
CGI_MAP(WirelessMode2, CFG_WLN_WirelessMode);
CGI_MAP(BasicRate2, CFG_WLN_BasicRate);
CGI_MAP(ssid, CFG_WLN_SSID1);
CGI_MAP(mssid_1, CFG_WLN_SSID2);
CGI_MAP(mssid_2, CFG_WLN_SSID3);
CGI_MAP(mssid_3, CFG_WLN_SSID4);
CGI_MAP(SSID3, CFG_WLN_WscSSID);
CGI_MAP(bssid_num, CFG_WLN_BssidNum);
CGI_MAP(HideSSID2, CFG_WLN_HideSSID);
CGI_MAP(AutoChannelSelect2, CFG_WLN_AutoChannelSelect);
CGI_MAP(sz11aChannel, CFG_WLN_Channel);
CGI_MAP(sz11bChannel, CFG_WLN_Channel);
CGI_MAP(sz11gChannel, CFG_WLN_Channel);
CGI_MAP(WdsEnable3, CFG_WLN_WdsEnable);
CGI_MAP(EncrypType2, CFG_WLN_WdsEncrypType);
CGI_MAP(WdsDefaultKey2, CFG_WLN_WdsDefaultKeyID);
CGI_MAP(Wds1KeyTemp, CFG_WLN_Wds1Key);
CGI_MAP(Wds2KeyTemp, CFG_WLN_Wds2Key);
CGI_MAP(Wds3KeyTemp, CFG_WLN_Wds3Key);
CGI_MAP(Wds4KeyTemp, CFG_WLN_Wds4Key);
CGI_MAP(n_mode, CFG_WLN_HT_OpMode);
CGI_MAP(n_bandwidth, CFG_WLN_HT_BW);
CGI_MAP(n_gi, CFG_WLN_HT_GI);
CGI_MAP(n_mcs, CFG_WLN_HT_MCS);
CGI_MAP(n_rdg, CFG_WLN_HT_RDG);
CGI_MAP(EXTCHA2, CFG_WLN_HT_EXTCHA);
CGI_MAP(n_amsdu, CFG_WLN_HT_AMSDU);
CGI_MAP(n_dis_tkip, CFG_WLN_HT_DisallowTKIP);
CGI_MAP(f_40mhz, CFG_WLN_HT_40MHZ_INTOLERANT);
CGI_MAP(wifi_opt, CFG_WLN_WiFiTest);
CGI_MAP(rx_stream, CFG_WLN_HT_RxStream);
CGI_MAP(tx_stream, CFG_WLN_HT_TxStream);	
CGI_MAP(n_stbc, CFG_WLN_HT_STBC);
CGI_MAP(opmode, CFG_SYS_OPMODE);
CGI_MAP(wifi_rdg, CFG_WLN_RadioOn);
%>
REAL*/

var ModuleName = "<% CGI_CFG_GET(CFG_WLN_ModuleName); %>"; 
var PhyMode = "<% CGI_CFG_GET(CFG_WLN_WirelessMode); %>"; 
var ssid_num = "<% CGI_CFG_GET(CFG_WLN_BssidNum); %>"; 
var broadcastssidEnable = "<% CGI_CFG_GET(CFG_WLN_HideSSID); %>";
var channel_index  = "<% CGI_WLN_Channel();%>"; 
var wdsMode = "<% CGI_CFG_GET(CFG_WLN_WdsEnable); %>"; 
var wdsPhyMode = "<% CGI_CFG_GET(CFG_WLN_WdsPhyMode); %>"; 
var wdsEncrypType = "<% CGI_CFG_GET(CFG_WLN_WdsEncrypType); %>";
var wdsDefaultKey = "<% CGI_CFG_GET(CFG_WLN_WdsDefaultKeyID); %>"; 
var wdsEncrypKey = "<% CGI_CFG_GET(CFG_WLN_Wds1Key); %>";
var countrycode = "<% CGI_CFG_GET(CFG_WLN_CountryCode); %>";
var ht_mode = "<% CGI_CFG_GET(CFG_WLN_HT_OpMode); %>"; 
var ht_bw = "<% CGI_CFG_GET(CFG_WLN_HT_BW); %>"; 
var ht_gi = "<% CGI_CFG_GET(CFG_WLN_HT_GI); %>"; 
var ht_stbc = "<% CGI_CFG_GET(CFG_WLN_HT_STBC); %>"; 
var ht_mcs = "<% CGI_CFG_GET(CFG_WLN_HT_MCS); %>"; 	
var ht_htc = "<% CGI_CFG_GET(CFG_WLN_HT_HTC); %>"; 
var ht_rdg = "<% CGI_CFG_GET(CFG_WLN_HT_RDG); %>"; 
var ht_linkadapt = "<% CGI_CFG_GET(CFG_WLN_HT_LinkAdapt); %>"; 
var ht_extcha  = "<% CGI_CFG_GET(CFG_WLN_HT_EXTCHA); %>"; 
var ht_amsdu = "<% CGI_CFG_GET(CFG_WLN_HT_AMSDU); %>"; 
var ht_distkip = "<% CGI_CFG_GET(CFG_WLN_HT_DisallowTKIP); %>";
var ht_f_40mhz = "<% CGI_CFG_GET(CFG_WLN_HT_40MHZ_INTOLERANT); %>"; 
var wifi_optimum = "<% CGI_CFG_GET(CFG_WLN_WiFiTest); %>";
var rx_stream_idx = "<% CGI_CFG_GET(CFG_WLN_HT_RxStream); %>"; 
var tx_stream_idx = "<% CGI_CFG_GET(CFG_WLN_HT_TxStream); %>"; 
var all_tkip_wep = "<% CGI_GET_WLN_ALL_BSS_TKIP_OR_WEP();%>";
var ssid_s1 = "<% CGI_CFG_GET(CFG_WLN_SSID1); %>";
var ssid_s2 = "<% CGI_CFG_GET(CFG_WLN_SSID2); %>";
var ssid_s3 = "<% CGI_CFG_GET(CFG_WLN_SSID3); %>";
var ssid_s4 = "<% CGI_CFG_GET(CFG_WLN_SSID4); %>";
var basicRateTemp = "<% CGI_CFG_GET(CFG_WLN_BasicRate); %>";
var wsc_mode = "<% CGI_CFG_GET(CFG_WLN_WscConfMode); %>";
var wln_apclienable = "<% CGI_CFG_GET(CFG_WLN_RadioOn); %>";
var wln_authMode = "<% CGI_CFG_GET(CFG_WLN_AuthMode); %>";
var wln_passwd = "<% CGI_CFG_GET(CFG_WLN_WPAPSK1); %>"

