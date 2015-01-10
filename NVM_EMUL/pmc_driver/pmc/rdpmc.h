#define CTRL_SET_ENABLE(val) (val |= 1 << 20)
#define CTRL_SET_INT(val) (val |= 1 << 20)
#define CTRL_SET_USR(val, u) (val |= ((u & 1) << 16))
#define CTRL_SET_KERN(val, k) (val |= ((k & 1) << 17))
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT(val, e) (val |= e)
#define CTRL_SET_UM(val, m) (val |= (m << 8))
#define CTRL_SET_EVENT(val, e) (val |= e)
#define CTRL_SET_INV(val) (val |= (1 << 23))
#define CTRL_SET_CM(val, c) (val |= (c << 24))

#define _HRTIME_CPUFREQ 2100 /* MHz */ /* you can read it from /proc/cpuinfo instead */
#define HRTIME_NS2CYCLE(__ns) ((__ns) * _HRTIME_CPUFREQ / 1000)

void cpuid();
long long rdpmc(int counter);
int rdpmc32(int counter);
int pmc_init(int *eventId, int count);
int start_perf_monitoring();
int stop_perf_monitoring();
void constructor();
