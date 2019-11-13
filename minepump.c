#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <semaphore.h>
//vishal :- vishal.vishal@student.isae-supaero.fr
#include "msg_box.h"
#include "minteger.h"
#include "periodic_task.h"
#include "simu.h"
#include "utils.h"

#define MS_L1 70
#define MS_L2 100

#define Normal 0
#define Alarm1 1
#define Alarm2 2
#define LowLevel 0
#define HighLevel 1

/*****************************************************************************/
/* These global variables support communication and synchronization
   between tasks
*/

msg_box mbox_alarm;
sem_t synchro;
m_integer LvlWater, LvlMeth;

/*****************************************************************************/
/* WaterLevelMonitoring_Task is a periodic task, period = 250 ms. At
   each dispatch, it reads the HLS and LLS sensors.
   - If HLS is true, it sends "HighValue" to LvlWater m_integer
   - else, if LLS is false, it sends "LowValue" to LvlWater m_integer
*/
void WaterLevelMonitoring_Body(void) {
  int level = LowLevel;
	BYTE read;
		printf("water level start\n");
	
		if (ReadHLS()==1)
			MI_write(LvlWater, HighLevel);
		else if(ReadLLS()==0)
			MI_write (LvlWater, LowLevel);
	printf("water level finish\n");
}

/*****************************************************************************/
/* MethaneMonitoring_Task is a periodic task, period = 100 ms. At each
   dispatch, it reads the MS sensor. Depending on the methane level
   (constant MS_L1 and MS_L2), it sends either normal, Alert1 or
   Alert2 to LvlMeth. At the end of the dispatch, it triggers the
   synchronization (semaphore synchro).
*/

void MethaneMonitoring_Body (void) {
	printf("methane level start\n");
  int level = Normal;
  BYTE MS=ReadMS();
		if (MS<MS_L1 && MS<MS_L2)
		{
				MI_write(LvlMeth, Normal);
		}
		else if(MS>=MS_L1 && MS<MS_L2)
		{
				MI_write(LvlMeth, Alarm1);
		}
		else if(MS>=MS_L2 )
		{	
			MI_write(LvlMeth, Alarm2);
		}

  sem_post(&synchro);
	printf("methane level finish\n");
}

/*****************************************************************************/
/* PumpCtrl_Task is a sporadic task, it is triggered by a synchronization
   semaphore, upon completion of the MethaneMonitoring task. This task
   triggers the alarm logic, and the pump.
   - if the alarm level is different from Normal, it sends the value 1
     to the mbox_alarm message box, otherwise it sends 0;
   - if the alarm level is Alarm2 then the pump is deactivated (cmd =
     0 sent to CmdPump); else, {if the water level is high, then the
     pump is activated (cmd = 1 sent to CmdPump), else if the water
     level is low, then the pump is deactivate, otherwise the pump is
     left off.}
*/

void *PumpCtrl_Body(void *no_argument) {
	
  int niveau_eau, niveau_alarme, alarme;
  int cmd;
  for (;;) {
	  printf("Pump control start\n");
		sem_wait(&synchro);
		niveau_alarme=MI_read(LvlMeth);
	niveau_eau=MI_read(LvlWater);
		if (niveau_alarme!=Normal)
			alarme=HighLevel;
		else
			alarme=LowLevel;
		
			
		if (niveau_alarme==Alarm2)
		{		
					cmd=0;//deactivate the pump
					//CmdPump((char)cmd);
		}
		else
		{
			
			if (niveau_eau==HighLevel)
			{		cmd=1;//activate the pump
					//CmdPump((char)cmd);
			}
			else
			{
				cmd=0;//deactivate the pump
				//CmdPump((char)cmd);
			}
		}
		CmdPump(cmd);
		int p = msg_box_send (mbox_alarm,(char*) &alarme);
		  printf("Pump control finish\n");
  }

}

/*****************************************************************************/
/* CmdAlarm_Task is a sporadic task, it waits on a message from
   mbox_alarm, and calls CmdAlarm with the value read.
*/

void *CmdAlarm_Body() {
	
  int value;
  for (;;) {
	  printf("Command alarm start\n");
    msg_box_receive(mbox_alarm,(char*)&value);
    CmdAlarm(value);
    printf("Command alarm finish\n");
  }
      
     
}

/*****************************************************************************/
#ifdef RTEMS
void *POSIX_Init() {
#else
int main(void) {
#endif /* RTEMS */

  pthread_t T3,T4;
  printf ("START\n");

  InitSimu(); /* Initialize simulator */

  /* Initialize communication and synchronization primitives */
  mbox_alarm =msg_box_init (sizeof (char));
  sem_init(&synchro,0,0) ;
  LvlWater =MI_init (98);
  LvlMeth =MI_init (97);

	
	//CHECK_NZ(pthread_create(&tid, &tattr,periodic_task_body, parameters));

  /* Create task WaterLevelMonitoring_Task */
  struct timespec period_water_level;
  period_water_level.tv_nsec = 250 * 1000 * 1000;
  period_water_level.tv_sec  = 0 ;
  create_periodic_task (period_water_level,10, WaterLevelMonitoring_Body);
	 
	 
	 
	 
  /* Create task MethaneMonitoring_Task */
  struct timespec period_meth_level;
  period_meth_level.tv_nsec = 100 * 1000 * 1000;
  period_meth_level.tv_sec  = 0 ;
	 create_periodic_task (period_meth_level, 27,MethaneMonitoring_Body);
	 
	 

  /* Create task PumpCtrl_Task */
	 //create_periodic_task (period_pump_ctrl, PumpCtrl_Body);
	 
	 pthread_attr_t tattr1;
	struct sched_param param1;
	CHECK_NZ(pthread_attr_init(&tattr1));
	
	CHECK_NZ(pthread_attr_getschedparam(&tattr1, &param1));
	
	param1.sched_priority=70;
	
	CHECK_NZ(pthread_attr_setinheritsched(&tattr1,PTHREAD_EXPLICIT_SCHED));
	CHECK_NZ(pthread_attr_setschedpolicy(&tattr1, SCHED_FIFO));
	CHECK_NZ(pthread_attr_setschedparam(&tattr1, &param1));
	pthread_create(&T3 , &tattr1, PumpCtrl_Body , NULL) ;

  /* Create task CmdAlarm_Task */
	// create_periodic_task (period_cmd_alarm, CmdAlarm_Body);
	
	pthread_attr_t tattr2;
	struct sched_param param2;
	CHECK_NZ(pthread_attr_init(&tattr2));
	
	CHECK_NZ(pthread_attr_getschedparam(&tattr2, &param2));
	
	param2.sched_priority=80;
	
	CHECK_NZ(pthread_attr_setinheritsched(&tattr2,PTHREAD_EXPLICIT_SCHED));
	CHECK_NZ(pthread_attr_setschedpolicy(&tattr2, SCHED_FIFO));
	CHECK_NZ(pthread_attr_setschedparam(&tattr2, &param2));
	pthread_create(&T4 , &tattr2, CmdAlarm_Body , NULL) ;
  
  

  pthread_join(T3,0);
  pthread_join(T4,0);
  

#ifndef RTEMS
  return 0;
#else
  return NULL;
#endif
}

#ifdef RTEMS
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_INIT

#include <rtems/confdefs.h>
#endif /* RTEMS */
