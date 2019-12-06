function dayWriteMap(v1,v2,v3,v4,v5,v6,v7) {
   return (v1|(v2<<1)|(v3<<2)|(v4<<3)|(v5<<4)|(v6<<5)|(v7<<6));	
}

function dayReadMap(v, i) {
  return((v>>i)&1);
}

function macsCheck(I,s){
    var m = /[0-9a-fA-F\:]{17}/;
    if(I.length!=17 || !m.test(I)){
   	    alert("Invalid "+s+"!");
   	    I.value = I.defaultValue;
   	    return false
    }
    var mac = I.split(":");
    if ((mac[0]&0x01)||(mac[0].toUpperCase()=="FF"))
    {
    	alert(s+" must be a unique address!");
   	    I.value = I.defaultValue;
   	    return false
	}	
    return true
}

function validNumCheck(v,m) {
	var t = /[^0-9]{1,}/;
	if (t.test(v.value))
	{
		alert(m+" is not a number!");
		v.value=v.defaultValue;
		return 0;
	}
	return 1 ;
}

function rangeCheck(v,a,b,s) {

   if (!validNumCheck(v,s)) return 0;          
   if ((v.value<a)||(v.value>b)) {	
      //alert(s+" ¶W¥X½d³ò !!!") ;
      alert(s+" is between "+a+" and "+b+" !") ;
      v.value=v.defaultValue ;
      return 0 ;
   } else return 1 ;
}

function pmapCheck(v,m){
	var t = /[^0-9,-]{1,}/;
	if (t.test(v.value)) {
		alert("There is an illegal port in item "+m) ;
		return 0;
	}             
	return 1 ;
}

function strCheck(s,msg) {
	return true;
}

function scCheck(s,msg) {
	var ck=/[\;]/;
	if (ck.test(s.value))
	{
		alert(msg+" contains an invalid symbol: \;");
		return false;
	}
	return true;
}

function refresh(destination) {
   window.location = destination ;		
}

function decomList(str,len,idx,dot) {  
	var t = str.split(dot);
	return t[idx];
}

function decomListLen(str, dot){
	var t = str.split(dot);
	return t.length;
}

function typeToIdx(type) {
	if (type=="tcp/udp")
		return 2 ;
	else if(type=="udp")
     	return 1 ;
	else 
		return 0 ;	
}

function IdxToType(idx) {
	if (idx == 2)
		return "tcp/udp" ;
	else if(idx == 1)
		return "udp" ;	
	else
		return "tcp";
}

function boolToType(bool) {
   if (bool)
      return "tcp" ;
    else return "udp" ;	
}

function boolToStr(bool) {
   if (bool)
      return "1" ;
    else return "0" ;	
}

function keyCheck(F){

   	var ok = 1 ;
	var cmplen;
	var i;

	for (i=1;i<5;i++) if (F.WEPDefKey[i-1].checked) break;
	var k=eval('F.key'+i);
  
   	if (F.wep_type.selectedIndex==0)
		cmplen=10;
	else
		cmplen=26;

	if (k.value.length!=cmplen) {alert("Length of Key"+i+" must be "+cmplen); ok=0 ;}
   	return ok ;
}

function valueToDayIdx(value) {   
   return (value/86400) ;		
}

function valueToTimeIdx(value) {   
   return ((value/3600)%24) ;		
}

function setCheckValue(t) {   	
   if (t.checked) t.value=1 ;
   else t.value=0 ;	
}

function preLogout() {	
   if ((confirm('Are you sure you want to log out?'))) {                    
     window.location = "login.htm";   	     
   }
}	

function showHidden(len) {
   var s = "" ;
   for (i=0;i<len;i++)
      s=s+"*" ;
   return s ;      			
}

function combinIP(d1,d2,d3,d4)
{
    var ip=d1.value+"."+d2.value+"."+d3.value+"."+d4.value;
    if (ip=="...")
        ip="";
    return ip;
}

function combinMAC(m1,m2,m3,m4,m5,m6)
{
    var mac=m1.value+":"+m2.value+":"+m3.value+":"+m4.value+":"+m5.value+":"+m6.value;
    if (mac==":::::")
        mac="";
    return mac;
}

function verifyIP0(ipa,msg)
{
	var ip=combinIP2(ipa);
	if (ip=='' || ip=='0.0.0.0') return true;
	return verifyIP2(ipa,msg);
}
function verifyIP2(ipa,msg,subnet)
{
	ip = new Array();
	if (ipa.length==4)
	{
		for (var i=0;i<4;i++)
			ip[i]=ipa[i].value;
	}
	else
		ip=ipa.value.split(".");
	if (ip.length!=4)
	{
		alert("Invalid "+msg+"!");
		return false;
	}

    for (var i = 0; i < 4; i++)
    {
        d = ip[i];
        if (d < 256 && d >= 0)
        {
			if (i!=3 || subnet==1)
				continue;
			else
			{
 				if (d != 255 && d !=0 )
					continue;
			}
        }	
        alert("Invalid "+msg+"!");
        return false;
    }
    return true;
}

function verifyMAC(ma,s,sp){
    var t = /[0-9a-fA-F]{2}/;
	m = new Array();
	if (ma.length==6)
	{
		for (var i=0;i<6;i++)
			m[i]=ma[i].value;
	}
	else
		m=ma.value.split(":");

	if (sp) { if (m.toString()==',,,,,' ) return true; }
	for (var i=0;i<6;i++)
	{
		if (!t.test(m[i])) { alert("Invalid "+s+"!");  return false; }
    }
    if ((m[0]&0x01)||(m[0].toUpperCase()=="FF")) {	alert(s+" must be a unique address!"); return false;}
    return true
}

function decomMAC2(ma,macs,nodef)
{
    var re = /^[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}:[0-9a-fA-F]{1,2}$/;
    if (re.test(macs)||macs=='')
    {
		if (ma.length!=6)
		{
			ma.value=macs;
			return true;
		}
	if (macs!='')
        	var d=macs.split(":");
	else
		var d=['','','','','',''];
        for (i = 0; i < 6; i++)
		{
            ma[i].value=d[i];
			if (!nodef) ma[i].defaultValue=d[i];
		}
        return true;
    }
    return false;
}

function decomIP2(ipa,ips,nodef)
{
    var re = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;
    if (re.test(ips))
    {
        var d =  ips.split(".");
        for (i = 0; i < 4; i++)
		{
            ipa[i].value=d[i];
			if (!nodef) ipa[i].defaultValue=d[i];
		}
        return true;
    }
    return false;
}

function combinIP2(d)
{
	if (d.length!=4) return d.value;
    var ip=d[0].value+"."+d[1].value+"."+d[2].value+"."+d[3].value;
    if (ip=="...")
        ip="";
    return ip;
}
function combinMAC2(m)
{
    var mac=m[0].value.toUpperCase()+":"+m[1].value.toUpperCase()+":"+m[2].value.toUpperCase()+":"+m[3].value.toUpperCase()+":"+m[4].value.toUpperCase()+":"+m[5].value.toUpperCase();
    if (mac==":::::")
        mac="";
    return mac;
}

function ipMskChk(mn,str)
{
	var m=new Array();
	if (mn.length==4)
		for (i=0;i<4;i++) m[i]=mn[i].value;
	else
	{
		m=mn.value.split('.');
		if (m.length!=4) { alert("Invalid "+str+"!") ; return 0; }
	}

	var t = /[^0-9]{1,}/;
	for (var i=0;i<4;i++)
	{
		if (t.test(m[i])||m[i]>255) { alert("Invalid "+str+"!") ; return 0; }
	}

	var v=(m[0]<<24)|(m[1]<<16)|(m[2]<<8)|(m[3]) ;

	var f=0 ;	  
	for (k=0;k<32;k++)
	{
		if ((v>>k)&1) f=1;
		else if (f==1)
		{
			alert("Invalid "+str+"!") ;
			//for(var i=0; i<4; i++) m[i].value=m[i].defaultValue;
			return 0 ;
		}
	}
	if (f==0) { alert("Invalid "+str+"!") ; return 0; }
	return 1 ;	
}

function Cfg(i,n,v)
{
	this.i=i;
    this.n=n;
    this.v=this.o=v;
}

var CA = new Array() ;

function addCfg(n,i,v)
{
	CA.length++;
    CA[CA.length-1]= new Cfg(i,n,v);
}

function idxOfCfg(kk)
{
    if (kk=='undefined') { alert("Undefined"); return -1; }
    for (var i=0; i< CA.length ;i++)
    {

        if ( CA[i].n != 'undefined' && CA[i].n==kk )
            return i;
    }
    return -1;
}

function getCfg(n)
{
	var idx=idxOfCfg(n)
	if ( idx >=0)
		return CA[idx].v ;
	else
		return "";
}

function setCfg(n,v)
{
	var idx=idxOfCfg(n)
	if ( idx >=0)
	{
//debug, if (CA[idx].v != v) alert("setCfg("+n+","+v+")");
		CA[idx].v = v ;
	}
}

function cfg2Form(f)
{
    for (var i=0;i<CA.length;i++)
    {
        var e=eval('f.'+CA[i].n);
        if ( e )
		{
			if (e.name=='undefined') continue;
			if ( e.length && e[0].type=='text' )
			{
				if (e.length==4) decomIP2(e,CA[i].v);
				else if (e.length==6) decomMAC2(e,CA[i].v);
			}
			else if ( e.length && e[0].type=='radio')
			{
				for (var j=0;j<e.length;j++)
					e[j].checked=e[j].defaultChecked=(e[j].value==CA[i].v);
			}
			else if (e.type=='checkbox')
				e.checked=e.defaultChecked=Number(CA[i].v);
			else if (e.type=='select-one')
			{
				for (var j=0;j<e.options.length;j++)
					 e.options[j].selected=e.options[j].defaultSelected=(e.options[j].value==CA[i].v);
			}
			else
				e.value=getCfg(e.name);
			if (e.defaultValue!='undefined')
				e.defaultValue=e.value;
		}
    }
}

var frmExtraElm='';
function form2Cfg(f)
{

    for (var i=0;i<CA.length;i++)
    {
        var e=eval('f.'+CA[i].n);
		if ( e )
		{
			if (e.disabled) continue;
			if ( e.length && e[0].type=='text' )
			{
				if (e.length==4) CA[i].v=combinIP2(e);
				else if (e.length==6) CA[i].v=combinMAC2(e);
			}
			else if ( e.length && e[0].type=='radio')
			{
				for (var j=0;j<e.length;j++)
					if (e[j].checked) { CA[i].v=e[j].value; break; }
			}
			else
			if (e.type=='checkbox')
				setCfg(e.name, Number(e.checked) );
			else
				setCfg(e.name, e.value);
		}
    }
}

var OUTF;
function frmHead(na,to,cmd,go)
{
	OUTF="<FORM name="+na+" action="+to+" method=POST>\n"+
	"<INPUT type=hidden name=CMD value="+cmd+">\n"+
	"<INPUT type=hidden name=GO value="+go+">\n";
}

function frmEnd()
{
	OUTF+="</FORM>\n";
}

function frmAdd(n,v)
{
	set1="<input type=hidden name="+n+" value=\"";
	v=v.replace(/\"/g,"&quot;");
	var r=new RegExp(set1+".*\n","g");
	if (OUTF.search(r) >= 0)
		OUTF=OUTF.replace(r,(set1+v+"\">\n"));
	else
		OUTF += (set1+v+"\">\n");
}

function genForm(n,a,d,g)
{
	frmHead(n,a,d,g);
	var sub=0;
    for (var i=0;i<CA.length;i++)
	{
		if (CA[i].v!=CA[i].o)
		{
			frmAdd("SET"+sub,String(CA[i].i)+"="+CA[i].v);
			sub++;
		}
	}
	if (frmExtraElm.length)
		OUTF+=frmExtraElm;
	frmExtraElm=''; //reset
	frmEnd();
	return OUTF;
}

function subForm(f1,a,d,g)
{
	var msg=genForm('OUT',a,d,g);
/*DEMO*/
	if (!confirm(msg)) return;
/*END_DEMO*/
	/*alert(OUTF);*/
	var newElem = document.createElement("div");
	newElem.innerHTML = msg ;
	f1.parentNode.appendChild(newElem);
	f=document.OUT;
	f.submit();
}

function addFormElm(n,v)
{
	var set1='<input type=hidden name='+n+' value="'+v+'">\n';
	frmExtraElm += set1;
}

function pwdSame(p,p2)
{
   	if (p != p2) { alert("Password Inconsistent!") ; return 0 ; }
   	else return 1 ;
}

function chkPwdUpdate(p,pv,c)
{
	if (c.value=='0') return true;
    // modified
    if (!pwdSame(p.value,pv.value)) return false;
    if (!confirm('Are you sure you want to change the password?'))
    {
        c.value=0 ;
        p.value=pv.value=p.defaultValue;
        return false;
    }
    return true;
}

function chkPwd1Chr(p,pv,c)
{
   	if (c.value=='0')
   	{
  		p.value=pv.value=""; // reset to null;
  		c.value='1';
	}
}

function chkPwd1Chr2(po,p,pv,c)
{
   	if (c.value=='0')
   	{
  		po.value=p.value=pv.value=""; // reset to null;
  		c.value='1';
	}
}

function chkStrLen(s,m,M,msg)
{
	var str=s.value;
	if ( str.length < m || str.length > M )
	{
		alert(msg+" Length Out of Range!");
		return false;
	}
    return true;
}

function isIE()
{
	var agt = navigator.userAgent.toLowerCase();
	return (agt.indexOf("msie") != -1); // ie
}

function fit2(n)
{
	var s=String(n+100).substr(1,2);
	return s;
}

function timeStr(t)
{
	if(t < 0)
	{
		str='00:00:00';
		return str;
	}
	var s=t%60;
	var m=parseInt(t/60)%60;
	var h=parseInt(t/3600)%24;
	var d=parseInt(t/86400);

	var str='';
	if (d > 999) { return 'Permanent'; }
	if (d) str+=d+'Day ';
	str+=fit2(h)+':';
	str+=fit2(m)+':';
	str+=fit2(s);
	return str;
}

// auto,NA,IC,ETS,SP,FR,JP
var dmnRng= new Array(16383,2047,2047,8191,1536,7680,16383);

function chanList(Opt,dn)
{
	var j = 0;
	for(var i=1;i<=14;i++)
	{
		if(dmnRng[dn] & (1<<(i-1)))
        {
			var fr;
			if (i!=14) fr=i*0.005+2.407;
			else fr=2.484;
			var opn = new Option(i+" - "+fr+"GHz",i);
			Opt.options[j++] = opn;
		}
	}
}

function rmEntry(a,i)
{
	if (a.splice)
		a.splice(i,1);
	else
	{
		if (i>=a.length) return;
		for (var k=i+1;k<=a.length;k++)
			a[k-1]=a[k];
		a.length--;
	}
}


function getStyle(objId) {
	var obj=document.getElementById(objId);
	if (obj) return obj.style;
	else return 0;
}

function setStyle(id, v) {
    var st = getStyle(id);
    if(st) { st.visibility = v; return true; } else return false;
} 

var   lastPage;

function switchPage(c, n) {
  	if (getStyle(c) && getStyle(n)) {
		lastPage=c;
		setStyle(c, "hidden");
		setStyle(n, "visible");
	}

}

function dateStr(sec)
{
var mons=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
	var d=new Date(1000*sec);
	d.setTime(d.getTime()+d.getTimezoneOffset()*60000);
	var str=mons[d.getMonth()]+'/'+(d.getDate())+'/'+d.getFullYear()+' ';
var h=(100+d.getHours()).toString().substr(1,2);
var m=(100+d.getMinutes()).toString().substr(1,2);
var s=(100+d.getSeconds()).toString().substr(1,2);
	str+=h+':'+m+':'+s;
	return str;
}
function setIdVal(id,val)
{
document.getElementById(id).innerHTML=val;
}

function ByCell_IPcheck(ad, num)
{
	var msg = new Array(3);
	var i;
	var pad = ad.split(".");
	msg[0]="Invail IP Address";
	msg[1]="Invail Mask Address";
	msg[2]="Invail Gateway Address";
	
	if(pad.length<4)
	{
			alert(msg[num]);
			return false;
	}
	
	for(i=0;i<pad.length;i++)
		if((parseInt(pad[i])<0)||(parseInt(pad[i])>255)||(!isNumber(pad[i])))
		{
			alert(msg[num]);
			return false;
		}
}

function ByMask_IPrange(IP, MASK, SoE)
{
	var IP_AMOUNT_MAX = 256;
	var ip = IP.split(".");
	var mask = MASK.split(".");
	var i;

	for(i=0;i<ip.length;i++)
	{
		/* SoE=0:show this Network IP ; SoE=1:show this Network Broadcast IP */
		if(SoE==1)
		{
			mask[i] = ~mask[i] + IP_AMOUNT_MAX;
			ip[i] = ip[i] | mask[i];
		}
		else
			ip[i] = ip[i] & mask[i];
	}
	return ip.join(".");
}

function BinaryChk_IP(IP,MASK,GW)
{
	var NetworkIP = ByMask_IPrange(IP, MASK, 0);
	var BroadcastIP = ByMask_IPrange(IP, MASK, 1);
	
	if(ByCell_IPcheck(IP, 0)==false){return false};
	if(ByCell_IPcheck(MASK, 1)==false){return false};
	if(ByCell_IPcheck(GW, 2)==false){return false};
	
	if(NetworkIP==IP)
	{
			alert("Attention:IP value should not be Network IP.\nNetwork IP : "+NetworkIP+"\nBroadcast IP : "+BroadcastIP);
			return false;
	}
	if(BroadcastIP==IP)
	{
			alert("Attention:IP value should not be Broadcast IP.\nNetwork IP : "+NetworkIP+"\nBroadcast IP : "+BroadcastIP);
			return false;
	}
	if(NetworkIP==GW)
	{
			alert("Attention:Gateway value should not be Network IP.\nNetwork IP : "+NetworkIP+"\nBroadcast IP : "+BroadcastIP);
			return false;
	}
	if(BroadcastIP==GW)
	{
			alert("Attention:Gateway value should not be Broadcast IP.\nNetwork IP : "+NetworkIP+"\nBroadcast IP : "+BroadcastIP);
			return false;
	}
	if(NetworkIP!=ByMask_IPrange(GW, MASK, 0))
	{
			alert("Attention:IP and Gateway are not in the same subnet!!");
			return false;
	}
	return true;
}

function prints(context)
{	
	document.write(context);
}
