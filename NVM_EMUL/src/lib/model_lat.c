#include "cpu/cpu.h"
#include "config.h"

int model_lat_init(config_t* cfg, cpu_model_t* cpu)
{
    int val;
    __cconfig_lookup_int(cfg, "latency.read", &val);

    printf("latency.read = %d\n", val);



}
