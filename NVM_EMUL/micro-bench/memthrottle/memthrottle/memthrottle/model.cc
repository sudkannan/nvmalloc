#include "model.h"
#include <stdlib.h>
#include <string.h>

void
emulate_latency_ns(int ns)
{
        hrtime_t cycles;
        hrtime_t start;
        hrtime_t stop;

        if (ns==0) {
                return;
        }

        start = hrtime_cycles();
        cycles = HRTIME_NS2CYCLE(ns);

        do {
                /* RDTSC doesn't necessarily wait for previous instructions to complete 
                 * so a serializing instruction is usually used to ensure previous 
                 * instructions have completed. However, in our case this is a desirable
                 * property since we want to overlap the latency we emulate with the
                 * actual latency of the emulated instruction. 
                 */
                stop = hrtime_cycles();
                asm_cpuid();
        } while (stop - start < cycles);
}
