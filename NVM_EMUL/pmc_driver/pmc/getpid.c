#include <stdio.h>
#include <stdint.h>
     
typedef uint64_t hrtime_t;

static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

                                                                           
int main(int argc, char** argv)
{
	int ret;
	int i;
	int loop = atol(argv[1]);
	hrtime_t hr_start;
	hrtime_t hr_stop;
	
	for (i = 0; i < loop; i++)
	{
	hr_start = rdtsc();
	{
		ret = syscall(39);
	}
	hr_stop = rdtsc();
	printf("%llu\n", (hr_stop-hr_start)/(loop));
	}
	return ret;
}	
