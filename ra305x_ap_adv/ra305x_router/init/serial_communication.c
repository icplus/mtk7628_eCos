#include <cyg/io/serialio.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include "cyg/kernel/kapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cfg_id.h>
#include <serial_communication.h>

#define SCIF_BAUDRATE	CYGNUM_SERIAL_BAUD_115200
#define STACK_SIZE	0x1000
#define DEV	"/dev/ser1"
static char stack[STACK_SIZE];
static cyg_thread thread_data;
static cyg_handle_t thread_handle;
extern int WLAN_start(int cmd);

unsigned char at_cmd[3][5] = {
	"AT+0", // wifi start
	"AT+1", // wifi stop
	"AT+2" // send apclient info
};

int  get_cfg(int id, char *buf)
{
	CFG_get_str(id, buf);
	sc_convert(buf);

	return 0;
}

int  serial_write(cyg_io_handle_t ser, char *name, char *val)
{
	int len;
	char send_buf[64] = {0};

	memset(send_buf, 0, sizeof(send_buf));
	snprintf(send_buf, sizeof(send_buf), "%s=%s\r\n", name, val);

	len =  strlen(send_buf);
	cyg_io_write(ser, send_buf, &len);

	return 0;
}

void handle_func(cyg_addrword_t p)
{
	Cyg_ErrNo err;
	cyg_uint32 len;
	cyg_io_handle_t ser;
	static char read_buf[8] = {0};
	int res, i=0;
	char enable = '\0';
	char c = '\0';
	char bssid[32] = {0};
	char auth_mode[16] = {0};
	char encry[16] = {0};
	char wpapsk[32] = {0};

	printf("===> uart communication init \n");
	cyg_serial_info_t serial_info =
		CYG_SERIAL_INFO_INIT( SCIF_BAUDRATE,
							CYGNUM_SERIAL_STOP_1,
							CYGNUM_SERIAL_PARITY_NONE,
							CYGNUM_SERIAL_WORD_LENGTH_8,
							0);
	len = sizeof(serial_info);
	err = cyg_io_lookup(DEV, &ser);
	if (ENOERR != err)
	{
		diag_printf("Open %s failed!\n", DEV);
		diag_printf("Please check if enable 'CYGPKG_IO_SERIAL_DEVICES' or not.\n");
		return;
	}

	res = cyg_io_set_config(ser, CYG_IO_SET_CONFIG_SERIAL_INFO, &serial_info, &len);
	if (res != ENOERR)
		diag_printf("set_config flow_control returned an error\n");

	while(true)
	{
		// clear
		len = 1;
		memset(read_buf, 0, sizeof(read_buf));
		//read
		cyg_io_read(ser, &read_buf[0], &len);
		if(read_buf[0] == 'A')
		{
			cyg_io_read(ser, &read_buf[1], &len); // 'T
			cyg_io_read(ser, &read_buf[2], &len); // '+'
			cyg_io_read(ser, &read_buf[3], &len); // '0' or '1' or '2'

			// 打印 串口换行
			len = 1;
			c = '\r';
			cyg_io_write(ser, &c, &len);
			c = '\n';
			cyg_io_write(ser, &c, &len);

			// 打印 串口输入的内容
			len = strlen(read_buf);
			cyg_io_write(ser, read_buf, &len);

			// "AT+0"; wifi start
			if( !strncmp(at_cmd[0], read_buf, 4))
			{
				WLAN_start(1);
			}
			// "AT+1"; wifi stop
			else if( !strncmp(at_cmd[1], read_buf, 4))
			{
				WLAN_start(0);
			}
			// "AT+2"; send apclient info
			else if( !strncmp(at_cmd[2], read_buf, 4))
			{
				get_cfg(CFG_WLN_ApCliEnable, &enable);
				printf("ApCliEnable = %c\n", enable);

				// 打印 串口换行
				len = 1;
				c = '\r';
				cyg_io_write(ser, &c, &len);
				c = '\n';
				cyg_io_write(ser, &c, &len);

				// WLN_ApCliEnable
				get_cfg(CFG_WLN_ApCliBssid, bssid);
				serial_write(ser, "ApCliBssid", bssid);

				//WLN_ApCliBssid
				get_cfg(CFG_WLN_ApCliAuthMode, auth_mode);
				serial_write(ser, "ApCliAuthMode", auth_mode);

				// WLN_ApCliAuthMode
				get_cfg(CFG_WLN_ApCliEncrypType, encry);
				serial_write(ser, "ApCliEncrypType", encry);

				// WLN_ApCliEncrypType
				get_cfg(CFG_WLN_ApCliWPAPSK, wpapsk);
				serial_write(ser, "WLN_ApCliWPAPSK", wpapsk);
			}
		}
		else
			cyg_thread_delay(5);    //以 x * 10ms为单位; 50ms
	}
}

void cyg_user_start(void)
{
	// Create a main thread, so we can run the scheduler and have time 'pass'
	cyg_thread_create(6,		// Priority - just a number
			handle_func,		// entry
			0,				// entry parameter
			"serial_test",		// Name
			&stack[0],		// Stack
			STACK_SIZE,		// Size
			&thread_handle,	// Handle
			&thread_data		// Thread data structure
		);

	cyg_thread_resume(thread_handle);	// Start it
}

void serial_communication_init(void)
{
	cyg_user_start();
}


#if 0
CGI_MAP(opmode, CFG_SYS_OPMODE);
#define	CGI_MAP(name,id)		CGI_var_map(req,#name,id)
void CGI_var_map(http_req *req, char *name, int id)
{
	char val[512];
	CFG_get_str(id, val);
	sc_convert(val);
	WEB_printf(req, "addCfg('%s',0x%08x,'%s');\n",name,id,val);
}
#endif
