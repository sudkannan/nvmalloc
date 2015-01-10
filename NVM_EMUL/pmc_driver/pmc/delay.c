#define _HRTIME_CPUFREQ 2500 /* MHz */ /* you can read it from /proc/cpuinfo instead */
#define HRTIME_NS2CYCLE(__ns) ((__ns) * _HRTIME_CPUFREQ / 1000)
 
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
