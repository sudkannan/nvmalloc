#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
 
#define KB 1024
#define MB 1024 * 1024
 
int main() {
    unsigned int steps = 64 * 1024 * 1024;
    static char arr[64 * 1024 * 1024];
    char dummy;	
    int lengthMod;
    unsigned int i;
    double timeTaken;
    clock_t start;
    int sizes[] = { 
        2.5 * MB, 3 * MB, 3.5 * MB, 4 * MB,5*MB,6*MB,7*MB,
	8 * MB, 12 * MB, 16 * MB, 20 * MB, 24 * MB, 28 * MB, 32 * MB
    };
    int results[sizeof(sizes)/sizeof(int)];
    int s;
 
    // for each size to test for ... 
    for (s = 0; s < sizeof(sizes)/sizeof(int); s++) {
	    lengthMod = sizes[s] - 1;
	    start = clock();
	    for (i = 0; i < steps; i++) {
                dummy = arr[(i*64) & lengthMod]; 
	        ++arr[(i*64) & lengthMod];
                //arr[(i<< 16) & lengthMod] /= 10;
	    }
 
	    timeTaken = (double)(clock() - start)/CLOCKS_PER_SEC;
        printf("%d, %.8f \n", sizes[s] / 1024, timeTaken);
    }
 
    return 0;
}
