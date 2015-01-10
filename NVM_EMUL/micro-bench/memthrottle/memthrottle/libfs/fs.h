#ifndef _FS_H
#define _FS_H

typedef struct {
    size_t size;
    size_t max_size;
} file_header_t; 


typedef struct {
    file_header_t* header;
    uintptr_t      base_addr;
    off_t          offset;
} file_t;

typedef struct {
    const char*  path;
    size_t       max_size;
} target_file_t;

#endif
