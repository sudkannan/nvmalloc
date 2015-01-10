#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "fs.h"

int main(int argc, char** argv)
{
    int           ret;
    char          buf[4096];
    file_header_t buf_header;
    char*         pathname = argv[1];
    int           fd = open(pathname, O_RDWR);
    int           i;

    if (fd < 0) {
        ret = -1;
        goto error;
    }

    if (pread(fd, &buf_header, sizeof(buf_header), 0) < sizeof(buf_header)) {
        ret = -1;
        goto error;
    }
    for (i=0; i<4096*(buf_header.size/4096); i+= 4096) {
        if (pread(fd, buf, 4096, 4096+i) < 4096) {
            ret = -1;
            goto error;
        }
        pwrite(fd, buf, 4096, i);
    }
    if (i < buf_header.size) {
        pread(fd, buf, buf_header.size % 4096, 4096+i);
        pwrite(fd, buf, buf_header.size % 4096, i);
    }
    ftruncate(fd, buf_header.size);
    return 0;
error:
    fprintf(stderr, "ERROR: %d\n", ret);
    return ret;
}
