#include <pthread.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include "cpu/cpu.h"
#include "uthash/src/utlist.h"
#include "error.h"
#include "interpose.h"
#include "thread.h"
#include "topology.h"

__thread thread_t* tls_thread = NULL;

static thread_manager_t* thread_manager;

// assign a virtual/physical node using a round-robin policy
static void rr_next_cpu_id(thread_manager_t* thread_manager, int* next_virtual_node_idp, int* next_cpu_idp)
{
    int i,v,c,j;
    int next_virtual_node_id;
    virtual_node_t* virtual_node;
    physical_node_t* physical_node;
    virtual_topology_t* virtual_topology = thread_manager->virtual_topology;

    *next_virtual_node_idp = thread_manager->next_virtual_node_id;
    *next_cpu_idp = thread_manager->next_cpu_id;

    // advance to the next virtual node and cpu id
    next_virtual_node_id = thread_manager->next_virtual_node_id;
    virtual_node = &virtual_topology->virtual_nodes[next_virtual_node_id];
    physical_node = virtual_node->dram_node; // we run threads on the dram node
    if ((thread_manager->next_cpu_id = next_cpu(physical_node->cpu_bitmask, thread_manager->next_cpu_id + 1)) < 0) {
        next_virtual_node_id = (next_virtual_node_id + 1) % virtual_topology->num_virtual_nodes;
        virtual_node = &virtual_topology->virtual_nodes[next_virtual_node_id];
        physical_node = virtual_node->dram_node;
        thread_manager->next_cpu_id = first_cpu(physical_node->cpu_bitmask);
    } 
}

int register_thread(thread_manager_t* thread_manager, pthread_t pthread, pid_t tid)
{
    int cpu_id;
    int virtual_node_id;
    int physical_node_id;
    thread_t* thread = malloc(sizeof(thread_t));

    thread->pthread = pthread;
    thread->tid = tid;
    thread->thread_manager = thread_manager;

    __lib_pthread_mutex_lock(&thread_manager->mutex);
    rr_next_cpu_id(thread_manager, &virtual_node_id, &cpu_id);
    thread->virtual_node = &thread_manager->virtual_topology->virtual_nodes[virtual_node_id];
    DBG_LOG(INFO, "Binding thread tid: %d pthread: 0x%x on processor %d\n", thread->tid, thread->pthread, cpu_id);
    struct bitmask* cpubind = numa_allocate_cpumask();
    numa_bitmask_setbit(cpubind, cpu_id);
    if (numa_sched_setaffinity(thread->tid, cpubind) != 0) {
        DBG_LOG(ERROR, "Cannot bind thread tid: %d pthread: 0x%x on processor %d\n", thread->tid, thread->pthread, cpu_id);
    }
    numa_bitmask_free(cpubind);
    struct bitmask* membind = numa_allocate_nodemask();
    physical_node_id = thread_manager->virtual_topology->virtual_nodes[virtual_node_id].dram_node->node_id;
    numa_bitmask_setbit(membind, physical_node_id);
    numa_set_membind(membind);
    numa_free_nodemask(membind);
    LL_APPEND(thread_manager->thread_list, thread);
    __lib_pthread_mutex_unlock(&thread_manager->mutex);

    tls_thread = thread;

    return E_SUCCESS;
}

thread_t* thread_self()
{
    return tls_thread;
}

int register_self()
{
    return register_thread(thread_manager, pthread_self(), (pid_t) syscall (SYS_gettid));
}

int init_thread_manager(virtual_topology_t* virtual_topology)
{
    int ret;
    thread_manager_t* mgr;
    virtual_node_t* virtual_node;
    physical_node_t* physical_node;

    if (!(mgr = malloc(sizeof(thread_manager_t)))) {
        ret = E_ERROR;
        goto done;    
    }
    mgr->thread_list = NULL;
    mgr->virtual_topology = virtual_topology;
    mgr->next_virtual_node_id = 0;
    virtual_node = &virtual_topology->virtual_nodes[mgr->next_virtual_node_id];
    physical_node = virtual_node->dram_node;
    mgr->next_cpu_id = first_cpu(physical_node->cpu_bitmask);
    pthread_mutex_init(&mgr->mutex, NULL);
    
    thread_manager = mgr;
    return E_SUCCESS;

done:
    return ret;
}
