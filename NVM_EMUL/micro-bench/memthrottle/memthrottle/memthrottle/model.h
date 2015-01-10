#ifndef _MODEL_H
#define _MODEL_H

#include <sys/types.h>

#define CPUFREQ 3500

#define HRTIME_NS2CYCLE(__ns) (((__ns) * 1LLU * CPUFREQ) / 1000)
#define HRTIME_CYCLE2NS(__cycles) (((__cycles) * 1LLU * 1000) / CPUFREQ)

void emulate_latency_ns(int ns);

static inline void asm_cpuid() {
        asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}

static inline void asm_mfence(void)
{
        __asm__ __volatile__ ("mfence");
}

#if defined(__i386__)

static inline unsigned long long hrtime_cycles(void)
{
        unsigned long long int x;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
        return x;
}

#elif defined(__x86_64__)

typedef unsigned long long hrtime_t;

static inline unsigned long long hrtime_cycles(void)
{
        unsigned hi, lo;
        __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#else
#error "What architecture is this???"
#endif




#endif
