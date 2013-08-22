#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <strings.h> // bzero
#include <time.h> // time_t time(time_t *t)

#define LINSTENING_PORT 7777
#define BACKLOG_LEN 256

#define EVENT_NUM 256

typedef enum fdtype_t {
    LISTNER,
    WORKER
} fdtype;

typedef struct epoll_cb_t{
    int fd;
    fdtype type;
    void *cb;
} epoll_cb;

struct epoll_event event_array[EVENT_NUM];
int epfd;

int init_server();
void handle_events(int);
void worker_callback(epoll_cb *,uint32_t);
void listener_callback_accept(epoll_cb *,uint32_t);

int main() {
    epfd = epoll_create(1);
    if (-1 == epfd) {
        fprintf(stderr, "epoll_create failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int listenfd = init_server();

    if (-1 == listenfd) {
        close(epfd);
        exit(EXIT_FAILURE);
    }

    epoll_cb *epcb = (epoll_cb *)malloc(sizeof(epoll_cb));
    if (NULL == epcb) {
        fprintf(stderr, "malloc failed\n");
        close(epfd);
        exit(EXIT_FAILURE);
    }

    epcb->fd = listenfd;
    epcb->type = LISTNER;
    epcb->cb = listener_callback_accept;
    struct epoll_event epevt;
    bzero(&epevt, sizeof(struct epoll_event));
    epevt.events = EPOLLIN;
    epevt.data.ptr= epcb;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd,&epevt)) {
        fprintf(stderr, "epoll_ctl failed to add listener fd: %s\n", strerror(errno));
        close(listenfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    for(;;) {
        int count = epoll_wait(epfd, event_array, EVENT_NUM, -1);
        if (-1 == count) {
            fprintf(stderr, "epoll_wait failed: %s\n", strerror(errno));
        } else {
            handle_events(count);
        }
    }

    close(epfd);
    exit(EXIT_SUCCESS);
}

int init_server() {
    struct sockaddr_in skaddr;
    int skfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if (-1 == skfd) {
        fprintf(stderr, "socket failed: %s\n", strerror(errno));
        return -1;
    }
    bzero(&skaddr, sizeof(struct sockaddr_in));
    skaddr.sin_family = AF_INET;
    skaddr.sin_port = htons(LINSTENING_PORT);
    skaddr.sin_addr.s_addr = INADDR_ANY;

    if ( -1 == bind(skfd, (struct sockaddr*)&skaddr, sizeof(struct sockaddr_in))) {
        fprintf(stderr, "bind failed: %s\n", strerror(errno));
        close(skfd);
        return -1;
    }

    if ( -1 == listen(skfd, BACKLOG_LEN)) {
        fprintf(stderr, "listen failed: %s\n", strerror(errno));
        close(skfd);
        return -1;
    }

    return skfd;
}

void listener_callback_accept(epoll_cb *cb,uint32_t events) {
    int listenfd = cb->fd;
    if (! (events & EPOLLIN)) return;

    fprintf(stdout, "listener_callback_accept\n");
    int fd = accept(listenfd, NULL, NULL); 
    if (-1 == fd) {
        fprintf(stderr, "listen failed: %s\n", strerror(errno));
        return;
    }
    int flags = fcntl(fd, F_GETFL);
    if (-1 == flags) {
        fprintf(stderr, "fcntl F_GETFL failed: %s\n", strerror(errno));
        return;
    }
    flags = flags|O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flags)) {
        fprintf(stderr, "fcntl F_SETFL failed: %s\n", strerror(errno));
        return;
    }

    struct epoll_event epevt;
    epevt.events = EPOLLIN|EPOLLOUT;
    epoll_cb *epcb = (epoll_cb*) malloc(sizeof(epoll_cb));
    if (NULL == epcb) {
        fprintf(stderr, "malloc failed\n");
        close(epfd);
        //TODO: release other resources
        exit(EXIT_FAILURE);
    }
    epcb->fd = fd;
    epcb->type = WORKER;
    epcb->cb = (void*) worker_callback;
    epevt.data.ptr = epcb;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epevt);
}

void worker_callback(epoll_cb *cb, uint32_t events) {
    int fd = cb->fd;
    int in_buf_len = 256;
    int out_buf_len = 256;
    char in_buf[in_buf_len];
    char out_buf[in_buf_len];
    if (events&EPOLLIN) { // read
        int count = read(fd, in_buf, in_buf_len);
        if (-1 == count) {
            fprintf(stderr, "read failed: %s\n", strerror(errno));
            return;
        } else if (0 == count) { // peer socket closed
            fprintf(stdout, "peer socket closed\n");
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            free(cb);
            return;
        }
        in_buf[count] = '\0';
        fprintf(stdout, "read from fd:%d, content:%s\n", fd, in_buf);
        snprintf(out_buf, out_buf_len,"from server,  fd:%d, timestamp:%ld\n",fd, (long)time(NULL));
        write(fd,out_buf, strlen(out_buf));
    }
    if (events&EPOLLOUT){ // write
       snprintf(out_buf, out_buf_len,"hello from fd:%d timestamp:%ld\n",fd, (long)time(NULL));
    }
}

void handle_events(int count) {
    for(int i = 0; i < count; ++i) {
        epoll_cb *epcb = (epoll_cb*) event_array[i].data.ptr;
        void (*cb)(epoll_cb *,uint32_t) = (void(*)(epoll_cb *,uint32_t))epcb->cb;
        cb(epcb, event_array[i].events);
    }
}

