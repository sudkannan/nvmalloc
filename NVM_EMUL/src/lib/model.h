#ifndef __MODEL_H
#define __MODEL_H

#include "config.h"
#include "cpu/cpu.h"

int model_bw_init(config_t* cfg, cpu_model_t* cpu);
int model_lat_init(config_t* cfg, cpu_model_t* cpu);

int set_write_bw(config_t* cfg, cpu_model_t* cpu);
int set_read_bw(config_t* cfg, cpu_model_t* cpu);

#endif /* __MODEL_H */
