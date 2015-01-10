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
#include "count.h"

#define PMC_USER_MASK    0x00010000
#define PMC_OS_MASK      0x00020000
#define PMC_ENABLE_MASK  0x00400000

#define SYS_GETTSC 300

#if 0
#define CTRL_SET_ENABLE(val) (val |= 1 << 20)
#define CTRL_SET_USR(val, u) (val |= ((u & 1) << 16))
#define CTRL_SET_KERN(val, k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT(val, e) (val |= e)
#endif

typedef uint64_t hrtime_t;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
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
	for(i=0; i<count; i++) {
		fprintf(stderr,"calling ioctl\n");
		result = ioctl(fd, 0, eventId[i] | PMC_USER_MASK | PMC_OS_MASK | PMC_ENABLE_MASK);
	}

	close(fd);
}

long long pmc_get_event(int reg)
{
	return rdpmc(reg);
}

int count(int eventId1, int eventId2, long long *res1, long long *res2)
{
	int fd;
	int result;
	char buf[512];
	hrtime_t hr_start;
	hrtime_t hr_stop;
	struct timeval tv;
	long ret;

	fd = open("/dev/pmc", O_RDONLY);
	if (fd < 0) {
		strerror_r(errno, buf, 512);
		perror("can't open /dev/pmc - is pmc driver installed?");
		perror(buf);
		return -1;
	}

	fprintf(stderr,"calling ioctl\n");
	result = ioctl(fd, 0, eventId1 | PMC_USER_MASK | PMC_OS_MASK | PMC_ENABLE_MASK);
	fprintf(stderr,"calling ioctl\n");
	result = ioctl(fd, 1, eventId2 | PMC_USER_MASK | PMC_OS_MASK | PMC_ENABLE_MASK);

	close(fd);

	int i,j;
	int iter = 1;
	int iter_j = 10;
	int sum;

	
	long long start = rdpmc(0x0);
	cpuid();
	for (i=0; i<1000; i++) {
		sum+=i;
	}
	cpuid();
	long long stop = rdpmc(0x0);
	printf("sum=%d\n", i);
	printf("%lld\n", start);
	printf("%lld\n", stop);

	return 0;
}


main()
{

	long long res1;
	long long res2;
	int event;

	CTRL_SET_EVENT(event, 0x3c);
	CTRL_SET_UM(event, 0x0);

	count(event, 0x3c, &res1, &res2);
}
