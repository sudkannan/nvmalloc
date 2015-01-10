#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void __attribute__((weak)) libfs_init(int read_latency);

void create()
{
    printf("CREATE\n");
    char buf[16384];
    char buf2[16384];
    int  i;
    int  fd = open("/dev/shm/volos/test.dat", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);

    for (i=0; i<1024; i++) {
        buf[i]='-';
    }
    write(fd, buf, 1024);

    lseek(fd, 0, SEEK_SET);
    read(fd, buf2, 1024);
    for (i=0; i<16; i++) {
        printf("%c\n", buf2[i]);
    }
    close(fd);
}

void readfile()
{
    printf("READFILE\n");
    char buf[16384];
    int  i;
    int  fd = open("/dev/shm/volos/test.dat", O_RDONLY);

    read(fd, buf, 1024);
    for (i=0; i<16; i++) {
        printf("%c\n", buf[i]);
    }
    sleep(100);
    printf("CLOSE\n");
    close(fd);
}


void append()
{
    char buf[16384];

    int fd = open("/dev/shm/volos/test.dat", O_WRONLY|O_APPEND);

    write(fd, buf, 1024);

    close(fd);
}


int main()
{
    libfs_init(100);
    printf("%d\n", getpid());
    create();
    readfile();
    //append();

    sleep(100);
}

