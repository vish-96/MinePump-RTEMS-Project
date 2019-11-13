#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#include "periodic_task.h"
#include "utils.h"


/*****************************************************************************/
void *periodic_task_body (void *parameters);

void *periodic_task_body (void *parameters) {
  struct timespec trigger;                      /* Stores next dispatch time */
  struct timespec period;                              /* Period of the task */
  sem_t timer;
  task_parameters *my_parameters = (task_parameters *) parameters;

  period = (*my_parameters).period;
 
	sem_init(&timer , 0 , 0 ) ;
	clock_gettime (CLOCK_REALTIME,&trigger) ;
	
	
  //printf("i m inside task");
  for ( ; ; )  {
	//  job
	my_parameters->job();
	add_timespec(&trigger,&trigger ,&period ) ;
	sem_timedwait(&timer ,&trigger) ;
	}
}

/*****************************************************************************/
void create_periodic_task (struct timespec period, int priority,void (*job) (void) ) {
  pthread_t tid;
 
  task_parameters *parameters = malloc (sizeof (task_parameters));

  parameters->period = period;
  parameters->job = job;
	
	pthread_attr_t tattr;
	struct sched_param param;
	CHECK_NZ(pthread_attr_init(&tattr));
	
	CHECK_NZ(pthread_attr_getschedparam(&tattr, &param));
	
	param.sched_priority=priority;
	
	CHECK_NZ(pthread_attr_setinheritsched(&tattr,PTHREAD_EXPLICIT_SCHED));
	
	CHECK_NZ(pthread_attr_setschedpolicy(&tattr, SCHED_FIFO));
	
	CHECK_NZ(pthread_attr_setschedparam(&tattr, &param));
	
	CHECK_NZ(pthread_create(&tid, &tattr,periodic_task_body, parameters));
	
	//CHECK_NZ(pthread_create(&tid , NULL, periodic_task_body, parameters));

	//pthread_join(tid,NULL);
}
/*****************************************************************************/
#ifdef __TEST__
void dummy (void) {
  printf ("o< \n");
}

int main (void) {
  struct timespec period;
  period.tv_nsec = 250 * 1000 * 1000;
  period.tv_sec  = 0 ;
  create_periodic_task (period, dummy);
  printf ("Task created\n");
  while (1);
  return 0;
}
#endif /* __TEST__ */
