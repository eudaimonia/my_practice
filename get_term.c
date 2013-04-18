#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LEN 64
char buff[64] = "hello,world!\n";

int acess_term();

int main(int argc, char *argv[]) {
    int fd = acess_term();
    /*
    write(fd, buff, strlen(buff));
    fsync(fd);
    FILE *stream = fdopen(fd, "r");
    fprintf(stream, "%s", buff);
    */
    int read_len = read(fd, buff, LEN);
    if (read_len > 0 && read_len < LEN) {
        buff[read_len] = 0;
        fprintf(stdout, "%s", buff);
        //write(fd, buff, read_len);
        //fsync(fd);
    }

    ioctl(fd, TIOCNOTTY);
    acess_term();
    return 0;
}

int acess_term() {
    int fd = open("/dev/tty", O_RDONLY);
    if ( -1 == fd) {
            fprintf(stderr, "Failed to access terminal device: %s\n", strerror(errno));
            exit(-1);
        }
    fprintf(stdout, "Controlling terminal's fd: %d\n", fd);
    return fd;
}
