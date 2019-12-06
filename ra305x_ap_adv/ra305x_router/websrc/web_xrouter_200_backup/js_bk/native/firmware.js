// JavaScript Document

/*DEMO*/
	addCfg("opmode",100,"3");
    FirwareVerion="v1.00(C)";
    FirwareDate=dateStrC(1103895780+28800);
/*END_DEMO*/
/*REAL
    FirwareVerion="v<%CGI_CUST_VER_STR();%>";
    FirwareDate=dateStrC(<%CGI_FMW_BUILD_SEC();%>);
REAL*/
/*REAL
<%
CGI_MAP(opmode, CFG_SYS_OPMODE);
%>
REAL*/