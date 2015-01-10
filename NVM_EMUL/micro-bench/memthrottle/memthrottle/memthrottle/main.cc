#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include "model.h"

const int PAGESZ = 4096;
int       bwtarget;
int socketid;

void * sse_memcpy(void * vto, const void * vfrom, size_t len);


void* throttle_thread(void*) 
{
	int      i;
	size_t   size;
	double   throughput;
	hrtime_t start;
	hrtime_t stop;
	hrtime_t duration;
	char*    src = NULL;
	char*    dst = NULL;
	int      extra_latency = 0;

	if (posix_memalign((void**) &src, PAGESZ, 1024*1024) != 0) {
		std::cerr << "Error: Memory allocation" << std::endl;
		return NULL;
	}
	if (posix_memalign((void**) &dst, PAGESZ, 1024*1024) != 0) {
		std::cerr << "Error: Memory allocation" << std::endl;
		return NULL;
	}

	i = 0;
	do {
		size = 128*1024;
		start = hrtime_cycles();
		sse_memcpy(dst, src, size);
		emulate_latency_ns(extra_latency);
		stop = hrtime_cycles();
		duration = stop - start;
		throughput = 1000*((double) size) / ((double) HRTIME_CYCLE2NS(duration));
		if (throughput > bwtarget && (throughput - bwtarget) > 1000) {
			extra_latency += 10;
		}
		if (i++ % 100000 == 0) {
		  std::cout << "[S:" << socketid << "T:" << pthread_self() << "] bandwidth = " << throughput << " MB/s\n" << std::endl;
		}
	} while (1);
	return NULL;
}


int main(int argc, char** argv)
{
	int i;
	int nthreads;
	pthread_t threads[128];

	if (argc < 3) {
		std::cerr << argv[0] << " NTHREADS BW_TARGET MB/s <SOCKET ID>\n" << std::endl;
		exit(0);
	}
	nthreads = atoi(argv[1]);
	bwtarget = atoi(argv[2]);

	if (argc == 4) {
	  socketid = atoi(argv[3]);
	} else {
	  socketid = 0;
	}

	for (i=0; i<nthreads; i++) {
		pthread_create(&threads[i], NULL, throttle_thread, NULL);
	}
	for (i=0; i<nthreads; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}
