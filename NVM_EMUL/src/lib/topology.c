#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <numa.h>
#include "cpu/cpu.h"
#include "error.h"
#include "measure_bw.h"
#include "topology.h"

#define MAX_NUM_MC_PCI_BUS 16

// return a list of memory-controller pci buses
int get_mc_pci_bus_list(unsigned int bus_id_list[], int max_list_size, int* bus_countp)
{
    FILE* fp;
    char* colon;
    char  buf[1024];
    int   bus_number;
    int   bus_count;

    fp = popen("lspci", "r");
    if (fp == NULL) {
        return E_ERROR;
    }

    for (bus_count=0; fgets(buf, sizeof(buf)-1, fp) != NULL; ) {
        if (strstr(buf, "Thermal")) {
            if (colon = index(buf, ':')) {
                *colon = '\0';
                bus_number = strtol(buf, NULL, 16);
                if (bus_count == 0 || bus_number != bus_id_list[bus_count-1]) {
                    if (bus_count > max_list_size) {
                        return E_ERROR;
                    }
                    bus_id_list[bus_count++] = bus_number;
                }
            }
        }
    }
    *bus_countp = bus_count;
    pclose(fp);

    return E_SUCCESS;
}


// To discover where a memory controller is connected to, we throttle the rest of 
// the memory controllers and measure local bandwidth of each node. The unthrottled 
// memory controller is attached to the node with the highest local bandwidth
int discover_mc_pci_topology(cpu_model_t* cpu_model, physical_node_t* physical_nodes[], int num_physical_nodes)
{
    unsigned int bus_ids[MAX_NUM_MC_PCI_BUS];
    int bus_count;
    physical_node_t* local_node;
    int b, i, j;
    int node = 0;
    double max_local_rbw;
    double rbw;

    get_mc_pci_bus_list(bus_ids, MAX_NUM_MC_PCI_BUS, &bus_count);

    if (bus_count < num_physical_nodes) {
        DBG_LOG(ERROR, "The number of physical nodes is greater than the number of memory-controller pci buses.\n");
    }

    for (b=0; b<bus_count; b++) {
        // throttle all other buses except the one we are currently trying 
        // to figure out where it is attached
        for (i=0; i<bus_count; i++) {
            if (i == b) {
                cpu_model->set_throttle_register(bus_ids[i], THROTTLE_DDR_ACT, 0xffff);
            } else {
                cpu_model->set_throttle_register(bus_ids[i], THROTTLE_DDR_ACT, 0x800f);
            }
        }
        // measure local bandwidth of each node. 
        max_local_rbw = 0;
        for (i=0; i<num_physical_nodes; i++) {
            physical_node_t* node_i = physical_nodes[i];
            rbw = measure_read_bw(node_i->node_id, node_i->node_id);
            if (rbw > max_local_rbw) {
                max_local_rbw = rbw;
                local_node = node_i;
            }
        }
        local_node->mc_pci_bus_id = bus_ids[b];
    }
    for (i=0; i<bus_count; i++) {
        cpu_model->set_throttle_register(bus_ids[i], THROTTLE_DDR_ACT, 0xffff);
    }
    return E_SUCCESS;
}

static int load_mc_pci_topology(const char* path, physical_node_t* physical_nodes[], int num_physical_nodes)
{
    FILE *fp;
    char *line = NULL;
    char str[64];
    size_t len = 0;
    ssize_t read;
    int i, j;
    int bus_id;
    int node_id;
    int bus_count;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return E_ERROR;
    }

    DBG_LOG(INFO, "Loading memory-controller pci topology from %s\n", path);
    for (i = 0, bus_count = 0; (read = getline(&line, &len, fp)) != -1; ) {
        sscanf(line, "%d\t%d", &node_id, &bus_id);
        DBG_LOG(INFO, "node: %d, pci bus: 0x%x\n", node_id, bus_id);
        for (j=0; j<num_physical_nodes; j++) {
            if (node_id == physical_nodes[j]->node_id) {
                physical_nodes[j]->mc_pci_bus_id;
            }
        }
        bus_count++;
    }
    free(line);
    if (bus_count != num_physical_nodes) {
        DBG_LOG(INFO, "No complete memory-controller pci topology found in %s\n", path);
        return E_ERROR;
    }
    fclose(fp);
    return E_SUCCESS;
}

static int save_mc_pci_topology(const char* path, physical_node_t* physical_nodes[], int num_physical_nodes)
{
    int i;
    FILE *fp;

    fp = fopen(path, "w");
    if (fp == NULL) {
        return E_ERROR;
    }

    DBG_LOG(INFO, "Saving memory-controller pci topology into %s\n", path);
    for (i=0; i<num_physical_nodes; i++) {
        int pci_id = physical_nodes[i]->mc_pci_bus_id;
        int node_id = physical_nodes[i]->node_id;
        DBG_LOG(INFO, "node: %d, pci bus: 0x%x\n", node_id, pci_id);
        fprintf(fp, "%d\t%d\n", node_id, pci_id);
    }
    fclose(fp);
    return E_SUCCESS;
}

int num_cpus(struct bitmask* bitmask) 
{
    int i,n;
    // if we had knowledge of the bitmask structure then we could
    // count the bits faster but bitmask seems to be an opaque structure
    for (i=0, n=0; i< numa_num_possible_nodes(); i++) {
        if (numa_bitmask_isbitset(bitmask, i)) {
            n++;
        }
    }
    return n;
}

// number of cpus in the system
int system_num_cpus()
{
    return sysconf( _SC_NPROCESSORS_ONLN );
}

void print_bitmask(struct bitmask* bitmask) {
    int i;
    for (i=0; i<numa_num_possible_nodes(); i++) {
        if (numa_bitmask_isbitset(bitmask, i)) {
            DBG_LOG(INFO, "bit %d\n", i);
        }
    }
    return;
}

int next_cpu(struct bitmask* bitmask, int cpu_id)
{
    int i;
    // if we had knowledge of the bitmask structure then we could
    // count the bits faster but bitmask seems to be an opaque structure
    for (i=cpu_id; i<numa_num_possible_nodes(); i++) {
        if (numa_bitmask_isbitset(bitmask, i)) {
            return i;
        }
    }
    return -1;
}

int first_cpu(struct bitmask* bitmask)
{
    return next_cpu( bitmask, 0);
}

int init_virtual_topology(config_t* cfg, cpu_model_t* cpu_model, virtual_topology_t** virtual_topologyp)
{
    char* mc_pci_file;
    char* str;
    char* saveptr;
    char* token = "NULL";
    int* physical_node_ids;
    physical_node_t** physical_nodes;
    int num_physical_nodes;
    int n, v, i, j, sibling_idx, node_i_idx;
    int node_id;
    physical_node_t* node_i, *node_j, *sibling_node;
    int ret;
    int min_distance;
    int hyperthreading;
    struct bitmask* mem_nodes;
    virtual_topology_t* virtual_topology;

    __cconfig_lookup_string(cfg, "topology.physical_nodes", &str);

    // parse the physical nodes string
    physical_node_ids = calloc(numa_num_possible_nodes(), sizeof(*physical_node_ids));
    num_physical_nodes = 0;
    while (token = strtok_r(str, ",", &saveptr)) {
        physical_node_ids[num_physical_nodes] = atoi(token);
        str = NULL;
        if (++num_physical_nodes > numa_num_possible_nodes()) {
            // we re being asked to run on more nodes than available
            free(physical_node_ids);
            ret = E_ERROR;
            goto done;
        }
    }
    physical_nodes = calloc(num_physical_nodes, sizeof(*physical_nodes));

    // select those nodes we can run on (e.g. not constrained by any numactl)
    mem_nodes = numa_get_mems_allowed();
    for (i=0, n=0; i<num_physical_nodes; i++) {
        node_id = physical_node_ids[i];
        if (numa_bitmask_isbitset(mem_nodes, node_id)) {
            physical_nodes[n] = malloc(sizeof(**physical_nodes));
            physical_nodes[n]->node_id = node_id;
            // TODO: what if we want to avoid using only a single hardware contexts of a hyperthreaded core?
            physical_nodes[n]->cpu_bitmask = numa_allocate_cpumask();
            numa_node_to_cpus(node_id, physical_nodes[n]->cpu_bitmask);
            __cconfig_lookup_bool(cfg, "topology.hyperthreading", &hyperthreading);
            if (hyperthreading) {
                physical_nodes[n]->num_cpus = num_cpus(physical_nodes[n]->cpu_bitmask);
            } else {
                DBG_LOG(INFO, "Not using hyperthreading.\n");
                // disable the upper half of the processors in the bitmask
                physical_nodes[n]->num_cpus = num_cpus(physical_nodes[n]->cpu_bitmask) / 2;
                int fc = first_cpu(physical_nodes[n]->cpu_bitmask);
                for (j=fc+system_num_cpus()/2; j<fc+system_num_cpus()/2+physical_nodes[n]->num_cpus; j++) {
                    if (numa_bitmask_isbitset(physical_nodes[n]->cpu_bitmask, j)) {
                        numa_bitmask_clearbit(physical_nodes[n]->cpu_bitmask, j);
                    }
                }
            }
            n++;
        }
    }
    free(physical_node_ids);
    num_physical_nodes = n;

    // if pci bus topology of each physical node is not provided then discover it 
    if (__cconfig_lookup_string(cfg, "topology.mc_pci", &mc_pci_file) == CONFIG_FALSE || 
        (__cconfig_lookup_string(cfg, "topology.mc_pci", &mc_pci_file) == CONFIG_TRUE &&
         load_mc_pci_topology(mc_pci_file, physical_nodes, num_physical_nodes) != E_SUCCESS)) 
    {
        discover_mc_pci_topology(cpu_model, physical_nodes, num_physical_nodes);
        save_mc_pci_topology(mc_pci_file, physical_nodes, num_physical_nodes);
    }

    // form virtual nodes by grouping physical nodes that are close to each other
    virtual_topology = malloc(sizeof(*virtual_topology));
    virtual_topology->num_virtual_nodes = num_physical_nodes / 2 + num_physical_nodes % 2;
    virtual_topology->virtual_nodes = calloc(virtual_topology->num_virtual_nodes, 
                                             sizeof(*(virtual_topology->virtual_nodes)));
    
    for (i=0, v=0; i<num_physical_nodes; i++) {
        min_distance = INT_MAX;
        sibling_node = NULL;
        sibling_idx = -1;
        if ((node_i = physical_nodes[i]) == NULL) {
            continue;
        }
        for (j=i+1; j<num_physical_nodes; j++) {
            if ((node_j = physical_nodes[j]) == NULL) {
                continue;
            }
            if (numa_distance(node_i->node_id,node_j->node_id) < min_distance) {
                sibling_node = node_j;
                sibling_idx = j;
            }
        }
        if (sibling_node) {
            physical_nodes[i] = physical_nodes[sibling_idx] = NULL;
            virtual_node_t* virtual_node = &virtual_topology->virtual_nodes[v];
            virtual_node->dram_node = node_i;
            virtual_node->nvram_node = sibling_node;
            virtual_node->node_id = v;
            virtual_node->cpu_model = cpu_model;
            DBG_LOG(INFO, "Fusing physical nodes %d %d into virtual node %d\n", 
                    node_i->node_id, sibling_node->node_id, virtual_node->node_id);
            v++;
        }
    }

    // any physical node that is not paired with another physical node is 
    // formed into a virtual node on its own
    if (2*v < num_physical_nodes) {
        for (i=0; i<num_physical_nodes; i++) {
            node_i = physical_nodes[i];
            virtual_node_t* virtual_node = &virtual_topology->virtual_nodes[v];
            virtual_node->dram_node = virtual_node->nvram_node = node_i;
            virtual_node->node_id = v;
 
            DBG_LOG(WARNING, "Forming physical node %d into virtual node %d without a sibling node.\n",
                    node_i->node_id, virtual_node->node_id);
        }
    }

    *virtual_topologyp = virtual_topology;
    ret = E_SUCCESS;

done:
    free(physical_nodes);
    return ret;
}
