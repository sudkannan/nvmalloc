#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

void printmsrs()
{
	char buf[512];
	int fd;
	int result;

	fd = open("/dev/pmc", O_RDONLY);
	if (fd < 0) {
		strerror_r(errno, buf, 512);
		perror("can't open /dev/pmc - is pmc driver installed?");
		perror(buf);
		return -1;
	}

	result = ioctl(fd, 2, 0);

	close(fd);
}

long long rdpmc(int counter) 
{
	unsigned eax;
	unsigned edx;
	unsigned long long r;

	__asm__ __volatile__ ("mov %2, %%ecx\n\t"
	                      "rdpmc\n\t"
	                      "mov %%eax, %0\n\t"
	                      "and $255, %%edx\n\t"
	                      "mov %%edx, %1\n\t"
	                      : "=m" (eax), "=m" (edx), "=m" (counter)
	                      : /* no inputs */
	                      : "eax", "ecx", "edx"); /* eax, ecx, edx clobbered */
	                      r = ((unsigned long long) edx << 32) | eax;
	return r;
}


static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

int
main(int argc, char **argv)
{
    struct perf_event_attr pe;
    long long count;
    int fd;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    printmsrs();

    printf("Measuring instruction count for this printf\n");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    printf("Used %lld instructions\n", count);

	printf("%lld\n", rdpmc(0));
    close(fd);
}
