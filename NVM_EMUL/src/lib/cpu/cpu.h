#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>

#define MAX_THROTTLE_VALUE 1023

int set_throttle_register(int node, uint64_t val);

typedef enum {
    THROTTLE_DDR_ACT = 0,
    THROTTLE_DDR_READ,
    THROTTLE_DDR_WRITE
} throttle_type_t;

typedef struct {
    const char* vendor_name;
    const char* uarch;
    const char* brand_name;
    const char* brand_processor_number;
} cpu_desc_t;

typedef struct {
    cpu_desc_t desc;
    int (*set_throttle_register)(int bus_id, throttle_type_t throttle_type, uint16_t val);
    int (*get_throttle_register)(int bus_id, throttle_type_t throttle_type, uint16_t* val);

} cpu_model_t;


cpu_model_t* cpu_model();

#endif /* __CPU_H */
