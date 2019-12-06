#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "wsc_common.h"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

char *WSCGetValueFromNameValueList(char *pbuf,
                          const char *Name, int *len)
{
	char prefix[32], postfix[32], *ptr=NULL, *endptr=NULL;

	*len = 0;
	sprintf(prefix, "<%s", Name);
	sprintf(postfix, "</%s>", Name);
	ptr = strstr(pbuf, prefix);
	if(ptr)
	{
		ptr = strstr(ptr, ">");
		/* for ">" */
		ptr += 1;
	}
	else
	{
		return ptr;
	}	
	endptr = strstr(pbuf, postfix);
	if (ptr && endptr)
	{
		*len = endptr - ptr;
	}

	return ptr;
}

void wsc_chardump(char *title, char *ptr, int len)
{

	int32 i;
	char *tmp = ptr;

	if (RT_DBG_PKT <= upnp_debug_level)
	{
		printf("\n===StartOfMsgCharDump:%s\n", title);
		for(i = 0; i < len; i++)
		{
			printf("%c", tmp[i] & 0xff);
		}
		printf("\n===EndOfMsgCharDump!\n");
	}

	return;
}

void wsc_hexdump(char *title, char *ptr, int len)
{

	int32 i;
	char *tmp = ptr;

	if (RT_DBG_PKT <= upnp_debug_level)
	{
		printf("\n---StartOfMsgHexDump:%s\n", title);
		for(i = 0; i < len; i++)
		{
			if(i%16==0 && i!=0)
				printf("\n");
			printf("%02x ", tmp[i] & 0xff);
		}
		printf("\n---EndOfMsgHexDump!\n");
	}

	return;
}

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
void ILibencodeblock( unsigned char in[3], unsigned char out[4], int len )
{
	out[0] = cb64[ in[0] >> 2 ];
	out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

/*! \fn ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
	\brief Base64 encode a stream adding padding and line breaks as per spec.
	\par
	\b Note: The encoded stream must be freed
	\param input The stream to encode
	\param inputlen The length of \a input
	\param output The encoded stream
	\returns The length of the encoded stream
*/
int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* out;
	unsigned char* in;

	*output = (unsigned char*)malloc(((inputlen * 4) / 3) + 5);
	out = *output;
	in  = input;
	
	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}
	if(!out)
	{
		diag_printf("%s,malloc failed!\n",__FUNCTION__);
		return 0;
	}

	while ((in+3) <= (input+inputlen))
	{
		ILibencodeblock(in, out, 3);
		in += 3;
		out += 4;
	}

	if ((input+inputlen)-in == 1)
	{
		ILibencodeblock(in, out, 1);
		out += 4;
	}
	else
	if ((input+inputlen)-in == 2)
	{
		ILibencodeblock(in, out, 2);
		out += 4;
	}
	*out = 0;

	return (int)(out-*output);
}

/* Decode 4 '6-bit' characters into 3 8-bit binary bytes */
void ILibdecodeblock( unsigned char in[4], unsigned char out[3] )
{
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*! \fn ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
	\brief Decode a base64 encoded stream discarding padding, line breaks and noise
	\par
	\b Note: The decoded stream must be freed
	\param input The stream to decode
	\param inputlen The length of \a input
	\param output The decoded stream
	\returns The length of the decoded stream
*/
int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output)
{
	unsigned char* inptr;
	unsigned char* out;
	unsigned char v;
	unsigned char in[4]={0};
	int i, len;
	
	if (input == NULL || inputlen == 0)
	{
		*output = NULL;
		return 0;
	}
	
	*output = (unsigned char*)malloc(((inputlen * 3) / 4) + 4);
	out = *output;
	inptr = input;
	if(!out)
	{
		diag_printf("%s,malloc failed!\n",__FUNCTION__);
		return 0;
	}	
	while( inptr <= (input+inputlen) )
	{
		for( len = 0, i = 0; i < 4 && inptr <= (input+inputlen); i++ )
		{
			v = 0;
			while( inptr <= (input+inputlen) && v == 0 ) {
				v = (unsigned char) *inptr;
				inptr++;
				v = (unsigned char) ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if( v ) {
					v = (unsigned char) ((v == '$') ? 0 : v - 61);
				}
			}
			if( inptr <= (input+inputlen) ) {
				len++;
				if( v ) {
					in[ i ] = (unsigned char) (v - 1);
				}
			}
			else {
				in[i] = 0;
			}
		}
		if( len )
		{
			ILibdecodeblock( in, out );
			out += len-1;
		}
	}
	*out = 0;
	return (int)(out-*output);
}

