#ifndef __TOPOLOGY_H
#define __TOPOLOGY_H

#include <numa.h>
#include "config.h"
#include "cpu/cpu.h"

typedef struct {
    int node_id;
    int mc_pci_bus_id;
    int num_cpus; // number of node's cpus
    struct bitmask* cpu_bitmask; // a bitmask of the node's CPUs 
} physical_node_t;

typedef struct virtual_node_s {
    int node_id;
    physical_node_t* dram_node;
    physical_node_t* nvram_node;
    cpu_model_t* cpu_model;
} virtual_node_t;

typedef struct virtual_topology_s {
    virtual_node_t* virtual_nodes; // pointer to an array of virtual nodes
    int num_virtual_nodes;
} virtual_topology_t;

int init_virtual_topology(config_t* cfg, cpu_model_t* cpu_model, virtual_topology_t** virtual_topologyp);
int system_num_cpus();
int first_cpu(struct bitmask* bitmask);
int next_cpu(struct bitmask* bitmask, int cpu_id);

#endif /* __TOPOLOGY_H */
