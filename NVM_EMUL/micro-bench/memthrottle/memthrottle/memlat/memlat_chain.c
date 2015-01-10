#include <string.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <numa.h>
#include "rdpmc.h"

#define P  (void)printf
#define FP (void)fprintf

#define PAGESZ 4096

#ifndef ELEMSZ
# define ELEMSZ 64
#endif

#define MAX_NUM_CHAINS 16

void mbarrier() {                                                                                           
    asm volatile( "mfence" :::"memory");                                                
} 

typedef struct {
	uint64_t val;
#if (ELEMSZ > 8)
	uint64_t padding[(ELEMSZ - sizeof(uint64_t))/sizeof (uint64_t)];
#endif
} element_t;

/* G. Marsaglia, 2003.  "Xorshift RNGs", Journal of Statistical
   Software v. 8 n. 14, pp. 1-6, discussed in _Numerical Recipes_
   3rd ed. */
static uint64_t prng(uint64_t* seed) {
    uint64_t x = *seed;
    x ^= x >> 21;
    x ^= x << 35;
    x ^= x >>  4;
    *seed = x;
    return x;
}

static uint64_t safe_strtoull(const char *s) {
  char *ep;
  uint64_t r;
  errno = 0;
  assert(NULL != s && '\0' != *s);
  r = strtoull(s, &ep, 10);
  assert(0 == errno && '\0' == *ep);
  return r;
}

static uint64_t T(void) {
  struct timeval tv;
  int r = gettimeofday(&tv, NULL);
  assert(0 == r);
  return (uint64_t)(tv.tv_sec) * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
  uint64_t seed, seedin, sum, N, j, i, p, T1, T2, nodei, nodej, nchains, monitoring;
  uint64_t sumv[MAX_NUM_CHAINS];
  uint64_t nextp[MAX_NUM_CHAINS];
  element_t *B;
  element_t *C[MAX_NUM_CHAINS];
  element_t tmp;
  char *A;

  if (7 != argc) {
    FP(stderr, "usage: %s PRNGseed Nchains Nelems node_i node_j\n", argv[0]);
    return 1;
  }
  seedin  = safe_strtoull(argv[1]);
  nchains = safe_strtoull(argv[2]);
  N     = safe_strtoull(argv[3]);
  nodei = safe_strtoull(argv[4]);
  nodej = safe_strtoull(argv[5]);
  monitoring = safe_strtoull(argv[6]);

  assert(UINT64_MAX > N);
  assert(nchains < MAX_NUM_CHAINS);

  if (1000000 > N) 
    FP(stderr, "warning:  N == %" PRIu64 " seems small!\n", N);

  FP(stderr, "element size = %" PRIu64 " bytes\n", sizeof(element_t));
  for (j=0; j < nchains; j++) {
    seed = seedin + j*j;
    /* fill B[] with random permutation of 1..N */
    A = (char *)malloc(2 * PAGESZ + N * sizeof *B);
    assert(NULL != A);
    while ( 0 != (A - (char *)0) % PAGESZ )
      A++;
    B = (element_t *)A;
    for (i = 0; i < N; i++)
      B[i].val = 1+i;
    for (i = 0; i < N; i++) {
      uint64_t r, t;
      r = prng(&seed);
      r = r % N;  /* should be okay for N << 2^64 */
      t = B[i].val;
      B[i].val = B[r].val;
      B[r].val = t;
    }
    sum = 0;
    for (i = 0; i < N; i++)
      sum += B[i].val;
    assert((N+1)*N/2 == sum);  /* Euler's formula */

    FP(stderr, "Setup chasing pointers %d...\n", j);
    /* set up C[] such that "chasing pointers" through it visits
       every element exactly once */
    //A = numa_alloc_onnode(2 * PAGESZ + (1+N) * sizeof *C[0], nodej);
    numa_run_on_node(nodei);
    A = (char*) mmap(NULL, 2 * PAGESZ + (1+N) * sizeof *C[0], PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_HUGETLB, -1, 0);
    assert(NULL != A);
    while ( 0 != (A - (char *)0) % PAGESZ )
      A++;
    C[j] = (element_t *)A;
    for (i = 0; i <= N; i++)
      C[j][i].val = UINT64_MAX;
    p = 0;
    for (i = 0; i < N; i++)
      p = C[j][p].val = B[i].val;
    C[j][p].val = 0;
    for (i = 0; i <= N; i++)
      assert(N >= C[j][i].val);
  }

  FP(stderr, "Trash the CPU cache...\n");
  /* trash the CPU cache */
  T1 = T() % 1000;
  for (i = 0; i < N; i++)
    B[i].val = T1 * i + i % T1;
  sum = 0;
  for (i = 0; i < N; i++)
    sum += B[i].val;
  P("garbage == %" PRIu64 "  \n", sum);  /* prevent optimizer from removing preceding loops */

#if 0
  /* Initializing the rdpmc library */
  int eventId[4] = {0, 0, 0, 0};
  long long start0, start1, start2, start3, stop0, stop1, stop2, stop3;
  start0 = start1 = start2 = start3 = stop0 = stop1 = stop2 = stop3 = 0;

  CTRL_SET_EVENT(eventId[0], 0xa3);
  CTRL_SET_UM(eventId[0], 0x05);
  CTRL_SET_INT(eventId[0]);
  CTRL_SET_ENABLE(eventId[0]);
  
  //printf("eventId[0]=%x\n", eventId[0]);
  eventId[0] = 0x55305a3;
  //printf("eventId[0]=%x\n", eventId[0]);

  CTRL_SET_EVENT(eventId[1], 0xd4);
  CTRL_SET_UM(eventId[1], 0x02);
  CTRL_SET_INT(eventId[1]);
  CTRL_SET_ENABLE(eventId[1]);

  CTRL_SET_EVENT(eventId[2], 0xd1);
  CTRL_SET_UM(eventId[2], 0x04);
  CTRL_SET_INT(eventId[2]);
  CTRL_SET_ENABLE(eventId[2]);

  CTRL_SET_EVENT(eventId[3], 0xb1);
  CTRL_SET_UM(eventId[3], 0x01);
  CTRL_SET_INT(eventId[3]);
  CTRL_SET_ENABLE(eventId[3]);
  CTRL_SET_INV(eventId[3]);
  CTRL_SET_CM(eventId[3], 0x01);

  pmc_init(eventId, 4);

 /* start reading rdpmc counter */
  start0 = rdpmc(0);
  start1 = rdpmc(1);
  start2 = rdpmc(2);
  start3 = rdpmc(0x3);
#endif

 if(monitoring)
          start_perf_monitoring();

  /* chase the pointers */
  FP(stderr, "Chase the pointers...\n");
  if (nchains == 1) {
    T1 = T();
    sumv[0] = 0;
    for (i = 0; 0 != C[0][i].val; i = C[0][i].val) {
      sumv[0] += C[0][i].val;
      //memcpy(&tmp, (void*) &C[0][i], ELEMSZ);
    }
    T2 = T();
  } else {
    T1 = T();
    for (j=0; j < nchains; j++) {
      sumv[j] = 0;
      nextp[j] = 0;
    }
    for (; 0 != C[0][nextp[0]].val; ) {
      for (j=0; j < nchains; j++) {
        sumv[j] += C[0][nextp[0]].val;
        //memcpy(&tmp, (void*) &C[j][nextp[j]], ELEMSZ);
        nextp[j] = C[j][nextp[j]].val;
      }
    //mbarrier();
    }
    T2 = T();
  }

#if 0
  /* read rdpmc counter again*/
  stop0 = rdpmc(0);
  stop1 = rdpmc(1);
  stop2 = rdpmc(2);
  stop3 = rdpmc(0x3);
#endif

 if(monitoring)
          start_perf_monitoring();

  for (j=0; j < nchains; j++) {
    P( "sum[%" PRIu64 "] == %" PRIu64 "\n", j, sumv[j]);
  }
  P("N == %" PRIu64 " sum[0] == %" PRIu64 " time == %" PRIu64 "us time_per_op == %" PRIu64 "ns\n", N, sumv[0], T2-T1, (T2-T1)*1000/N);

 //printf("Stall_l2_pending: %llu\nLLC_Miss: %llu\nLLC_Hit: %llu\nDispatch Stall: %llu LDM_Stall: %llu\n", stop0-start0, stop1-start1, stop2-start2, stop3-start3, (stop0-start0)*(7*(stop1-start1)/(7*(stop1-start1) + stop2-start2))); 
  assert((N+1)*N/2 == sumv[0]);  /* Euler's formula */

  return 0;
}

