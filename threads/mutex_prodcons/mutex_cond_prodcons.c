/*
 * Requirement of the project
 * -> Create a Message queue having objects 
 * -> Two threads accessing the message queue 
 *  	->one thread fills the queue {producer}
 *  	->the other empty's the queue {consumer}
 *  	->use two conditional variable to synchronise the producer and consumer
 *  	  along with a mutex 
 *  	->also set the priorities and attributes for the threads
 *  ->the message Queue must have a head and objects maintained in singly linked list
 *  ->the thread must be cleared in the main using pthreadjoin
 *  */


/*Headers to be included*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


/*Private objects required*/
pthread_mutex_t mutex;	//mutex

pthread_cond_t enque_var,deque_var;	//conditional variables for enque and deque operations in the 
					//msg. queue

pthread_t prod_thread,cons_thread;	//thread ids

/*Creating structure objects{} to be placed inside the message queue*/
typedef struct object{
	char info[1024];
	struct object *next;
}job; 

struct obj_head{
	unsigned int job_count;
	struct object *next;
}job_head;


/*thread methods*/
void *prod_method(void *arg){
	job *new_job;
	int ret;

	printf("enter the list of jobs you want\n");
	
	while(1){
		printf("enter the job\n");
		ret = pthread_mutex_lock(&mutex);	//locking the mutex 

		/*condition check for waiting in the enqueue variable*/
		while(job_head.job_count >= 5)	//blocks here until the condition in true
			pthread_cond_wait(&enque_var,&mutex);	//while blocking releases the mutex lock
								//for other threads in this case cons_thread
		
		/*once the above condition gets false then this thread will be unblocked
		 * and the following will be processed further*/
		new_job = (job*) malloc(sizeof(job));	//create a new object for job
		read(STDIN_FILENO,new_job->info,1024);	//read from the user and fill the info
		printf("The job is enqueued\n");
		new_job->next = NULL;
		/*fitting the object inside the queue*/
		if(job_head.next==NULL){
			job_head.next = new_job;
		}else{
			job *temp = job_head.next;
			while(temp->next!=NULL)
				temp = temp->next;
			temp->next = new_job;
		}
		job_head.job_count++;
		
		/*again checking for the same condition check*/
		if(job_head.job_count >= 5)
			pthread_cond_signal(&deque_var);	//release the thread for dequeue variable

		pthread_mutex_unlock(&mutex);		//unlocking the mutex
	}
	pthread_exit(0);	//successful execution
}

void *cons_method(void *arg){
	printf("Listing and removing the queue objects\n");
	while(1){
		job *next_job;

		pthread_mutex_lock(&mutex);
		
		while(job_head.job_count<5)
			pthread_cond_wait(&deque_var,&mutex);
		while(1){
			if(job_head.next==NULL)
				break;
			else{
				next_job = job_head.next;
				job_head.next = next_job->next;
				printf("the job is dequeued\n");
				write(STDOUT_FILENO,next_job->info,1024);
				free(next_job);
			}	
		}

		pthread_cond_signal(&enque_var);

		pthread_mutex_unlock(&mutex);
	}
	pthread_exit(0);
}

/*main method*/
int main(){
	int ret;

	/*creating variables for attribute setup of the threads*/
	pthread_mutexattr_t mutex_attr;
	pthread_attr_t thread_attr;
	struct sched_param param1;		//used to set the scheduling priorities 
						//of the threads
	/*setting up the job_head1*/
	job_head.job_count = 0;
	job_head.next =NULL;

	/*initialising the default attributes for mutex*/
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_NORMAL);	//here the flag can be accordingly
								//refer man pages for the threads flags

	/*initialising the mutex with the above mutex attribute*/
	pthread_mutex_init(&mutex,&mutex_attr);	//the mutex is initialised in unlocked state with 
						//appropriate attributes
	
	/*initialising the conditional variables with default attributes*/
	pthread_cond_init(&enque_var,NULL);
	pthread_cond_init(&deque_var,NULL);

	/*setting up the attribute object for the threads*/
	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr,SCHED_FIFO);
	param1.sched_priority =10;	//initialising the parameter with the appropriate policy 
	/*setting the attribute object with the priority parameter*/
	pthread_attr_setschedparam(&thread_attr,&param1);

	/*creating the producer thread*/
	ret = pthread_create(&prod_thread,&thread_attr,prod_method,NULL);
	if(ret){
		perror("Error while creating thread\n");
		exit(1);
	}
	param1.sched_priority = 10;
	pthread_attr_setschedparam(&thread_attr,&param1);
	ret = pthread_create(&cons_thread,&thread_attr,cons_method,NULL);
	if(ret){
		perror("Error while creating thread\n");
		exit(1);
	}

	/*setting the attributes for the main thread*/
	param1.sched_priority = 15;
	pthread_setschedparam(pthread_self(),SCHED_FIFO,&param1);
	
	/*Cleaning of the threads */
	pthread_join(prod_thread,NULL);
	pthread_join(cons_thread,NULL);

	exit(0);
}
