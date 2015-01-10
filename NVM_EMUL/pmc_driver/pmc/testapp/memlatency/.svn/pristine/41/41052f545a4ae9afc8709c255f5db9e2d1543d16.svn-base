#include <stdio.h> 
#include <time.h>
#include <iostream>
#include <vector>
#include <algorithm>

#define KB 1024 // 1 KB is 1024 bytes
#define MB 1024 * KB // 1 MB is 1024 KB
#define SIZE 8 * MB // Size of the data array
#define REPS 512 * MB // times to access/modify memory (MB/KB just used as millions/thousands multiplier)
#define MAX_STRIDE 512 // in bytes, should be multiple of 4 (sizeof(int))
#define TIMES 6 // times to run to get median

long long wall_clock_time();
 
int main() {
	using namespace std;
	long long start, end;
	int lengthMod;
	float totalTime;
	int tmp = 0, median, medianIndex = TIMES / 2;
	vector<float> times(TIMES, 1.0000); 

	int *data = new int[SIZE/sizeof(int)]; 
	lengthMod = SIZE/sizeof(int) - 1;

	// repeatedly access/modify data, varying the STRIDE (i)
	for (int i = 4; i <= MAX_STRIDE/sizeof(int); i*=2)  {
		totalTime = 0;
		for (int j = 0; j < TIMES; j++) {
			start = wall_clock_time();
			for (unsigned int k = 0; k < REPS; k++) {
				// repeatedly read the data in strides (`i`)
				tmp += data[(k * i) & lengthMod];
			}
			end = wall_clock_time();
			times[j] = ((float)(end - start))/1000000000;
		}
		// we can expect a spike in time (more cache miss) when it exceeds the line size
		nth_element(times.begin(), times.begin() + medianIndex, times.end());
		printf("%d, %1.2f \n", (int)(i * sizeof(int)), times[medianIndex]);
	}

	// ensure tmp is used to prevent over-optimization
	FILE *debug = fopen("/dev/null", "w");
	fprintf(debug, "%d", tmp);

	// cleanup
	delete[] data;
}

/*******************************************************
 * Helpers
 ******************************************************/
long long wall_clock_time() {
	#ifdef __linux__
		struct timespec tp;
		clock_gettime(CLOCK_REALTIME, &tp);
		return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
	#else 
	#warning "Your timer resolution may be too low. Compile on linux"
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
	#endif
}
