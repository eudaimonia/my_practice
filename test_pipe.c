#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define BUF_LEN 256

int main(int argc, char *argv[]) {
    int fd[2];
    int pid;
    char msg[BUF_LEN];
    int data_len = 0;
    if ( -1 == pipe(fd)) {
        fprintf(stderr, "pipe(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (-1 == (pid = fork())) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if ( 0 == pid) { // child process
        if (-1 == close(fd[0])) {// try to close the read end
            fprintf(stderr, "close(): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "In process %ld\n",(long)getpid());
        snprintf(msg, BUF_LEN, "hello from %ld", (long)getpid());
        data_len = strlen(msg);
        if ( -1 == write(fd[1], msg, data_len)) {
            fprintf(stderr, "write(): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {
        if (-1 == close(fd[1])) {// try to close the write end
            fprintf(stderr, "close(): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "In process %ld\n",(long)getpid());
        int read_len = 0;
        int acc_read_len = 0;
        while ((read_len = read(fd[0], msg + read_len, BUF_LEN - 1 - read_len)) > 0) {
            acc_read_len = acc_read_len + read_len;
        }
        if (-1 == read_len) {
            fprintf(stderr, "read(): %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        msg[acc_read_len] = '\0';
        fprintf(stdout, "%ld: %s\n", (long)getpid(), msg);
    }

    return 0;
}
