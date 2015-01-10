#include <errno.h>
#include "cpu/cpu.h"
#include "config.h"
#include "error.h"
#include "model.h"
#include "thread.h"
#include "topology.h"

static void init() __attribute__((constructor));

void init()
{
    config_t cfg;
    cpu_model_t* cpu;
    virtual_topology_t* virtual_topology;
    char* ld_preload_path;

    // FIXME: do we need to register the main thread with our system?
    // YES: for sure for single-threaded apps

    // we reset LD_PRELOAD to ensure we don't get into recursive preloads when 
    // calling popen during initialization. before exiting we reactivate LD_PRELOAD 
    // to allow LD_PRELOADS on children
    ld_preload_path = getenv("LD_PRELOAD");
    unsetenv("LD_PRELOAD");

    __cconfig_init(&cfg, "nvmemul.ini");
    if (dbg_init(&cfg, -1, NULL) != E_SUCCESS) {
        goto error;
    }

    if (init_interposition() != E_SUCCESS) {
        goto error;
    }

    if ((cpu = cpu_model()) == NULL) {
        DBG_LOG(ERROR, "No supported processor found\n");
        goto error;
    }

    init_virtual_topology(&cfg, cpu, &virtual_topology);
    init_thread_manager(virtual_topology);
    //model_lat_init(&cfg);
    //model_bw_init(&cfg, cpu);
    //set_read_bw(&cfg, cpu);
    //set_write_bw(&cfg, cpu);
   
    register_self();

    setenv("LD_PRELOAD", ld_preload_path, 1);

    return;

error:
    /* Cannot initialize library -- catastrophic error */

    DBG_LOG(CRITICAL, "Cannot initialize library\n");
}
