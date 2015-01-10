#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <dlfcn.h>
#include <string.h>
#include <fnmatch.h>
#include <numa.h>
#include <numaif.h>
#include "fs.h"
#include "model.h"

#define MAX_FD 1024
#define PAGE_SIZE 4096

#define DEBUG_LEVEL 0

#if (DEBUG_LEVEL > 0)
# define DBG_LOG(level, format, ...)                             \
    if (DEBUG_LEVEL >= level) {                                  \
      fprintf(stderr, "%s <%s,%d>: " format,                     \
              __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);  \
    }  
#else
# define DBG_LOG(level, format, ...) do { } while (0); 
#endif

#define DBG_INTERNAL_ERROR(format, ...)                          \
do {                                                             \
    fprintf(stderr, "INTERNAL_ERROR: %s <%s,%d>: " format,       \
            __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);    \
    abort();                                                     \
} while(0);
    

extern int READ_LATENCY;


target_file_t target_files[] = {
    {"/dev/shm/volos/foster/data*", 16*1024*1024*1024LLU},
//    {"/dev/shm/volos/foster/log/log.*", 1024*1024*1024},
    { NULL, 0}
};

file_t known_fds[MAX_FD];

target_file_t* target_file(const char* pathname)
{
    int i;

    for (i=0; target_files[i].path != NULL; i++) {
        if (fnmatch(target_files[i].path, pathname, FNM_PATHNAME) == 0) {
            return &target_files[i];
        }
    };
    return NULL;
}

size_t roundup(size_t size, size_t block_size)
{
    return ((size / block_size) + 1) * block_size;
}

extern "C" {

int __open(const char *pathname, int flags, ...);
int __stat(const char *path, struct stat *buf);
ssize_t __write(int fd, const void *buf, size_t count);
ssize_t __read(int fd, void *buf, size_t count);
off_t __lseek(int fd, off_t offset, int whence);
int __close(int fd);

ssize_t sys_pread(int fd, void *buf, size_t count, off_t offset)
{
    ssize_t (*fp)(int fd, void *buf, size_t count, off_t offset) = NULL;
    fp = (ssize_t (*)(int, void*, size_t, off_t)) dlsym(RTLD_NEXT, "pread");
    return fp(fd, buf, count, offset);
}

ssize_t sys_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    ssize_t (*fp)(int fd, const void *buf, size_t count, off_t offset) = NULL;
    fp = (ssize_t (*)(int fd, const void *buf, size_t count, off_t offset)) dlsym(RTLD_NEXT, "pwrite");
    return fp(fd, buf, count, offset);
}

ssize_t sys_writev(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t (*fp)(int fd, const struct iovec *iov, int iovcnt) = NULL;
    fp = (ssize_t (*)(int fd, const struct iovec *iov, int iovcnt)) dlsym(RTLD_NEXT, "writev");
    return fp(fd, iov, iovcnt);
}

ssize_t sys_readv(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t (*fp)(int fd, const struct iovec *iov, int iovcnt) = NULL;
    fp = (ssize_t (*)(int fd, const struct iovec *iov, int iovcnt)) dlsym(RTLD_NEXT, "readv");
    return fp(fd, iov, iovcnt);
}

int sys_ftruncate(int fd, off_t length)
{
    ssize_t (*fp)(int fd, off_t length) = NULL;
    fp = (ssize_t (*)(int fd, off_t length)) dlsym(RTLD_NEXT, "ftruncate");
    return fp(fd, length);
}

int sys_truncate(const char *path, off_t length)
{
    int (*fp)(const char *path, off_t length);
    fp = (int (*)(const char *path, off_t length)) dlsym(RTLD_NEXT, "truncate");
    return fp(path, length);
}

int sys_fsync(int fd)
{
    int (*fp)(int fd) = NULL;
    fp = (int (*)(int fd)) dlsym(RTLD_NEXT, "fsync");
    return fp(fd);
}

int sys_fxstat (int __ver, int __fildes, struct stat *__stat_buf)
{
    ssize_t (*fp)(int ver, int fd, struct stat *buf) = NULL;
    fp = (ssize_t (*)(int ver, int fd, struct stat *buf)) dlsym(RTLD_NEXT, "__fxstat");
    return fp(__ver, __fildes, __stat_buf);
}

FILE *sys_fopen(const char *path, const char *mode)
{
    FILE *(*fp)(const char *path, const char *mode);
    fp = (FILE *(*)(const char *path, const char *mode)) dlsym(RTLD_NEXT, "fopen");
    return fp(path, mode);
}

size_t sys_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t (*fp)(void *ptr, size_t size, size_t nmemb, FILE *stream);
    fp = (size_t (*)(void *ptr, size_t size, size_t nmemb, FILE *stream)) dlsym(RTLD_NEXT, "fread");
    return fp(ptr, size, nmemb, stream);
}

size_t sys_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t (*fp)(const void *ptr, size_t size, size_t nmemb, FILE *stream);
    fp = (size_t (*)(const void *ptr, size_t size, size_t nmemb, FILE *stream)) dlsym(RTLD_NEXT, "fwrite");
    return fp(ptr, size, nmemb, stream);
}

int sys_fseek(FILE *stream, long offset, int whence)
{
    int (*fp)(FILE *stream, long offset, int whence);
    fp = (int (*)(FILE *stream, long offset, int whence)) dlsym(RTLD_NEXT, "fseek");
    return fp(stream, offset, whence);
}

int sys_fseeko(FILE *stream, off_t offset, int whence)
{
    int (*fp)(FILE *stream, off_t offset, int whence);
    fp = (int (*)(FILE *stream, off_t offset, int whence)) dlsym(RTLD_NEXT, "fseeko");
    return fp(stream, offset, whence);
}

int sys_fclose(FILE *stream)
{
    int (*fp)(FILE* stream);
    fp = (int (*)(FILE* stream)) dlsym(RTLD_NEXT, "fclose");
    return fp(stream);
}

long sys_ftell(FILE *stream)
{
    long (*fp)(FILE* stream);
    fp = (long (*)(FILE* stream)) dlsym(RTLD_NEXT, "ftell");
    return fp(stream);
}

off_t sys_ftello(FILE *stream)
{
    long (*fp)(FILE* stream);
    fp = (long (*)(FILE* stream)) dlsym(RTLD_NEXT, "ftello");
    return fp(stream);
}

int sys_fileno(FILE *stream)
{
    int (*fp)(FILE* stream);
    fp = (int (*)(FILE* stream)) dlsym(RTLD_NEXT, "fileno");
    return fp(stream);
}

void libfs_set_read_latency(int read_latency)
{
    READ_LATENCY = read_latency;
}

void init() __attribute__ ((constructor));
void init()
{
    char* str;

    if ((str = getenv("LIBFS_READ_LATENCY"))) {
        READ_LATENCY = atoi(str);
    }
}

file_t* fd2file(int fd)
{
    file_t* file;
    if (fd < MAX_FD) {
        file = &known_fds[fd];
        if (file->header) {
            return file;
        }
    }
    return NULL;
}

file_t* stream2file(FILE* stream)
{
    file_t* file = (file_t*) stream;
    if (file > &known_fds[0] && file < &known_fds[MAX_FD]) {
        return file;
    }
    return NULL;
}

void flags2str(int flags, char* buf)
{
    buf[0] = 0;
    if (flags & O_DIRECT) {
        sprintf(buf, "%s O_DIRECT |", buf);
    }
    if (flags & O_SYNC) {
        sprintf(buf, "%s O_SYNC |", buf);
    }
    if (flags & O_APPEND) {
        sprintf(buf, "%s O_APPEND |", buf);
    }
    if (flags & O_CREAT) {
        sprintf(buf, "%s O_CREAT |", buf);
    }
    if (flags & O_TRUNC) {
        sprintf(buf, "%s O_TRUNC |", buf);
    }
    if (flags & O_WRONLY) {
        sprintf(buf, "%s O_WRONLY |", buf);
    } else if (flags & O_RDWR) {
        sprintf(buf, "%s O_RDWR |", buf);
    } else {
        sprintf(buf, "%s O_RDONLY |", buf);
    }
}

int create_and_mmap(const char* pathname, int flags, mode_t mode, size_t max_size)
{
    int     ret;
    void*   addr;
    char    tmp[512];
    /* always open the file using O_RDWR to avoid access problems with mmap */
    flags2str(flags, tmp); 
    DBG_LOG(1, "pathname = %s, flags=%x (%s), max_size=%llu\n", pathname, flags, tmp, max_size);
    flags = flags & ~O_RDONLY;
    flags = flags & ~O_WRONLY;
    flags = flags | O_RDWR;
    int     fd = __open(pathname, flags, mode);
    assert(fd < MAX_FD);
    file_t* file = &known_fds[fd];
    assert(file->header == NULL);
    size_t  size = roundup(max_size, PAGE_SIZE) + PAGE_SIZE;
    if ((ret = sys_ftruncate(fd, size)) < 0) {
        goto error;
    }
    if ((addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        ret = -1;
        goto error;
    }
    file->header = (file_header_t*) addr;
    file->header->size = 0;
    file->header->max_size = roundup(max_size, PAGE_SIZE);
    file->base_addr = (uintptr_t) addr + PAGE_SIZE;
    file->offset = 0;
    return fd;

error:
    //unlink(pathname);
    return ret;
}

int open_and_mmap(const char* pathname, int flags)
{
    int           ret;
    void*         addr;
    int           fd;
    file_header_t buf_header;
    char          tmp[512];
    int           trunc = 0;
    file_t*       file = NULL;

    /* always open the file using O_RDWR to avoid access problems with mmap */
    flags2str(flags, tmp); 
    DBG_LOG(1, "pathname=%s, flags=%x (%s)\n", pathname, flags, tmp);
    flags = flags & ~O_RDONLY;
    flags = flags & ~O_WRONLY;
    flags = flags | O_RDWR;
    if (flags & O_TRUNC) {
        flags = flags & ~O_TRUNC;
        trunc = 1;
    }
    flags = flags & ~O_DIRECT;
    if ((fd = __open(pathname, flags)) < 0) {
        ret = fd;
        goto error;
    }
    assert(fd < MAX_FD);
    file = &known_fds[fd];
    assert(file->header == NULL);
    if (sys_pread(fd, &buf_header, sizeof(buf_header), 0) < ((ssize_t) sizeof(buf_header))) {
        ret = -1;
        goto error;
    }
    if ((addr = mmap(0, buf_header.max_size + PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "FAILED: buf_header.max_size=%zu\n", buf_header.max_size);
        ret = -1;
        goto error;
    }
    file->header = (file_header_t*) addr;
    if (trunc) {
        file->header->size = 0;
    }
    file->base_addr = (uintptr_t) addr + PAGE_SIZE;
    if (flags & O_APPEND) {
        file->offset = file->header->size;
    } else {
        file->offset = 0;
    } 
    return fd;

error:
    return ret;
}

int internal_open(const char *pathname, int flags, mode_t mode, size_t max_size)
{
    if (flags & O_CREAT) {
        int         fd;
        struct stat stat_buf;
        if (__stat(pathname, &stat_buf) == 0) {
            /* file already exists */
            fd = open_and_mmap(pathname, flags);
        } else {
            fd = create_and_mmap(pathname, flags, mode, max_size);
        }
        DBG_LOG(1, "pathname=%s, fd=%d, base_addr=%p\n", pathname, fd, fd >=0 && fd2file(fd)->header ? (void*) (fd2file(fd)->base_addr): NULL);
        return fd;
    } else {
        int fd = open_and_mmap(pathname, flags);
        DBG_LOG(1, "pathname=%s, fd=%d, base_addr=%p\n", pathname, fd, fd >=0 && fd2file(fd)->header ? (void*) (fd2file(fd)->base_addr): NULL);
        return fd;
    }
}

int open(const char *pathname, int flags, ...)
{
    mode_t         have_mode = 0;
    mode_t         mode = 0;
    target_file_t* tf;

    if (flags & O_CREAT) {
        va_list ap;

        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        have_mode = 1;
    } 

    /* interpose? */
    if ((tf = target_file(pathname))) {
        return internal_open(pathname, flags, mode, tf->max_size);
    }
    if (have_mode) {
        return __open(pathname, flags, mode);
    } else {
        return __open(pathname, flags);
    }
}


int close(int fd)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(1, "fd=%d\n", fd);
        munmap((void*) (file->base_addr-PAGE_SIZE), file->header->max_size+PAGE_SIZE);
        file->header = NULL;
        return 0;
    }
    return __close(fd);
}


int fstat(int /* fd */, struct stat * /* buf */)
{
    DBG_INTERNAL_ERROR("Not implemented\n")
}

int stat(const char * /* path */, struct stat * /* buf */)
{
    DBG_INTERNAL_ERROR("Not implemented\n")
}

int __fxstat (int __ver, int __fildes, struct stat *__stat_buf)
{
    file_t* file;
    int     ret;
    if ((file = fd2file(__fildes))) {
        DBG_LOG(2, "fd=%d\n", __fildes);
        if ((ret = sys_fxstat(__ver, __fildes, __stat_buf)) < 0) {
            return ret;
        }
        __stat_buf->st_size = file->header->size;
        return 0;
    }
    return sys_fxstat(__ver, __fildes, __stat_buf);
}

int fsync(int fd)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        /* NOP */
        return 0;
    }
    return sys_fsync(fd);   
}

int ftruncate(int fd, off_t length)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(1, "fd=%d, length=%llu\n", fd, length)
        assert(((size_t) length) < file->header->max_size);
        file->header->size = length;
        return 0;   
    }
    return sys_ftruncate(fd, length);
}

int truncate(const char *path, off_t length)
{
    file_header_t  buf_header;
    target_file_t* tf;
    int            fd;

    if ((tf = target_file(path))) {
        DBG_LOG(1, "path = %s , length = %llu\n", path, length);
        if ((fd = __open(path, O_RDWR)) < 0) {
            return -1;
        }
        if (sys_pread(fd, &buf_header, sizeof(buf_header), 0) < ((ssize_t) sizeof(buf_header))) {
            return -1;
        }
        buf_header.size = length;
        if (sys_pwrite(fd, &buf_header, sizeof(buf_header), 0) < ((ssize_t) sizeof(buf_header))) {
            return -1;
        }
        __close(fd);
        return 0;
    }
    return sys_truncate(path, length);
}

ssize_t internal_write(file_t* file, const void *buf, size_t count)
{
    memcpy((void*) (file->base_addr+file->offset), buf, count);
    file->offset+=count;
    if (file->header->size < ((size_t) file->offset)) {
        file->header->size = file->offset;
    } 
    return count;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(2, "fd=%d, base_addr=%p, offset=%llu, count=%llu, size=%llu\n", fd, file->base_addr, file->offset, count, file->header->size);
        return internal_write(file, buf, count);
    }
    return __write(fd, buf, count);
}

ssize_t internal_read(file_t* file, void *buf, size_t count)
{
    if (file->offset+count > file->header->size) {
        count = (file->header->size > ((size_t) file->offset)) ? file->header->size - file->offset : 0;
    }
    if (count > 0) {
        memcpy(buf, (void*) (file->base_addr+file->offset), count);
        file->offset+=count;
    }
    return count;
}


ssize_t read(int fd, void *buf, size_t count)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(1, "fd=%d, base_addr=%p, offset=%llu, count=%llu, size=%llu\n", fd, file->base_addr, file->offset, count, file->header->size);
        return internal_read(file, buf, count);
    }
    return __read(fd, buf, count);
}


off_t lseek(int fd, off_t offset, int whence)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(1, "fd=%d, base_addr=%p, offset=%llu, size=%llu\n", fd, file->base_addr, file->offset,  file->header->size);
        switch(whence) {
            case SEEK_SET:
                file->offset = offset;
                break;
            case SEEK_CUR:
                file->offset += offset;
                break;
            case SEEK_END:
                file->offset = file->header->size + offset;
        }
        return file->offset;
    }
    return __lseek(fd, offset, whence);    
}


ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(2, "fd=%d, base_addr=%p, offset=%llu, count=%llu, size=%llu\n", fd, file->base_addr, offset, count, file->header->size);
        if (offset+count > file->header->size) {
            count = (file->header->size > ((size_t) offset)) ? file->header->size - offset : 0;
        }
        if (count>0) {
            nvm_memcpy(buf, (void*) (file->base_addr+offset), count);
        }
        return count;
    }
    return sys_pread(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(2, "fd=%d, base_addr=%p, offset=%llu, count=%llu, size=%llu\n", fd, file->base_addr, offset, count, file->header->size);
        if (offset + count > file->header->max_size) {
            count = file->header->max_size - offset;
        }
        memcpy((void*) (file->base_addr+offset), buf, count);
        if (file->header->size < offset+count) {
            file->header->size = offset+count;
        } 
        return count;
    }
    return sys_pwrite(fd, buf, count, offset);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    int     i;
    file_t* file;
    ssize_t count = 0;
    if ((file = fd2file(fd))) {
        DBG_LOG(2, "fd=%d, base_addr=%p, offset=%llu, size=%llu, iovcnt=%d\n", fd, file->base_addr, file->offset, file->header->size, iovcnt);
        for (i=0; i<iovcnt; i++) {
            count+=internal_write(file, iov[i].iov_base, iov[i].iov_len);
        }
        return count;
    }
    return sys_writev(fd, iov, iovcnt);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    DBG_INTERNAL_ERROR("buggy\n")
    file_t* file;
    if ((file = fd2file(fd))) {
        DBG_LOG(2, "fd=%d, base_addr=%p, offset=%llu, size=%llu, iovcnt=%d\n", fd, file->base_addr, file->offset, file->header->size, iovcnt);
        abort();
    }
    return sys_readv(fd, iov, iovcnt);
}

/*
 * STREAM OPERATIONS
 */

FILE *fopen(const char *path, const char *mode)
{
    file_t*        file;
    target_file_t* tf;

    /* interpose? */
    if ((tf = target_file(path))) {
        int flags = 0;
        if (strcmp(mode, "a") == 0) {
            flags = O_CREAT | O_APPEND | O_RDWR;
        } else if (strcmp(mode, "r") == 0) {
            flags = O_RDONLY;
        }
        int fd = internal_open(path, flags, S_IRWXU, tf->max_size);
        file = &known_fds[fd];
        DBG_LOG(1, "path=%s, mode=%s, size=%llu\n", path, mode, file->header->size);
        return (FILE*) file;
    } 
    return sys_fopen(path, mode);
}


size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        DBG_LOG(1, "stream=%p\n", stream);
        return internal_read(file, ptr, nmemb*size) / size;
    }
    return sys_fread(ptr, size, nmemb, stream);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        DBG_LOG(1, "stream=%p, size=%llu, nmemb=%llu, count=%llu\n", stream, size, nmemb, size*nmemb);
        return internal_write(file, ptr, nmemb*size) / size;
    }
    return sys_fwrite(ptr, size, nmemb, stream);
}

int fseeko(FILE *stream, off_t offset, int whence)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        DBG_LOG(1, "stream=%p\n", stream);
        switch (whence) {
            case SEEK_SET:
                file->offset = offset;
                break;
            case SEEK_CUR:
                file->offset += offset;
                break;
            case SEEK_END:
                file->offset = file->header->size + offset;
        }
        return 0;
    }
    return sys_fseeko(stream, offset, whence);
}

int fseek(FILE *stream, long offset, int whence)
{
    return fseeko(stream, offset, whence);
}

long ftell(FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        return file->offset;
    }
    return sys_ftell(stream);
}

off_t ftello(FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        return file->offset;
    }
    return sys_ftello(stream);
}


int fileno(FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        return file - known_fds;
    }
    return sys_fileno(stream);
}

int fclose(FILE *stream)
{
    file_t* file;
    if ((file = stream2file(stream))) {
        DBG_LOG(1, "stream=%p\n", stream);
        munmap((void*) (file->base_addr-PAGE_SIZE), file->header->max_size+PAGE_SIZE);
        file->header = NULL;
        return 0;
    }
    return sys_fclose(stream);
}

}
