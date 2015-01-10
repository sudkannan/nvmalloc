#ifndef _MODEL_H
#define _MODEL_H

#include <sys/types.h>

#define CPUFREQ 3500

#define HRTIME_NS2CYCLE(__ns) (((__ns) * 1LLU * CPUFREQ) / 1000)
#define HRTIME_CYCLE2NS(__cycles) (((__cycles) * 1LLU * 1000) / CPUFREQ)

void emulate_latency_ns(int ns);
void *nvm_memcpy(void *dest, const void *src, size_t n);

#endif
