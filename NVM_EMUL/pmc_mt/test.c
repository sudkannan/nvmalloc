#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>
#include <pthread.h>

void *func(void* args)
{
	fprintf(stderr, "registering thread: %llu\n", pthread_self());
	nvm_thread_register();
	fprintf(stderr,"registration over\n");
	while(1);
}

int main()
{
	int thread_count = 2;
	int i;
	pthread_t thIDs[100];

	nvm_init();

	for (i = 0; i< thread_count; i++)	
		pthread_create(&thIDs[i], NULL, func, NULL);

	for(i = 0 ; i < thread_count ; i++)
		pthread_join(thIDs[i], NULL);

	return 0;
}
