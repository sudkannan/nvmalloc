#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "cpu.h"
#include "dev.h"
#include "error.h"
#include "known_cpus.h"

// Mainline architectures and processors available here:
// https://software.intel.com/en-us/articles/intel-architecture-and-processor-identification-with-cpuid-model-and-family-numbers
//
// It turns out that CPUID is not an accurate approach to identifying a 
// processor as different processors may have the same CPUID.
// So instead we rely on the brand string returned by /proc/cpuinfo:model_name

#define MASK(msb, lsb) (~((~0) << (msb+1)) & ((~0) << lsb))
#define EXTRACT(val, msb, lsb) ((MASK(msb, lsb) & val) >> lsb)  
#define MODEL(eax) EXTRACT(eax, 7, 4)
#define EXTENDED_MODEL(eax) EXTRACT(eax, 19, 16)
#define MODEL_NUMBER(eax) ((EXTENDED_MODEL(eax) << 4) | MODEL(eax))
#define FAMILY(eax) EXTRACT(eax, 11, 8)

void cpuid(unsigned int info, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx)
{
    __asm__(
        "cpuid;"                                           
        :"=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        :"a" (info) 
    );
}

// caller is responsible for freeing memory allocated by this function
char* cpu_model_name()
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        return NULL;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, "model name")) {
            char* buf = malloc(strlen(line) - strlen("model name"));
            strcpy(buf, &line[strlen("model name")+3]);
            free(line);
            return buf;
        }
    }

    free(line);
    return NULL;
}

int match(const char* to_match, const char* regex_text)
{
    int ret;
    const char* p = to_match;
    regex_t regex;
    regmatch_t m[1];

    if ((ret = regcomp(&regex, regex_text, REG_EXTENDED|REG_NEWLINE)) != 0) {
        return E_ERROR;
    }
    if ((ret = regexec(&regex, p, 1, m, 0))) {
        return E_ERROR; // no match
    } 
    return E_SUCCESS;
}

cpu_model_t* cpu_model()
{
    int i;
    char* model_name;
    int matched = -1;

    if ((model_name = cpu_model_name()) == NULL) {
        return NULL;
    }

    for (i=0; known_cpus[i] != 0; i++) {
        cpu_model_t* c = known_cpus[i];
	
        if (match(model_name, c->desc.vendor_name) == E_SUCCESS && 
            match(model_name, c->desc.brand_name) == E_SUCCESS && 
            match(model_name, c->desc.brand_processor_number) == E_SUCCESS) 
        {
            matched = i;
            break;
        }       
    }
    free(model_name);

    if (matched < 0) {
        return NULL;
    }

    return known_cpus[matched];
}
