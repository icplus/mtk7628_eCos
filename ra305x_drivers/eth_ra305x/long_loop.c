#include <config.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/cyg_type.h>

#include <stdio.h>
#include <cyg/infra/diag.h>

#include "if_ra305x.h"

#define LOOP_DELAY 500000 // in us

#define printf diag_printf


static cyg_handle_t h_long_loop_t;
static cyg_thread long_loop_t;
static char long_loop_stack[1024*4];

static char long_loop_run=0;

void clear_max_gain_flag(int port_num)
{
	int i;
	int address[6] = {31, 16, 31, 26, 31, 16};
	int value[6] = {0x8000, 0x0001, 0x0000, 0x0002, 0x8000, 0x0000};

	for(i=0; i<6; i++)
	{
		mii_write_register(port_num, address[i], value[i]);
	}
	
}

int read_max_gain_flag(int port_num)
{
	int val_out;

	mii_write_register(port_num, 31, 0x8000);
	//need delay???
	val_out = mii_read_register(port_num, 22);

	if(val_out&0x0400)
		return 1;
	else
		return 0;
}


int read_signal_detect_flag(int port_num)
{
	static int Up2Down_cnt[5] = {0, 0, 0, 0, 0};
	int val_out;
	

	mii_write_register(port_num, 31, 0x8000);
	//need delay???
	val_out = mii_read_register(port_num, 28);

	if ((val_out & 0x0001) == 0x0001 ){ // link up
		if(Up2Down_cnt[port_num] > 0)
			Up2Down_cnt[port_num]--;
	}else if ((val_out & 0x8000) == 0x8000 ){ //plug but not link up
		Up2Down_cnt[port_num]++;
	}

	if( Up2Down_cnt[port_num] > 3){
		Up2Down_cnt[port_num] = 0;
		return 1;
	}else
		return 0;
}

int read_link_status(int port_num)
{
	int val_out;

	mii_write_register(port_num, 31, 0x8000);
	//need delay???
	val_out = mii_read_register(port_num, 28);

	if(val_out&0x0001)
		return 1;
	else
		return 0;

}


void set_100M_extend_setting(int port_num)
{

	int i;
	int address[] = {31, 19, 31, 21, 31, 0};
    int value[] = {0x8000, 0x6710, 0xa000, 0x8513, 0x8000, 0x3100};

	for( i=0; i<4; i++)
	{
		mii_write_register(port_num, address[i], value[i]);
	}

}

void set_100M_extend_setting_manual(int port_num)
{

	int i;
	int address[] = {31, 19, 31, 21, 31, 4};
    int value[] = {0x8000, 0x6710, 0xa000, 0x8513, 0x8000, 0x5e1};

	for( i=0; i<6; i++)
	{
		mii_write_register(port_num, address[i], value[i]);
	}

}


void set_100M_normal_setting(int port_num)
{
	int i;
	int address[] = {31, 19, 31, 21, 31, 4, 0};
	int value[] = {0x8000, 0x6750, 0xa000, 0x8553, 0x8000, 0x5e1, 0x3100};

	for( i=0; i<7; i++)
	{
		mii_write_register(port_num, address[i], value[i]);
	}

}

void set_10M_setting(int port_num)
{
	mii_write_register(port_num, 4, 0x0461);
	//need delay???
	mii_write_register(port_num, 0, 0x3100);	
}

int long_loop_main(void)
{
	int ret, i;
	int pre_link_state = 0;
	int port_state[5] = {0, 0, 0, 0, 0}; // 0: 100M normal; 1: 100M extend; 2: 10M
	int port_skip_loop_cnt[5] = {0, 0, 0, 0, 0}; // skip loop count for mode change

	for( i = 0; i < 5; i++)	{
		clear_max_gain_flag(i);
	}

	while(1){
		for (i=0; i<5;i++){
			if(port_skip_loop_cnt[i] > 0){
				port_skip_loop_cnt[i]--;
			}else{
				if(port_state[i]==0){ //at 100M normal setting
#ifdef CONFIG_USER_LONG_LOOP_3_STATE
					if(read_max_gain_flag(i)){			
						port_state[i] = 1;
						clear_max_gain_flag(i);
						set_100M_extend_setting(i);
						port_skip_loop_cnt[i] = 6;
						printf("port#%d switch to 100M extend setting\n",i);
					}else 
#endif						
						if(read_signal_detect_flag(i)){
							port_state[i]=2;
							set_10M_setting(i);
							port_skip_loop_cnt[i] = 10;
							printf("port#%d switch to 10M setting\n",i);
						}
				}else if(port_state[i]==1){// at 100M extend setting
					if(!read_link_status(i)){
						if(pre_link_state == 1){//unplug calbe
							port_state[i]=0;
							clear_max_gain_flag(i);
							set_100M_normal_setting(i);
							printf("port#%d switch to 100M normal setting\n",i);
						}else /*if(read_max_gain_flag(i))*/{
							port_state[i]=2;
							set_10M_setting(i);
							port_skip_loop_cnt[i] = 10;
							printf("port#%d switch to 10M setting\n",i);
						}
						pre_link_state = 0;
					}else{
						pre_link_state = 1;
					}
				}else if(port_state[i]==2){   //at 10M mode
					if(!read_link_status(i)){
						port_state[i]=0;
						clear_max_gain_flag(i);
						set_100M_normal_setting(i);
						printf("port#%d switch to 100M normal setting\n",i);
					}
				}
			}
		}
		//usleep(LOOP_DELAY);
		cyg_thread_delay(LOOP_DELAY/1000/10);	//cyg_thread_delay unit:10ms

	}
	return 0;

}

void long_loop_start()
{	
	if(!long_loop_run)
	{
		cyg_thread_create(10, 
						long_loop_main,
						(cyg_addrword_t) 0,
						"long_loop_thread",
						(void *)&long_loop_stack[0],
						sizeof(long_loop_stack),
						&h_long_loop_t,
						&long_loop_t);
						  
		cyg_thread_resume(h_long_loop_t);

		long_loop_run = 1;
	}
}

void long_loop_stop()
{
	if(long_loop_run)
	{
		cyg_thread_delete(h_long_loop_t);
		long_loop_run = 0;
	}
}



