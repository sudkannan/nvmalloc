// small utility to measure cache and memory read access times
// afb 10/2008
//
// Usage:
//    pointer-chasing [memsize in kb [count]]
// where
//    memsize in kb          is the size of the memory buffer
//                           which is accessed in a randomized
//                           and sequentialized way
//    count                  total number of memory accesses
//                           from the memory buffer
// The memory buffer is organized as an array of pointers where
//    - all pointers point into the very same buffer and where
//    - beginning from any pointer all other pointers are
//      referenced directly or indirectly, and where
//    - the pointer chain is randomized
//
// Once such a memory buffer has been set up, we measure the time of
//
//    void** p = (void**) memory[0];
//    while (count-- > 0) {
//       p = (void**) *p;
//    }
//
// The "p = (void**) *p" construct causes all memory accesses to
// be serialized, i.e. the next access can only be started whenever
// the previous is finished.

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <sys/times.h>
#include <limits.h>
#include <unistd.h>

using namespace std;
volatile void* global; // to defeat optimizations

// setup a memory buffer of the given size and access all
// memory cells randomly count times;
// the number of clock ticks passed for chasing the
// pointers in the memory buffer is returned (this
// does not include the setup);
// the global pointer is updated to defeat optimizations
clock_t chase_pointers(unsigned int size, unsigned int count) {
   unsigned int len = size / sizeof(void*);
   void** memory = new void*[len];

   // shuffle indices
   int* indices = new int[len];
   for (int i = 0; i < len; ++i) {
      indices[i] = i;
   }
   for (int i = 0; i < len-1; ++i) {
      int j = i + lrand48() % (len - i);
      if (i != j) {
	 int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
      }
   }
   // fill memory with pointer references
   for (int i = 1; i < len; ++i) {
      memory[indices[i-1]] = (void*) &memory[indices[i]];
   }
   memory[indices[len-1]] = (void*) &memory[indices[0]];
   delete[] indices;

   // sleep(3); // for cputrack
   struct tms timebuf1; times(&timebuf1);
   // chase the pointers count times
   void** p = (void**) memory[0];
   while (count-- > 0) {
      p = (void**) *p;
   }
   global = *p;
   struct tms timebuf2; times(&timebuf2);
   clock_t ticks = timebuf2.tms_utime - timebuf1.tms_utime;
   // sleep(3); // for cputrack

   delete[] memory;
   return ticks;
}

char* cmdname;
void usage() {
   cerr << "Usage: " << cmdname << " [memsize in kb [count]]" << endl;
   exit(1);
}

int main(int argc, char** argv) {
   unsigned int memsize = 1024;
   unsigned int count;

   cmdname = *argv++; --argc;
   // first optional argument is the memsize in kb
   if (argc > 0) {
      unsigned int kb;
      istringstream arg(*argv++); --argc;
      if (!(arg >> kb) || kb <= 0) usage();
      cout << "memsize in kb = " << kb << endl;
      memsize = kb * 1024;
   }
   // second optional argument is count
   if (argc > 0) {
      istringstream arg(*argv++); --argc;
      if (!(arg >> count) || count <= 0) usage();
   } else {
      // compute some reasonable default value for count
      count = memsize * 16;
      unsigned int min = 1024 * 1024 * 1024;
      if (count < min) {
	 count = min;
      }
   }
   if (argc > 0) usage();
   clock_t ticks = chase_pointers(memsize, count);
#ifndef CLK_TCK
   int CLK_TCK = sysconf(_SC_CLK_TCK);
#endif
   double avgTimeInNanosecs =
      (double) ticks / CLK_TCK * 1000000000 / (double) count;
   cout << "avg access time in ns: " << avgTimeInNanosecs << endl;
}
