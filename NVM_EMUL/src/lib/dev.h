#ifndef __DEVICE_DRIVER_API_H
#define __DEVICE_DRIVER_API_H

#include <stdint.h>

int set_counter(unsigned int counter_id, unsigned int event_id);
int set_pci(unsigned bus_id, unsigned int device_id, unsigned int function_id, unsigned int offset, uint16_t val);
int get_pci(unsigned bus_id, unsigned int device_id, unsigned int function_id, unsigned int offset, uint16_t* val);

#endif /* __DEVICE_DRIVER_API_H */
