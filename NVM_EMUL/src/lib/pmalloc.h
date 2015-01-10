#ifndef __PMALLOC_H
#define __PMALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *pmalloc(size_t size);
void pfree(void *start, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __PMALLOC_H */
