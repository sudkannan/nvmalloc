#include "model.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int READ_LATENCY;

static inline void asm_cpuid() {
        asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}

static inline void asm_mfence(void)
{
        __asm__ __volatile__ ("mfence");
}

#if defined(__i386__)

static inline unsigned long long hrtime_cycles(void)
{
        unsigned long long int x;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
        return x;
}

#elif defined(__x86_64__)

typedef unsigned long long hrtime_t;

static inline unsigned long long hrtime_cycles(void)
{
        unsigned hi, lo;
        __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#else
#error "What architecture is this???"
#endif


void
emulate_latency_ns(int ns)
{
        hrtime_t cycles;
        hrtime_t start;
        hrtime_t stop;

        if (ns==0) {
                return;
        }

        start = hrtime_cycles();
        cycles = HRTIME_NS2CYCLE(ns);

        do {
                /* RDTSC doesn't necessarily wait for previous instructions to complete 
                 * so a serializing instruction is usually used to ensure previous 
                 * instructions have completed. However, in our case this is a desirable
                 * property since we want to overlap the latency we emulate with the
                 * actual latency of the emulated instruction. 
                 */
                stop = hrtime_cycles();
                asm_cpuid();
        } while (stop - start < cycles);
}

#if 0

/* this code needs to be made much more generic - currently assumes
 * 64 bit and 64 byte cache lines.
 */

#define CACHE_LINE_SIZE 64
#define CACHE_LINE_MASK (0xffffffffffffffff - CACHE_LINE_SIZE + 1)

// offset within the cache line 
#define CACHE_LINE_OFFSET(addr) ( (uint64_t *) (((uint64_t) (addr)) & (CACHE_LINE_SIZE - 1)) )

static inline char *round_to_cache(char *addr) {
    return (char *)((uint64_t)addr & CACHE_LINE_MASK);
}

static inline void clflush(volatile void *p) {
    asm volatile("clflush %0" :: "m"(*(volatile char *) p) : "memory");
}

static inline void cflush(void *addr, size_t nbytes) {
    asm volatile("mfence");
    char *start = round_to_cache((char *)addr);
    char *end = (char *)addr+nbytes;
    do {
    clflush(start);
    start += CACHE_LINE_SIZE;
    } while (start < end);
    asm volatile("mfence" ::: "memory");
}

static inline void compiler_barrier() {
    asm volatile("" ::: "memory");
}

static inline void mfence() {
    asm volatile("mfence" ::: "memory");
}

static inline void asm_sse_write_block64(volatile void *dst, void *src)
{
    uint64_t* daddr = (uint64_t*) dst;
    uint64_t* saddr = (uint64_t*) src;

    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[0]): "r" (saddr[0]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[1]): "r" (saddr[1]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[2]): "r" (saddr[2]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[3]): "r" (saddr[3]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[4]): "r" (saddr[4]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[5]): "r" (saddr[5]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[6]): "r" (saddr[6]));
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*&daddr[7]): "r" (saddr[7]));
}

static inline void asm_sse_write(volatile void *dst, uint64_t val)
{
    uint64_t* daddr = (uint64_t*) dst;
    __asm__ __volatile__ ("movnti %1, %0" : "=m"(*daddr): "r" (val));
}


void* ntmemcpy_noinline(void *dst, const void *src, size_t n);

// handle some special cases as inline for speed
static inline void* ntmemcpy(void *dst, const void *src, size_t n)
{
    if (n == sizeof(uint64_t)) {
        uint64_t val = *((uint64_t*) src);
        asm_sse_write(dst, val);
        return dst;
    }
    return ntmemcpy_noinline(dst, src, n);
}


void*
sse_memcpy(void *dst, const void *src, size_t n)
{
    uintptr_t   saddr = (uintptr_t) src;
    uintptr_t   daddr = (uintptr_t) dst;
    uintptr_t   offset;
    uint64_t*   val;
    size_t      size = n;

    // We need to align stream stores at cacheline boundaries
    // Start with a non-aligned cacheline write, then proceed with
    // a bunch of aligned writes, and then do a last non-aligned
    // cacheline for the remaining data.

    if (size >= CACHE_LINE_SIZE) {
        if ((offset = (uintptr_t) CACHE_LINE_OFFSET(daddr)) != 0) {
            size_t nlivebytes = (CACHE_LINE_SIZE - offset);
            while (nlivebytes >= sizeof(uint64_t)) {
                asm_sse_write((uint64_t *) daddr, *((uint64_t *) saddr));
                saddr+=sizeof(uint64_t);
                daddr+=sizeof(uint64_t);
                size-=sizeof(uint64_t);
                nlivebytes-=sizeof(uint64_t);
            }
        }
        while(size >= CACHE_LINE_SIZE) {
            val = ((uint64_t *) saddr);
            asm_sse_write_block64((uintptr_t *) daddr, val);
            saddr+=CACHE_LINE_SIZE;
            daddr+=CACHE_LINE_SIZE;
            size-=CACHE_LINE_SIZE;
        }
    }
    while (size >= sizeof(uint64_t)) {
        val = ((uint64_t *) saddr);
        asm_sse_write((uint64_t *) daddr, *((uint64_t *) saddr));
        saddr+=sizeof(uint64_t);
        daddr+=sizeof(uint64_t);
        size-=sizeof(uint64_t);
    }
    return dst;
}

#else

/* for small memory blocks (<256 bytes) this version is faster */
#define small_memcpy(to,from,n)\
{\
register unsigned long int dummy;\
__asm__ __volatile__(\
  "rep; movsb"\
  :"=&D"(to), "=&S"(from), "=&c"(dummy)\
  :"0" (to), "1" (from),"2" (n)\
  : "memory");\
}

/* linux kernel __memcpy (from: /include/asm/string.h) */
static __inline__ void * __memcpy (
			       void * to,
			       const void * from,
			       size_t n)
{
int d0, d1, d2;

  if( n < 4 ) {
    small_memcpy(to,from,n);
  }
  else
    __asm__ __volatile__(
    "rep ; movsl\n\t"
    "testb $2,%b4\n\t"
    "je 1f\n\t"
    "movsw\n"
    "1:\ttestb $1,%b4\n\t"
    "je 2f\n\t"
    "movsb\n"
    "2:"
    : "=&c" (d0), "=&D" (d1), "=&S" (d2)
    :"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
    : "memory");

  return (to);
}

#define SSE_MMREG_SIZE 16
#define MMX_MMREG_SIZE 8

#define MMX1_MIN_LEN 0x800  /* 2K blocks */
#define MIN_LEN 0x40  /* 64-byte blocks */

/* SSE note: i tried to move 128 bytes a time instead of 64 but it
didn't make any measureable difference. i'm using 64 for the sake of
simplicity. [MF] */
static void * sse_memcpy(void * vto, const void * vfrom, size_t len)
{
  const char* from = (const char*) vfrom;
  char* to = (char*) vto;
  void *retval;
  size_t i;
  retval = to;

  /* PREFETCH has effect even for MOVSB instruction ;) */
  __asm__ __volatile__ (
    "   prefetchnta (%0)\n"
    "   prefetchnta 32(%0)\n"
    "   prefetchnta 64(%0)\n"
    "   prefetchnta 96(%0)\n"
    "   prefetchnta 128(%0)\n"
    "   prefetchnta 160(%0)\n"
    "   prefetchnta 192(%0)\n"
    "   prefetchnta 224(%0)\n"
    "   prefetchnta 256(%0)\n"
    "   prefetchnta 288(%0)\n"
    : : "r" (from) );

  if(len >= MIN_LEN)
  {
    register unsigned long int delta;
    /* Align destinition to MMREG_SIZE -boundary */
    delta = ((unsigned long int)to)&(SSE_MMREG_SIZE-1);
    if(delta)
    {
      delta=SSE_MMREG_SIZE-delta;
      len -= delta;
      small_memcpy(to, from, delta);
    }
    i = len >> 6; /* len/64 */
    len&=63;
    if(((unsigned long)from) & 15)
      /* if SRC is misaligned */
      for(; i>0; i--)
      {
        __asm__ __volatile__ (
        "prefetchnta 320(%0)\n"
       "prefetchnta 352(%0)\n"
        "movups (%0), %%xmm0\n"
        "movups 16(%0), %%xmm1\n"
        "movups 32(%0), %%xmm2\n"
        "movups 48(%0), %%xmm3\n"
        "movntps %%xmm0, (%1)\n"
        "movntps %%xmm1, 16(%1)\n"
        "movntps %%xmm2, 32(%1)\n"
        "movntps %%xmm3, 48(%1)\n"
        :: "r" (from), "r" (to) : "memory");
        from+=64;
        to+=64;
      }
    else
      /*
         Only if SRC is aligned on 16-byte boundary.
         It allows to use movaps instead of movups, which required data
         to be aligned or a general-protection exception (#GP) is generated.
      */
      for(; i>0; i--)
      {
        __asm__ __volatile__ (
        "prefetchnta 320(%0)\n"
       "prefetchnta 352(%0)\n"
        "movaps (%0), %%xmm0\n"
        "movaps 16(%0), %%xmm1\n"
        "movaps 32(%0), %%xmm2\n"
        "movaps 48(%0), %%xmm3\n"
        "movntps %%xmm0, (%1)\n"
        "movntps %%xmm1, 16(%1)\n"
        "movntps %%xmm2, 32(%1)\n"
        "movntps %%xmm3, 48(%1)\n"
        :: "r" (from), "r" (to) : "memory");
        from+=64;
        to+=64;
      }
    /* since movntq is weakly-ordered, a "sfence"
     * is needed to become ordered again. */
    __asm__ __volatile__ ("sfence":::"memory");
    /* enables to use FPU */
    __asm__ __volatile__ ("emms":::"memory");
  }
  /*
   *	Now do the tail of the block
   */
  if(len) __memcpy(to, from, len);
  return retval;
}

#endif

void *nvm_memcpy(void *dest, const void *src, size_t n) 
{
    emulate_latency_ns(READ_LATENCY);
    return sse_memcpy(dest, src, n);
}
