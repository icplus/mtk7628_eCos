//var _LM_=(<%CGI_CFG_GET(CFG_SYS_LM);%>)?<%CGI_CFG_GET(CFG_SYS_LM);%>:0;
//var _LM_=0;
var _LM_=<%CGI_GET_LM(CFG_SYS_LM);%>;
var debugmode=0;
var langmode=['en','cn']
function dw(str){document.write(str+"\n")}
function loadjs(lm)
{
	var srcv=langmode[_LM_]+'_ssetup.js';
    	dw("<script type='text/javascript' src='"+srcv+"'></script>");
}
loadjs(_LM_)

function chglang(func)
{
    var func=lang
    if((typeof func) !="string"){return}
    try{eval("langa="+func)}catch(e){return}
    for(l=0;l<langa.length;l++)
    {
	SetElmCont("_"+l,langa[l]);
    }
}
function SetElmCont(idx,cont)
{
	ddiv=document.getElementById(idx);
	if(cont=="ddiv.innerHTML")return false
	if(ddiv!=null)
	{
		if(debugmode&&(idx.substr(0,1)=="_"))ddiv.style.color='#ff0000';
		if(ddiv.text!=null) ddiv.text=cont
			else ddiv.innerHTML=cont
				if(debugmode)ddiv.style.color='red'
				return true
	}
	else return false
}
