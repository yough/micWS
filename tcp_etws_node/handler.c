#include "common.h"

float calc(float f);
long long get_time();
int handler_routine(void *arg, void *result);

long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long l=tv.tv_sec*1000*1000+tv.tv_usec;
    return l/1000;
}

float calc(float f)
{
    float sum=0;
    long long l1=get_time();
    for(int i=0; i<10000000; i++)
    {
        sum+=f;
    }
    long long l2=get_time();
    print("time in cosf=%lld\n", l2-l1);
    return sum;
}
int handler_routine(void *arg, void *result)
{
    runtime_args * pArgs=(runtime_args*)malloc(sizeof(runtime_args));
    memcpy(pArgs, arg, sizeof(runtime_args));
    free(arg);
	long long l1=get_time();
	float sum=calc(2.3);
	long long l2=get_time();
    sprintf(pArgs->root_path, "%f", sum);
    pArgs->sender->put_job(pArgs->sender, (void*)pArgs);
    free(pArgs);
	return 0;
}

int sender_routine(void *args, void *result)
{
    int cfd, len;
    struct sockaddr_in server_addr;
    len = sizeof(server_addr);

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=inet_addr("172.16.16.24");
    server_addr.sin_port       =htons(7759);

    if((cfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
     perror("socket error:");
     exit(1);
    }

    if(connect(cfd,(struct sockaddr *)&server_addr,len)<0)
    {
     perror("connect error:");
     exit(1);
    }

    runtime_args *pArgs=(runtime_args*)args;	

    int err=send(cfd, pArgs, sizeof(runtime_args), 0);
    if(err<0)
    {
        printf("send failed with err %s\n", strerror(errno));
        fflush(stdout);
    }
    close(cfd);

	return 0;
}	

int main(int argc, char **argv)
{
	int lfd, efd, cfd, len;
    struct sockaddr_in client_addr, server_addr;
    workers *handler=new_workers(handler_routine, 16);
    workers *sender=new_workers(sender_routine, 2);

    if((lfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
     perror("socket error:");
     exit(1);
    }

    len = sizeof(client_addr);
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(7758);

    if(bind(lfd, (struct sockaddr *)&server_addr,sizeof(server_addr))<0)
    {
     perror("bind error");
     exit(1);
    }

    if(listen(lfd, 1024)<0)
    {
     perror("listen error:");
     exit(1);
    }
    
    efd=init_epoll(lfd);
    struct epoll_event events[MAX_EVENT_NUM];

    while(true)
    {
        int n=epoll_wait(efd, events, MAX_EVENT_NUM, -1);
        if(n==-1)
        {
            printf("epoll failured: %s\n", strerror(errno));
            continue;
        }

        for(int i=0; i<n; i++)
        {
            int cfd=events[i].data.fd;
            uint32_t ev=events[i].events;

            if(cfd==lfd)
            {
                accept_add_fd(lfd, efd, &events[i]);
            }
            else if(ev & EPOLLIN)
            {
                runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
                memset(pArgs, 0, sizeof(runtime_args));
                int err=recv(cfd, pArgs, sizeof(runtime_args), 0);
                if(err>0)
                {
                    print("root_path=%s\n", pArgs->root_path);
                    pArgs->sender=sender;
                    handler->put_job(handler, (void*)pArgs);
                }
                else if(err<0)
                {
                    printf("scif_recv: %s\n", strerror(errno));
                    fflush(stdout);		
                }
                free(pArgs);
                close(cfd);
            }
            else if(ev & EPOLLHUP)
            {
                cfd=events[i].data.fd;
                close(cfd);
            }
        }
    }
    close(lfd);
    delete_workers(handler);
    delete_workers(sender);
    return 0;
}
