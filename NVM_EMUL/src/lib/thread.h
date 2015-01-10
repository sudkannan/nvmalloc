#ifndef __THREAD_H
#define __THREAD_H

#include <sys/types.h>
#include <numa.h>
#include <pthread.h>

struct cpu_model_s; // opaque
struct virtual_node_s; // opaque
struct virtual_topology_s; // opaque
struct thread_manager_s; // opaque

typedef struct thread_s {
    struct virtual_node_s* virtual_node;
    pthread_t pthread;
    pid_t tid;
    struct thread_manager_s* thread_manager;
    struct thread_s* next;
} thread_t;

typedef struct thread_manager_s {
    pthread_mutex_t mutex;
    thread_t* thread_list;
    int next_virtual_node_id; // used by the round-robin policy -- next virtual node to run on 
    int next_cpu_id; // used by the round-robin policy -- next cpu to run on
    struct virtual_topology_s* virtual_topology;   
} thread_manager_t; 

int register_self();
thread_t* thread_self();

#endif /* __THREAD_H */
