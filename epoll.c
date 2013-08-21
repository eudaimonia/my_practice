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
void worker_callback(int,uint32_t);
void listener_callback_accept(int,uint32_t);

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

void listener_callback_accept(int listenfd,uint32_t events) {
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

void worker_callback(int fd, uint32_t events) {
    if (events&EPOLLIN) { // read
    }
    if (events&EPOLLOUT){ // write
    }
}

void handle_events(int count) {
    for(int i = 0; i < count; ++i) {
        epoll_cb *epcb = (epoll_cb*) event_array[i].data.ptr;
        if (epcb->type == LISTNER) { // handle event on linster
            void (*cb)(int,uint32_t) = (void(*)(int,uint32_t))epcb->cb;
            cb(epcb->fd, event_array[i].events);
            free(epcb);
        } else {
            // TODO
        }
    }
}

