#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "cpu/cpu.h"
#include "config.h"
#include "error.h"
#include "stat.h"

typedef struct {
    unsigned int throttle_reg_val[MAX_THROTTLE_VALUE]; 
    double bandwidth[MAX_THROTTLE_VALUE];
    int npoints;
} bw_model_t;

static bw_model_t read_bw_model;
static bw_model_t write_bw_model; 


int train_model(cpu_model_t* cpu, char model_type, bw_model_t* bw_model)
{
    double x[MAX_THROTTLE_VALUE];
    double best_rate[4];
    double m;
    int    i;
    int    throttle_reg_val;

    int ntimes = 2;
    int min_number_throttle_points = 5;
    double stop_slope = 1;
    // FIXME: how to select node?
    int node = 0;

    for (i=0; i < MAX_THROTTLE_VALUE; i++) {
        throttle_reg_val = i+1;
        //FIXME: set_throttle_register must allow selecting read or write bandwidth
        if (model_type == 'r') {
            //cpu->set_throttle_register(node, throttle_reg_val);
            cpu->set_throttle_register(node, THROTTLE_DDR_ACT, 0x80ff);
        } else if (model_type == 'w') {
            //cpu->set_throttle_register(node, throttle_reg_val);
            cpu->set_throttle_register(node, THROTTLE_DDR_ACT, 0x80ff);
        }
        // FIXME: stream_measure_bw is deprecated.
        //stream_measure_bw(ntimes, best_rate);
        cpu->set_throttle_register(node, THROTTLE_DDR_ACT, 0xffff);
        bw_model->throttle_reg_val[i] = throttle_reg_val;
        bw_model->bandwidth[i] = avg(best_rate, 4); // we use the average of four different bw values: copy scale add triad
        x[i] = (double) throttle_reg_val; // slope calculation requires values of type double
        if (i > min_number_throttle_points) {
            m = slope(&x[i-min_number_throttle_points], 
                      &bw_model->bandwidth[i-min_number_throttle_points], 
                      min_number_throttle_points);
            if (m < stop_slope) {
                break;
            }
        }
    }
    bw_model->npoints = i;
    return E_SUCCESS;
}

static int load_model(const char* path, const char* prefix, bw_model_t* bw_model)
{
    FILE *fp;
    char *line = NULL;
    char str[64];
    size_t len = 0;
    ssize_t read;
    int i;
    int x;
    double y;
    int found_points;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return E_ERROR;
    }

    DBG_LOG(INFO, "Loading %s bandwidth model from %s\n", prefix, path);
    for (i = 0, found_points = 0; (read = getline(&line, &len, fp)) != -1; ) {
        if (strstr(line, prefix)) {
            sscanf(line, "%s\t%d\t%lf", str, &x, &y);
            DBG_LOG(INFO, "throttle reg: %d, bandwidth: %f\n", x, y);
            bw_model->throttle_reg_val[found_points] = x;
            bw_model->bandwidth[found_points] = y;
            found_points++;
        }
    }
    free(line);
    if (found_points) {
        bw_model->npoints = found_points;
    } else {
        DBG_LOG(INFO, "No %s bandwidth model found in %s\n", prefix, path);
        return E_ERROR;
    }
    fclose(fp);
    return E_SUCCESS;
}

static int save_model(const char* path, const char* prefix, bw_model_t* bw_model)
{
    int i;
    FILE *fp;

    fp = fopen(path, "a");
    if (fp == NULL) {
        return E_ERROR;
    }

    DBG_LOG(INFO, "Saving %s bandwidth model into %s\n", prefix, path);
    for (i=0; i<bw_model->npoints; i++) {
        int x = bw_model->throttle_reg_val[i];
        double y = bw_model->bandwidth[i];
        DBG_LOG(INFO, "throttle reg: %d, bandwidth: %f\n", x, y);
        fprintf(fp, "%s\t%d\t%f\n", prefix, x, y);
    }
    fclose(fp);
    return E_SUCCESS;
}


int model_bw_init(config_t* cfg, cpu_model_t* cpu)
{
    char* model_file;

    if (__cconfig_lookup_string(cfg, "bandwidth.model", &model_file) == CONFIG_TRUE) {
        if (load_model(model_file, "read", &read_bw_model) != E_SUCCESS) {
            train_model(cpu, 'r', &read_bw_model);
            save_model(model_file, "read", &read_bw_model);
        }
        if (load_model(model_file, "write", &write_bw_model) != E_SUCCESS) {
            train_model(cpu, 'w', &write_bw_model);
            save_model(model_file, "write", &write_bw_model);
        }
    }
}

int find_data_point(bw_model_t* model, double target_bw, unsigned int* point)
{
    int i;

    *point = 0;
    for (i=1; i<model->npoints; i++) {
        if (model->bandwidth[i] > target_bw) {
            break;
        }
        *point = i;
    }
    return E_SUCCESS;
}

int __set_write_bw(cpu_model_t* cpu, uint64_t target_bw)
{
    // FIXME: how to select node?
    int node = 0x1f;
    int ret;
    unsigned int point;

    if ((ret = find_data_point(&write_bw_model, (double) target_bw, &point)) != E_SUCCESS) {
        return ret;
    }
    DBG_LOG(INFO, "Setting throttle reg: %d, target write bandwidth: %" PRIu64 ", actual write bandwidth: %" PRIu64 "\n", write_bw_model.throttle_reg_val[point], target_bw, (uint64_t) write_bw_model.bandwidth[point]);
    //FIXME: set_throttle_register must allow selecting read or write bandwidth
    uint16_t val;
    cpu->get_throttle_register(node, THROTTLE_DDR_ACT, &val);
    printf("throttle_register == %x\n", val);
    cpu->set_throttle_register(node, THROTTLE_DDR_ACT, write_bw_model.throttle_reg_val[point]);
    
    return E_SUCCESS;
}

int set_write_bw(config_t* cfg, cpu_model_t* cpu)
{
    int target_bw;
    __cconfig_lookup_int(cfg, "bandwidth.write", &target_bw);

    return __set_write_bw(cpu, target_bw);
}

int __set_read_bw(cpu_model_t* cpu, uint64_t target_bw)
{
    // FIXME: how to select node?
    int bus_id = 0;
    int ret;
    unsigned int point;

    if ((ret = find_data_point(&read_bw_model, (double) target_bw, &point)) != E_SUCCESS) {
        return ret;
    }
    DBG_LOG(INFO, "Setting throttle reg: %d, target read bandwidth: %" PRIu64 ", actual read bandwidth: %" PRIu64 "\n", read_bw_model.throttle_reg_val[point], target_bw, (uint64_t) read_bw_model.bandwidth[point]);
    //FIXME: set_throttle_register must allow selecting read or write bandwidth
    cpu->set_throttle_register(bus_id, THROTTLE_DDR_ACT, read_bw_model.throttle_reg_val[point]);
    
    return E_SUCCESS;
}

int set_read_bw(config_t* cfg, cpu_model_t* cpu)
{
    int target_bw;
    __cconfig_lookup_int(cfg, "bandwidth.read", &target_bw);

    return __set_read_bw(cpu, target_bw);
}
