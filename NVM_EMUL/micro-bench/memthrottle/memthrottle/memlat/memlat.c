#include <string.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <numa.h>
#include "memlat.h"
#include "rdpmc.h"

#define P  (void)printf
#define FP (void)fprintf

#define PAGESZ 4096

#ifndef ELEMSZ
# define ELEMSZ 8192
#endif

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
static void prng(const uint64_t seed, uint64_t *r) {
  static uint64_t x;  /* state */
  if (seed) { assert(0 == x && NULL == r);
              x = seed;  }
  else {      assert(0 != x && NULL != r);
              x ^= x >> 21;
              x ^= x << 35;
              x ^= x >>  4;
              *r = x;  }
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
  uint64_t seed, N, i, sum, p, T1, T2, nodei, nodej, element_size;
  element_t *B;
  element_t *C;
  element_t tmp;
  char *A;

  if (5 != argc) {
    FP(stderr, "usage: %s PRNGseed N node_i node_j\n", argv[0]);
    return 1;
  }
  seed  = safe_strtoull(argv[1]);
  N     = safe_strtoull(argv[2]);
  nodei = safe_strtoull(argv[3]);
  nodej = safe_strtoull(argv[4]);

  assert(UINT64_MAX > N);

  if (1000000 > N)
    FP(stderr, "warning:  N == %" PRIu64 " seems small!\n", N);

  FP(stderr, "element size = %" PRIu64 " bytes\n", sizeof(element_t));
  prng(seed, NULL);

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
    prng(0, &r);
    r = r % N;  /* should be okay for N << 2^64 */
    t = B[i].val;
    B[i].val = B[r].val;
    B[r].val = t;
  }
  sum = 0;
  for (i = 0; i < N; i++)
    sum += B[i].val;
  assert((N+1)*N/2 == sum);  /* Euler's formula */

  FP(stderr, "Setup chasing pointers...\n");
  /* set up C[] such that "chasing pointers" through it visits
     every element exactly once */
  A = numa_alloc_onnode(2 * PAGESZ + (1+N) * sizeof *C, nodej);
  numa_run_on_node(nodei);
  //A = (char*) mmap(NULL, 2 * PAGESZ + (1+N) * sizeof *C, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_HUGETLB, -1, 0);
  assert(NULL != A);
  while ( 0 != (A - (char *)0) % PAGESZ )
    A++;
  C = (element_t *)A;
  for (i = 0; i <= N; i++)
    C[i].val = UINT64_MAX;
  p = 0;
  for (i = 0; i < N; i++)
    p = C[p].val = B[i].val;
  C[p].val = 0;
  for (i = 0; i <= N; i++)
    assert(N >= C[i].val);

  FP(stderr, "trash the CPU cache...\n");
  /* trash the CPU cache */
  T1 = T() % 1000;
  for (i = 0; i < N; i++)
    B[i].val = T1 * i + i % T1;
  sum = 0;
  for (i = 0; i < N; i++)
    sum += B[i].val;
  P("garbage == %" PRIu64 "  ", sum);  /* prevent optimizer from removing preceding loops */

  /* Initializing the rdpmc library */
  int eventId[4] = {0, 0, 0, 0};
  long long start0, start1, start2, start3, stop0, stop1, stop2, stop3;
  start0 = start1 = start2 = start3 = stop0 = stop1 = stop2 = stop3 = 0;

  CTRL_SET_EVENT(eventId[0], 0xa3);
  CTRL_SET_UM(eventId[0], 0x05);
  CTRL_SET_INT(eventId[0]);
  CTRL_SET_ENABLE(eventId[0]);

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

  //for(i=0; i<4; i++)
//	  printf("Event[%d]:0x%x\n", i, eventId[i]);
  pmc_init(eventId, 4);

  /* start reading rdpmc counter */
  start0 = rdpmc(0);
  start1 = rdpmc(1);
  start2 = rdpmc(2);
  start3 = rdpmc(0x3);

  /* chase the pointers */
  T1 = T();
  sum = 0;
  for (i = 0; 0 != C[i].val; i = C[i].val) {
    sum += C[i].val;
    //memcpy(&tmp, (void*) &C[i], ELEMSZ);
    //memcpy(&tmp, (void*) &C[i], element_size);
    //mbarrier();
  }
  T2 = T();
  /* read rdpmc counter again*/
  stop0 = rdpmc(0);
  stop1 = rdpmc(1);
  stop2 = rdpmc(2);
  stop3 = rdpmc(0x3);

  P("N == %" PRIu64 " sum == %" PRIu64 " time == %" PRIu64 "us time_per_op == %" PRIu64 "ns\n", N, sum, T2-T1, (T2-T1)*1000/N);

 printf("Stall_l2_pending: %llu\nLLC_Miss: %llu\nLLC_Hit: %llu\nDispatch Stall:%llu LDM_Stall:%llu\n", stop0-start0, stop1-start1, stop2-start2, stop3-start3, (stop0-start0)*(7*(stop1-start1)/(7*(stop1-start1) + stop2-start2) )); 
  assert((N+1)*N/2 == sum);  /* Euler's formula */

  return 0;
}

