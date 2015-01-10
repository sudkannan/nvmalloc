#include <numa.h>
#include "topology.h"
#include "pmalloc.h"
#include "thread.h"

// pmalloc should be implemented as a separate library

// FIXME: pmalloc currently uses numa_alloc_onnode() which is slower than regular malloc.
// Consider layering another malloc on top of a emulated nvram 

void* pmalloc(size_t size)
{
    thread_t* thread = thread_self();
    return numa_alloc_onnode(size, thread->virtual_node->nvram_node->node_id);
}

void pfree(void* start, size_t size)
{
    numa_free(start, size);
}
