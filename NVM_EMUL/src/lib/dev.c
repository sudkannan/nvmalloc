#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "dev/ioctl_query.h"
#include "error.h"
#include "dev.h"

int set_counter(unsigned int counter_id, unsigned int event_id)
{
	int fd; 
    int ret;

    ioctl_query_setcounter_t q;
	fd = open("/dev/nvmemul", O_RDONLY);
	if (fd < 0) {
		DBG_LOG(ERROR, "Can't open /dev/nvmemul - Is the NVM emulator device driver installed?");
		return E_ERROR;
	}
    q.counter_id = counter_id;
    q.event_id = event_id;
    if ((ret = ioctl(fd, IOCTL_SETCOUNTER, &q)) < 0) {
        return E_ERROR;
    }
	close(fd);
    return E_SUCCESS;
}


int set_pci(unsigned int bus_id, unsigned int device_id, unsigned int function_id, unsigned int offset, uint16_t val)
{
	int fd; 
    int ret;

    ioctl_query_setgetpci_t q;
	fd = open("/dev/nvmemul", O_RDONLY);
	if (fd < 0) {
		DBG_LOG(ERROR, "Can't open /dev/nvmemul - Is the NVM emulator device driver installed?");
		return E_ERROR;
	}
    q.bus_id = bus_id;
    q.device_id = device_id;
    q.function_id = function_id;
    q.offset = offset;
    q.val = val;
    if ((ret = ioctl(fd, IOCTL_SETPCI, &q)) < 0) {
        return E_ERROR;
    }
	close(fd);
    return E_SUCCESS;
}

int get_pci(unsigned int bus_id, unsigned int device_id, unsigned int function_id, unsigned int offset, uint16_t* val)
{
	int fd; 
    int ret;

    ioctl_query_setgetpci_t q;
	fd = open("/dev/nvmemul", O_RDWR);
	if (fd < 0) {
		DBG_LOG(ERROR, "Can't open /dev/nvmemul - Is the NVM emulator device driver installed?");
		return E_ERROR;
	}
    q.bus_id = bus_id;
    q.device_id = device_id;
    q.function_id = function_id;
    q.offset = offset;
    q.val = 0;
    if ((ret = ioctl(fd, IOCTL_GETPCI, &q)) < 0) {
        return E_ERROR;
    }
    *val = q.val;
	close(fd);
    return E_SUCCESS;
}


