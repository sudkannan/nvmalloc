#define _GNU_SOURCE 

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <pthread.h>

#include "rdpmc.h"

#include <sched.h>

#define PMC_USER_MASK    0x00010000
#define PMC_OS_MASK      0x00020000
#define PMC_ENABLE_MASK  0x00400000

#define SYS_GETTSC 300

#define CTRL_SET_ENABLE(val) (val |= 1 << 20)
#define CTRL_SET_USR(val, u) (val |= ((u & 1) << 16))
#define CTRL_SET_KERN(val, k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT(val, e) (val |= e)

static pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

typedef unsigned long long hrtime_t;
		
long long start0, start1, start2, start3, stop0, stop1, stop2, stop3;

static void con() __attribute__((constructor));
static inline unsigned long long hrtime_cycles(void);

static inline
void
emulate_latency(int cycles)
{
    hrtime_t start;
    hrtime_t stop;
 
    start = hrtime_cycles();   
    //printf("start:%llu\n", start);
    //printf("cycles:%llu\n", cycles);

    do {
        /* RDTSC doesn't necessarily wait for previous instructions to complete
         * so a serializing instruction is usually used to ensure previous
         * instructions have completed. RDTSCP does not require this.
         */
        stop = hrtime_cycles();
    } while (stop - start < cycles);
}   
    
/* replace rdtsc with rdtscp if running on a processor that supports the rdtscp instruction */
static inline unsigned long long hrtime_cycles(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}   

void delay_handler (int signum)
{
	static int count = 0;

	pthread_mutex_lock(&myMutex);	

	//fprintf (stderr, "timer expired %d times\n", ++count);
	/* read rdpmc counter again*/
	stop0 = rdpmc(0);
	stop1 = rdpmc(1);
	//stop2 = rdpmc(2);
	//stop3 = rdpmc(0x3);

	//fprintf(stderr,"stop0-start0: %llu\n stop1: %llu\tstop2: %llu\tstop3: %llu\n", 
	//	stop0-start0, stop1, stop2, stop3); 
	
	//double l2_pending = stop0-start0;
	double LLC_Miss = stop1-start1;
	//double LLC_Hit = stop2-start2;
	//double ldm_stall_double = l2_pending*((7*LLC_Miss)/(7*LLC_Miss+LLC_Hit));  
	//long long ldm_stall = (long long) ldm_stall_double;  //l2_pending;
	//long long ldm_stall = (long long) ldm_stall_double; 
	//long long delay = (ldm_stall*1000)/60; //in processor cycles
	 long long delay = LLC_Miss * (500-60);
	//printf("Stall_l2_pending: %llu\nLLC_Miss: %llu\nLLC_Hit: %llu\nInstructions: %llu ldm_stall: %llu\n", stop0-start0, stop1-start1, stop2-start2, stop3-start3, ldm_stall); 

	//delay = 10000000;
	//printf("Wait for %lld cycles\n", delay);
	emulate_latency(delay);

	/* start reading rdpmc counter */
	start0 = rdpmc(0);
	start1 = rdpmc(1);
	//start2 = rdpmc(2);
	//start3 = rdpmc(0x3);
	pthread_mutex_unlock(&myMutex);
}

void con() {

    //printf("I'm a constructor\n");
    
    struct sigaction sa;

    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(0, &my_set);     /* set the bit that represents core 0. */
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set); /* Set affinity of tihs process to */
                                                  /* the defined mask */

    /* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &delay_handler;
    sigaction (SIGPROF, &sa, NULL);

    start_perf_monitoring();

    /* Do busy work. */
    //while (1);
}

int start_perf_monitoring(){

    struct itimerval timer;

    /* Initializing the rdpmc library */
    int eventId[4] = {0, 0, 0, 0};

    //Event INSTRUCTION_RETIRED
    //CTRL_SET_EVENT(eventId[3], 0xb1);
    //CTRL_SET_UM(eventId[3], 0x01);
    //CTRL_SET_INT(eventId[3]);
    //CTRL_SET_ENABLE(eventId[3]);
    //CTRL_SET_INV(eventId[3]);
    //CTRL_SET_CM(eventId[3], 0x01);
    eventId[0] = 0x5300c0;

    // Event MEM_LOAD_RETIRED:LLC_MISS
    //CTRL_SET_EVENT(eventId[1], 0xd4);
    //CTRL_SET_UM(eventId[1], 0x02);
    //CTRL_SET_INT(eventId[1]);
    //CTRL_SET_ENABLE(eventId[1]);
    //eventId[1] = 0x5302cb;

    //Event MEM_LOAD_RETIRED:L2_LINE_IN:BOTH_CORES
    //CTRL_SET_EVENT(eventId[1], 0xd4);
    //CTRL_SET_UM(eventId[1], 0x02);
    //CTRL_SET_INT(eventId[1]);
    //CTRL_SET_ENABLE(eventId[1]);
    eventId[1] = 0x53f024;
	

    //Event MEM_LOAD_RETIRED:LLC_MISS
    //CTRL_SET_EVENT(eventId[2], 0xd1);
    //CTRL_SET_UM(eventId[2], 0x04);
    //CTRL_SET_INT(eventId[2]);
    //CTRL_SET_ENABLE(eventId[2]);
    eventId[2] = 0x5301cb;


    // Event CYCLE_ACTIVITY:STALLS_L2_PENDING
    //CTRL_SET_EVENT(eventId[0], 0xa3);
    //CTRL_SET_UM(eventId[0], 0x05);
    //CTRL_SET_INT(eventId[0]);
    //CTRL_SET_ENABLE(eventId[0]);
   eventId[3] = 0x5301cb;


    pmc_init(eventId, 2);

    /* start reading rdpmc counter */
    start0 = rdpmc(0);
    start1 = rdpmc(1);
    //start2 = rdpmc(2);
    //start3 = rdpmc(0x3);

    /* Configure the timer to expire after 10 msec... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000;
    /* ... and every 250 msec after that. */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    /* Start a virtual timer. It counts down whenever this process is
     executing. */
    setitimer (ITIMER_PROF, &timer, NULL);
    
    return 0;
}
 
int stop_perf_monitoring(){

    struct itimerval timer;

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    //Disabling the timer 
    setitimer (ITIMER_PROF, &timer, NULL);

    return 0;
}

static __inline__ long rdtsc_lo(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (long)lo);
}


void cpuid() {
    asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}



long long rdpmc(int counter) 
{
	unsigned eax;
	unsigned edx;
	unsigned long long r;

	__asm__ __volatile__ ("mov %2, %%ecx\n\t"
	                      "rdpmc\n\t"
	                      "mov %%eax, %0\n\t"
	                      "and $255, %%edx\n\t"
	                      "mov %%edx, %1\n\t"
	                      : "=m" (eax), "=m" (edx), "=m" (counter)
	                      : /* no inputs */
	                      : "eax", "ecx", "edx"); /* eax, ecx, edx clobbered */
	                      r = ((unsigned long long) edx << 32) | eax;
	return r;
}


int rdpmc32(int counter) {
	unsigned eax;
	
	__asm__ __volatile__ ("mov %1, %%ecx\n\t"
	                      "rdpmc\n\t"
	                      "mov %%eax, %0\n\t"
	                      : "=m" (eax), "=m" (counter)
	                      : /* no inputs */
	                      : "eax", "ecx", "edx"); /* eax, ecx, edx clobbered */
	return eax;
}

int pmc_init(int *eventId, int count)
{
	int fd, i, result; 
	char buf[512];
	fd = open("/dev/pmc", O_RDONLY);
	if (fd < 0) {
		strerror_r(errno, buf, 512);
		perror("can't open /dev/pmc - is pmc driver installed?");
		perror(buf);
		printf("/dev/pmc open failed\n");
		return -1;
	}
	for(i=0; i<count; i++) {
		//fprintf(stderr,"ioctl called \n");
		result = ioctl(fd, 10+i, eventId[i] | PMC_USER_MASK | PMC_OS_MASK | PMC_ENABLE_MASK);
	}

	close(fd);
}

long long pmc_get_event(int reg)
{
	return rdpmc(reg);
}

