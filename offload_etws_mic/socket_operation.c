#include "common.h"

int init_socket(char *address, int port) 
{
    struct in_addr iaddr;
    struct sockaddr_in saddr;

    int lfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (lfd == -1) 
    {
        printf("socket: %s\n", strerror(errno));
        exit(1);
    }

    int optval = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) 
    {
        printf("setsockopt: %s\n", strerror(errno));
        exit(1);
    }


    memset(&iaddr, 0, sizeof(struct in_addr));
    int err = inet_pton(AF_INET, address, &iaddr);
    if (err != 1) 
    {
        printf("inet_pton: %s\n", strerror(errno));
        exit(1);
    }
    
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr = iaddr;

    err = bind(lfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in));
    if (err == -1) 
    {
        printf("bind: %s\n", strerror(errno));
        exit(1);
    }

    err = listen(lfd, SOMAXCONN);
    if (err == -1) 
    {
        printf("listen: %s\n", strerror(errno));
        exit(1);
    }

    return lfd;
}
