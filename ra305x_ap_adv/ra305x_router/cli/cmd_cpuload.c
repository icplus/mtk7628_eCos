#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <pkgconf/system.h>
#include <cyg/kernel/kapi.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <cli_cmd.h>

externC int  diag_printf( const char *fmt, ... ) CYGBLD_ATTRIB_PRINTF_FORMAT(1,2); 
extern cyg_handle_t cli_thread_h;
static void cpuload_alarm_func(cyg_handle_t alarm,cyg_addrword_t data);

typedef struct cyg_cpuload_s {  
		cyg_alarm alarm_s; 
		cyg_handle_t alarmH;  
		cyg_uint32 last_idle_loops;  
		cyg_uint32 average_point1s;  
		cyg_uint32 average_1s;  
		cyg_uint32 average_10s;  
		cyg_uint32 calibration;
		} cyg_cpuload_t;




externC int  diag_printf( const char *fmt, ... ) CYGBLD_ATTRIB_PRINTF_FORMAT(1,2);  
/* Here we run the idle thread as a high priority thread for 0.1
   second.  We see how much the counter in the idle loop is
   incremented. This will only work if there are no other threads
   running at priority 1 and 2 */

static cyg_thread       cpuload_thread;
static cyg_handle_t     cpuload_thread_handle;

static unsigned int      calibration=0;
static cyg_cpuload_t    cpuload_obj;
static cyg_handle_t     cpuload_handle;


static cyg_handle_t     alarm_hdl;
static cyg_alarm        alarm_obj;


static volatile unsigned int cpuload_counter=0;
static char cpuload_idle_stack[2048];//old 4096

void static 
alarm_func(cyg_handle_t alarm,cyg_addrword_t data) { 
  cyg_handle_t idleH = (cyg_handle_t) data;
  cyg_thread_suspend(idleH); 
}
void delay_cpuloading(void)
{
int i,j;
for (i=0;i<100000;i++)
 for (j=0;j<100000;j++);
}

void cpuload_idle(CYG_ADDRESS data)
{
   cpuload_counter=0;
	//diag_printf("\ncpuload_idle():cpuload_counter=%d\n",cpuload_counter);
	for (;;)
	{
		cpuload_counter++;
		delay_cpuloading();

	}	
}


/* Calibrate the cpuload measurements.*/
void 
cpuload_init(cyg_uint32  *calibration) {
  cyg_handle_t counter;
  cyg_alarm alarm_s;
  cyg_handle_t alarmH;
  cyg_uint32 idle_loops_start;  

  cyg_priority_t old_priority;
  
  cyg_thread_create(1,
		    cpuload_idle,
		    0,
		    "cpuload",
		    cpuload_idle_stack,
		    sizeof(cpuload_idle_stack),
		    &cpuload_thread_handle,
		    &cpuload_thread);
  
  cyg_clock_to_counter(cyg_real_time_clock(),&counter);
  cyg_alarm_create(counter,alarm_func,(cyg_addrword_t)cpuload_thread_handle,&alarmH,&alarm_s);
  
  cyg_alarm_initialize(alarmH,cyg_current_time()+10,0);
  cyg_alarm_enable(alarmH);
  //diag_printf("\n[cpuload_init]:cpuload_counter=%u",cpuload_counter);
  idle_loops_start = cpuload_counter;
  
  /* Dont be decieved, remember this is a multithreaded system ! */
  old_priority = cyg_thread_get_priority(cyg_thread_self());
  cyg_thread_set_priority(cyg_thread_self(),2);
  //call this function ,i will be preempt
  cyg_thread_resume(cpuload_thread_handle);
  cyg_thread_set_priority(cyg_thread_self(), old_priority);
  
  *calibration = cpuload_counter - idle_loops_start;
  diag_printf("\n[cpuload_init]:calibration=%u", *calibration);
  cyg_alarm_delete(alarmH);

  //diag_printf("\n[cpuload_init]:cpuload_counter=%u", cpuload_counter);
  cyg_thread_set_priority(cpuload_thread_handle,30);
  
}

static void 
cpuload_alarm_func(cyg_handle_t alarm,cyg_addrword_t data) { 
  cyg_cpuload_t * cpuload = (cyg_cpuload_t *)data;
  cyg_uint32 idle_loops_now = cpuload_counter;
  cyg_uint32 idle_loops=0;
  cyg_uint32 load =0;
  
  //diag_printf("\n[cpuload_alarm_func]:Entry");
  if (idle_loops_now >= cpuload->last_idle_loops) {
    idle_loops = idle_loops_now - cpuload->last_idle_loops;
  } else {
    idle_loops = ~0 - (cpuload->last_idle_loops - idle_loops_now);
  }
  
  /* We need 64 bit arithmatic to prevent wrap around */ 
  if (cpuload->calibration!=0)
  {
  load = (cyg_uint32) (((cyg_uint64) idle_loops * (cyg_uint64)100) / 
		       (cyg_uint64)cpuload->calibration);
  }
  //else 
  //diag_printf("\n[cpuload_alarm_func]:calibration=0");
  if (load > 100) {
    load = 100;
  }
  load = 100 - load;
  //diag_printf("\n\ncalibration=%d",cpuload->calibration);
  cpuload->average_point1s = load;
  cpuload->average_1s = load + ((cpuload->average_1s * 90)/100);
  cpuload->average_10s = load + ((cpuload->average_10s * 99)/100);
  cpuload->last_idle_loops = idle_loops_now;
  //diag_printf("\n\nin Cpuload:idle_loops_now=%d",idle_loops_now);
  //diag_printf("\n\nIn cpuload_alarm_fun();\n%d:%d:%d",cpuload->average_point1s,cpuload->average_1s,cpuload->average_10s);
}

/* Create a CPU load measurements object and start the
   measurements. */
externC void 
cpuload_start(cyg_cpuload_t *cpuload,
                   cyg_uint32 calibration, 
                   cyg_handle_t *handle) 
{
  cyg_handle_t counter;
  
  cpuload->average_point1s = 0;
  cpuload->average_1s = 0;
  cpuload->average_10s = 0;
  cpuload->calibration = calibration;
  cpuload->last_idle_loops = cpuload_counter;

  cyg_clock_to_counter(cyg_real_time_clock(),&counter);
  cyg_alarm_create(counter,
		   cpuload_alarm_func,
		   (cyg_addrword_t)cpuload,
		   &cpuload->alarmH,
		   &cpuload->alarm_s);
  cyg_thread_resume(cpuload_thread_handle);
  cyg_alarm_initialize(cpuload->alarmH,cyg_current_time()+10,10);
  cyg_alarm_enable(cpuload->alarmH);
  
  *handle = (cyg_handle_t) cpuload;
}

/* Stop measurements of the cpuload. The cyg_cpuload_t object can then
   be freed. */
externC void 
cpuload_delete(cyg_handle_t handle) {
  cyg_cpuload_t * cpuload = (cyg_cpuload_t *) handle;
  
  cyg_alarm_delete(cpuload->alarmH);
  cyg_thread_suspend(cpuload_thread_handle);
  //cyg_thread_kill(cpuload_thread_handle);
  //cyg_thread_delete(cpuload_thread_handle);
}  

/* Return the cpuload for the last 100ms, 1seconds and 10 second */
externC void
cpuload_get(cyg_handle_t handle,
                cyg_uint32 *average_point1s, 	    
                cyg_uint32 *average_1s, 	    
                cyg_uint32 *average_10s) {
  //diag_printf("\n[cpuload_get]:Entry");
  cyg_cpuload_t * cpuload = (cyg_cpuload_t *) handle;
  *average_point1s = cpuload->average_point1s;
  *average_1s = cpuload->average_1s/10;
  *average_10s = cpuload->average_10s/100;
 //diag_printf("\n\nIn cyg_cpuload_get();\n%d:%d/10:%d/100",cpuload->average_point1s,cpuload->average_1s,cpuload->average_10s);
}


void load_show(cyg_handle_t alarm_handle,cyg_addrword_t handle)
{
  cyg_uint32 average_point1s;
  cyg_uint32 average_1s;
  cyg_uint32 average_10s;

 // diag_printf("\n[load_show]:show timer Entry");
  //diag_printf("\n[load_show]:handle=%x",handle);
  cpuload_get((cyg_handle_t)handle,&average_point1s,&average_1s,&average_10s);
  diag_printf("\n\n[cpuload]:1s load=%d% :10s load=%d%\n",average_1s,average_10s);
}

void ra35xx_cpuload_start(void)
{

   cyg_handle_t counter_hdl;
   cyg_handle_t sys_clk;

   //diag_printf("\n[ra35xx_cpuload_start]:Entry");
   cpuload_start(&cpuload_obj,calibration,&cpuload_handle);   
   sys_clk=cyg_real_time_clock();
   cyg_clock_to_counter(sys_clk,
			&counter_hdl);
   cyg_alarm_create(counter_hdl,
			load_show,
			cpuload_handle,
			&alarm_hdl,
			&alarm_obj);
    cyg_alarm_initialize(alarm_hdl,cyg_current_time()+100,100);
    //diag_printf("\n[ra35xx_cpuload_start]:Exit");
}
   


void ra35xx_cpuload_stop(void)
{
   cpuload_delete(cpuload_handle);
   cyg_alarm_delete(alarm_hdl);
   cpuload_counter=0;
}


void ra35xx_cpuload_init(void)
{
  //cyg_priority_t old_priority;
  //old_priority = cyg_thread_get_priority(cli_thread_h);
  //cyg_thread_set_priority(cli_thread_h,3);
  cpuload_init(&calibration);
  //cyg_thread_set_priority(cli_thread_h,old_priority);



}


void cpuload_cmd(int argc,char *argv[])
{
    static unsigned char cpuload_is_running=0;	
    if (argc == 1)
    {
	if (!strcmp(argv[0],"start"))
		{
		   if (cpuload_is_running)
		   	  return ;
		   else
		   {

			 //ra35xx_cpuload_init();
		   	 ra35xx_cpuload_start();
			 cpuload_is_running = 1;
			 return ;
		   }
		} 
	else if (!strcmp(argv[0],"stop"))
		{
		  if (cpuload_is_running==1)
		  	{
		  	 ra35xx_cpuload_stop();
			 cpuload_is_running=0;
			 return ;
		  	}
		  else
		  	 return ;
		}
    	}
	diag_printf("cpuload start/stop\r\n");
	return ;
}




