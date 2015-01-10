#ifndef __IOCTL_QUERY_H
#define __IOCTL_QUERY_H

#include <linux/ioctl.h>

#define MYDEV_MAGIC (0xAA)

typedef struct { 
    unsigned int counter_id;
    unsigned int event_id;
} ioctl_query_setcounter_t;

typedef struct { 
    unsigned int bus_id;
    unsigned int device_id;
    unsigned int function_id;
    unsigned int offset;
    unsigned int val;
} ioctl_query_setgetpci_t;

#define IOCTL_SETCOUNTER _IOR(MYDEV_MAGIC, 0, ioctl_query_setcounter_t *) 
#define IOCTL_SETPCI     _IOR(MYDEV_MAGIC, 1, ioctl_query_setgetpci_t *) 
#define IOCTL_GETPCI     _IOWR(MYDEV_MAGIC, 2, ioctl_query_setgetpci_t *) 


#endif /* __IOCTL_QUERY_H */
