var X = new String("DCBA");
var Y = new String("1234");

function CheckMAC(form, name, no)
{
	var	val, i;
	
	val = eval('document.'+form+'.'+name+'.value');		
	
	for(i = 0; i < 5; i++)
	{
		if(isNaN(parseInt(val.charAt(3*i), 16)))
		{
			alert('The MAC address of No. '+no+' is worng format or using incorrect characters.');
		    eval('document.'+form+'.'+name+'.focus()');
		    eval('document.'+form+'.'+name+'.select()');			
			return false;
		}	
		else if(isNaN(parseInt(val.charAt(3*i+1), 16)))
		{
			alert('The MAC address of No. '+no+' is worng format or using incorrect characters.');
		    eval('document.'+form+'.'+name+'.focus()');
		    eval('document.'+form+'.'+name+'.select()');				
		    return false;
		}    
		else if (val.charAt(3*i+2) != ':')
		{
			alert('The MAC address of No. '+no+' is worng format or using incorrect characters.');
		    eval('document.'+form+'.'+name+'.focus()');
		    eval('document.'+form+'.'+name+'.select()');				
		    return false;     	
		}    
	}
	if(isNaN(parseInt(val.charAt(15), 16)))
	{
	   alert('The MAC address of No. '+no+' is worng format or using incorrect characters.');
	   eval('document.'+form+'.'+name+'.focus()');
	   eval('document.'+form+'.'+name+'.select()');			
	   return false;
	}	
	else if(isNaN(parseInt(val.charAt(16), 16)))
	{
	   alert('The MAC address of No. '+no+' is worng format or using incorrect characters.');
	   eval('document.'+form+'.'+name+'.focus()');
	   eval('document.'+form+'.'+name+'.select()');				
	   return false;
	}    	
	
	return true;
}

function CheckRange(form, name, low, high, msg)
{
	var	val;
	var val2;
    
    val = eval('document.'+form+'.'+name+'.value');
    val2 = 1*val;
    
    if (val == "")
    {
      alert(msg+' is not entered.');
	  eval('document.'+form+'.'+name+'.focus()');
	  eval('document.'+form+'.'+name+'.select()');      
      return false;
    }
    else if (isNaN(val2))
    {
    	alert(msg+' should be numeric.');
		eval('document.'+form+'.'+name+'.focus()');
		eval('document.'+form+'.'+name+'.select()');    	
    	return false;    	
    }  
    else if(isNaN(parseInt(val, 10)))
    {
    	alert(msg+' should be numeric.');
		eval('document.'+form+'.'+name+'.focus()');
		eval('document.'+form+'.'+name+'.select()');    	
    	return false;
    }
    else if(val - parseInt(val, 10) > 0)
    {
    	alert(msg+' should be numeric.');
		eval('document.'+form+'.'+name+'.focus()');
		eval('document.'+form+'.'+name+'.select()');    	
    	return false;
    }    	    
	else if (val > high || val < low)
	{
		alert(msg+' has the out of ranged value.');	
		eval('document.'+form+'.'+name+'.focus()');
		eval('document.'+form+'.'+name+'.select()');
	    return false;
	}
	return true;
}

function CheckIP(form, name, msg)
{
	var i;

	if(CheckRange(form, name+Y.charAt(0), 0, 223, msg)==false)
			return false;
	for(i=1; i<=3; i++)
	{
		if(CheckRange(form, name+Y.charAt(i), 0, 255, msg)==false)
			return false;
	}
	return true;
}

function CheckMask(form, name, msg)
{
	var i;
	
	for(i=0; i<=3; i++)
	{
		if(CheckRange(form, name+Y.charAt(i), 0, 255, msg)==false)
			return false;
	}
	return true;
	
}