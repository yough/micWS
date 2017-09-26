#include "common.h"

int set_non_blocking(int fd)
{
    int opts;
    opts=fcntl(fd, F_GETFL);
    opts=opts|O_NONBLOCK;
    fcntl(fd, F_SETFL, opts);
    return 0;
}

void add_fd(int efd, int fd, int oneshot)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd=fd;
    event.events=EPOLLIN;
    if(oneshot)
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
//    set_non_blocking(fd); 
}

void del_fd(int efd, int fd, int oneshot)
{
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN|EPOLLET;
    if(oneshot)
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(efd, EPOLL_CTL_DEL, fd, &event);
}

void mod_fd(int efd, int fd, int oneshot)
{
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLOUT|EPOLLET;
    if(oneshot)
    {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
    set_non_blocking(fd); 
}

void reset_oneshot(int efd, int fd)
{
    struct epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLOUT|EPOLLET|EPOLLONESHOT;
    epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
}

int accept_add_fd(int sockfd, int efd, struct epoll_event *evp) 
{
    int connfd = accept4(sockfd, NULL, NULL, SOCK_NONBLOCK);
    if (connfd == -1) 
    {
        if (errno == EAGAIN) 
        {
            return 0;
        } 
        else 
        {
            print("accept4: %s\n", strerror(errno));
            return -1;
        }
    }
    evp->events = EPOLLIN|EPOLLONESHOT;
    evp->data.fd = connfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, evp) == -1) 
    {
        print("epoll_ctl: %s\n", strerror(errno));
    }
    return 0;
}

int shut_remove_conn(int connfd, int efd) 
{

    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));

    ev.events = EPOLLHUP;
    ev.data.fd = connfd;
    if (epoll_ctl(efd, EPOLL_CTL_MOD, connfd, &ev) == -1) 
        print("epoll_ctl: %s\n", strerror(errno));

    if (shutdown(connfd, SHUT_WR) == -1) 
    {
        print("shutdown: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int init_epoll(int sockfd) 
{
    int efd = epoll_create(MAX_EVENT_NUM+1);
    if (efd == -1)  
    {
        print("epoll_create: %s\n", strerror(errno));
        exit(1);
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    int err = epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev);
    if (err == -1) 
    {
        print("epoll_ctl: %s\n", strerror(errno));
        exit(1);
    }

    return efd;
}

