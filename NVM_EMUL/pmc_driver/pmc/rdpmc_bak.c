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
#include "rdpmc.h"

#define PMC_USER_MASK    0x00010000
#define PMC_OS_MASK      0x00020000
#define PMC_ENABLE_MASK  0x00400000

#define SYS_GETTSC 300

#define CTRL_SET_ENABLE(val) (val |= 1 << 20)
#define CTRL_SET_USR(val, u) (val |= ((u & 1) << 16))
#define CTRL_SET_KERN(val, k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT(val, e) (val |= e)



typedef unsigned long long hrtime_t;

static void con() __attribute__((constructor));
static inline unsigned long long hrtime_cycles(void);

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
 *          * so a serializing instruction is usually used to ensure previous
 *                   * instructions have completed. RDTSCP does not require this.
 *                            */
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

void con() {
    printf("I'm a constructor\n");
    emulate_latency_ns(1000);
    printf("Waited for 1000ns\n");
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
		return -1;
	}
	for(i=0; i<count; i++)
		result = ioctl(fd, 10+i, eventId[i] | PMC_USER_MASK | PMC_OS_MASK | PMC_ENABLE_MASK);

	close(fd);
}

long long pmc_get_event(int reg)
{
	return rdpmc(reg);
}

