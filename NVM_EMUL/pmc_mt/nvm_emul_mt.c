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
#include <limits.h>
#include "rdpmc.h"

#define THREAD_MAX 1000
#define MAX_REG 4

typedef unsigned long long hrtime_t;

static long coreId = -1;
static pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct _Thread_Control_Block{
	pthread_t thread_id;
	int valid;
	int inCS;
	unsigned long long epoch_len;
	unsigned long long pmu_reg[MAX_REG];
}Thread_Control_Block;

Thread_Control_Block TCB[THREAD_MAX];
unsigned long curr_thread_count;

int nvm_register_tds(pthread_t thread_id, long thread_idx, void* reserved); 
static inline unsigned long long hrtime_cycles(void);

int P(int semID) 
{
	struct sembuf buff ;
	buff.sem_num = 0 ;
	buff.sem_op = -1 ;
	buff.sem_flg = 0 ;
	if(semop(semID, &buff, 1) == -1)
	{
		perror("CFSCHED semop P operation error") ;
		return 0 ;
	}
	return -1 ;
}

int V(int semID) 
{
	struct sembuf buff ;
	buff.sem_num = 0 ;
	buff.sem_op = 1 ;
	buff.sem_flg = 0 ;
	if(semop(semID, &buff, 1) == -1)
	{
		perror("NVM semop V operation error") ;
		return 0 ;
	}
	return -1 ;
}

void signal_block_all()
{
	int i;
	sigset_t set;
	union sigval data;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);
	sigaddset(&set, SIGALRM);

		
}

static inline
void
emulate_latency_ns(int ns)
{
    hrtime_t cycles;
    hrtime_t start;
    hrtime_t stop;

    start = hrtime_cycles();
    cycles = HRTIME_NS2CYCLE(ns);

    do {
        /* RDTSC doesn't necessarily wait for previous instructions to complete
         * so a serializing instruction is usually used to ensure previous
         * instructions have completed. RDTSCP does not require this.
         */
        stop = hrtime_cycles();
    } while (stop - start < cycles);
}

static inline unsigned long long hrtime_cycles(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

int findThreadIdx(pthread_t thread_id)
{
	int i = 0;
	long array_len = curr_thread_count;

	for(i = 0; i < array_len; i++)
		if (TCB[i].valid  == 1 && thread_id == TCB[i].thread_id)
			return i;
	return -1;
}

void delay_handler (int signum)
{
	unsigned long long stop0, stop1, stop2, stop3, start0, start1, start2, start3;
	int thread_idx;

	fprintf(stderr,"In thread:%llu %s called\n", pthread_self(), __func__);
	/* read rdpmc counter again*/
        stop0 = rdpmc(0);
        stop1 = rdpmc(1);
        stop2 = rdpmc(2);
        stop3 = rdpmc(3);

	thread_idx = findThreadIdx(pthread_self());

	start0 = TCB[thread_idx].pmu_reg[0];
	start1 = TCB[thread_idx].pmu_reg[1];
	start2 = TCB[thread_idx].pmu_reg[2];
	start3 = TCB[thread_idx].pmu_reg[3];

	/* calculate LDM stall cycles */
        long long ldm_stall = (stop0-start0)*((7*(stop1-start1))/(7*(stop1-start1) + stop3-start3));
        long long delay = (ldm_stall*60)/210;

	emulate_latency_ns(delay);

	/* start reading rdpmc counter */
	TCB[thread_idx].pmu_reg[0] = rdpmc(0);
	TCB[thread_idx].pmu_reg[1] = rdpmc(1);
	TCB[thread_idx].pmu_reg[2] = rdpmc(2);
	TCB[thread_idx].pmu_reg[3] = rdpmc(3);
}

int start_perf_monitoring(pthread_t thread_id, long thread_idx){

	struct itimerval timer;

	/* Initializing the rdpmc library */
	int eventId[4] = {0, 0, 0, 0};

	// Event CYCLE_ACTIVITY:STALLS_L2_PENDING
	eventId[0] = 0x55305a3;

	// Event MEM_LOAD_UOPS_MISC_RETIRED:LLC_MISS
	eventId[1] = 0x5302d4;

	//Event MEM_LOAD_UOPS_RETIRED:L3_HIT
	eventId[2] = 0x5304d1;

	//Event INSTRUCTION_RETIRED
	eventId[3] = 0x5300c0;

	pmc_init(eventId, 4);  

	/* start reading rdpmc counter */
	TCB[thread_idx].pmu_reg[0] = rdpmc(0);
	TCB[thread_idx].pmu_reg[1] = rdpmc(1);
	TCB[thread_idx].pmu_reg[2] = rdpmc(2);
	TCB[thread_idx].pmu_reg[3] = rdpmc(3);
}

int nvm_thread_register()
{
	pthread_t thread_id = pthread_self();

	long  thread_idx; 
	pthread_mutex_lock(&myMutex);
	coreId++;
	curr_thread_count++;
	thread_idx = curr_thread_count - 1;
	pthread_mutex_unlock(&myMutex);

	cpu_set_t my_set;        
	CPU_ZERO(&my_set);   
	CPU_SET(coreId, &my_set);

	fprintf(stderr, "pinning thread:%llu to core:%llu\n", thread_id, coreId);
	//pthread_setaffinity_np(0, sizeof(cpu_set_t), &my_set);

	
	nvm_register_tds(thread_id, thread_idx, NULL);

	/* Install delay handler as the signal handler for SIGUSR1. */
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &delay_handler;
	sigaction (SIGUSR1, &sa, NULL);

	start_perf_monitoring(thread_id, thread_idx);

	//unmask the user defined signal
	sigset_t set;	
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	return 0;
}

int nvm_thread_deregister()
{

	return 0;
}

int nvm_register_tds(pthread_t thread_id, long thread_idx, void *reserved)
{
	TCB[thread_idx].valid = 1;
	TCB[thread_idx].inCS = 0;
	TCB[thread_idx].epoch_len = 10000;
	TCB[thread_idx].thread_id = thread_id;
	return 0;
}

unsigned long long find_min(long array_len)
{
	int i;
	unsigned long long min_epoch = ULLONG_MAX;

	for(i = 0; i < array_len; i++){
		if (TCB[i].valid  == 1 && !TCB[i].inCS && TCB[i].epoch_len < min_epoch)
			min_epoch = TCB[i].epoch_len;
	}
	return min_epoch;
}

int signal_threads(unsigned long long min_epoch, long array_len)
{
	int i, signo;

	signo = SIGUSR1;
	for(i = 0; i < array_len; i++)
		if (TCB[i].valid  == 1 && !TCB[i].inCS && TCB[i].epoch_len == min_epoch)
			pthread_kill(TCB[i].thread_id, signo);	
}

void timer_handler (int signum)
{
	unsigned long long min_epoch;
	long array_len;

	do{
		array_len = curr_thread_count;   
		min_epoch = find_min(array_len);
		signal_threads(min_epoch, array_len);
	}while(min_epoch == ULLONG_MAX);

	//fprintf(stderr, "%s of controller thread\n", __func__);
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = min_epoch;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	/* Start a real timer */
	setitimer (ITIMER_REAL, &timer, NULL);
}

void* nvm_control_thread(void *attr)
{
	/* Install timer_handler as the signal handler for SIGALRM. */
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGALRM, &sa, NULL);

	//unmask the user defined signal
	sigset_t set;	
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	pthread_kill(pthread_self(), SIGALRM);

	while(1);

	return NULL;
}

int nvm_init()
{
	pthread_t control_thid;
	signal_block_all();

	pthread_create(&control_thid, NULL, nvm_control_thread, NULL);

	pthread_detach(control_thid);
	//pthread_join(control_thid, NULL);
}
