#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "workers.h"
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#define MAX_LINE 1024
#define REQUEST_NUM 10
#define SERVICE_THREAD_NUM 100

float cosf(float f);
long long get_time();
int handler_routine(void *arg, void *result);

typedef struct _runtime_args
{
    int efd;
    int cfd;
    char root_path[30];
    workers *sender;
} runtime_args;

long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long l=tv.tv_sec*1000*1000+tv.tv_usec;
    return l/1000;
}
float cosf(float f)
{
    long long l1=get_time();
    float sum=0;
    int i;
    for(i=0; i<10000000; i++)
    {
        sum+=f;
    }
    return sum;
}
int handler_routine(void *arg, void *result)
{
    runtime_args * pArgs=(runtime_args*)malloc(sizeof(runtime_args));
    memcpy(pArgs, arg, sizeof(runtime_args));
    free(arg);
	long long l1=get_time();
	float sum=cosf(2.3);
	long long l2=get_time();
	//printf("time=%lld\n", l2-l1); 
    sprintf(pArgs->root_path, "%f", sum);
    pArgs->sender->put_job(pArgs->sender, (void*)pArgs);
    free(pArgs);
	return 0;
}

runtime_args args_array[REQUEST_NUM];
int count=0;
int sender_routine(void *args, void *result)
{
    int cfd, len;
    struct sockaddr_in server_addr;
    len = sizeof(server_addr);

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=inet_addr("172.31.1.254");
    server_addr.sin_port       =htons(7759);

    runtime_args *pArray=(runtime_args*)malloc(sizeof(runtime_args));
    pArray=(runtime_args*)args;
    args_array[count++]=*pArray;
    //printf("in sender routine, cfd=%d, root_path=%s\n", pArray->cfd, pArray->root_path);
    if(count==REQUEST_NUM)
    {
        if((cfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
        {
         perror("socket error:");
         exit(1);
        }

        if(connect(cfd,(struct sockaddr *)&server_addr,len)<0)
        {
         perror("connect in sender_routine");
         exit(1);
        }

        //runtime_args *pArgs=(runtime_args*)args;	
        //int err=send(cfd, pArgs, sizeof(runtime_args), 0);

        int err=send(cfd, args_array, REQUEST_NUM*sizeof(runtime_args), 0);

        if(err<0)
        {
            printf("send failed with err %s\n", strerror(errno));
            fflush(stdout);
        }
        close(cfd);
        count=0;
    }
	return 0;
}	

int main(int argc, char **argv)
{
	int sfd, cfd, len;
    struct sockaddr_in client_addr, server_addr;
    workers *handler=new_workers(handler_routine, SERVICE_THREAD_NUM);
    workers *sender=new_workers(sender_routine, 1);

    if((sfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
     perror("socket error:");
     exit(1);
    }

    len = sizeof(client_addr);
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(7758);

    if(bind(sfd, (struct sockaddr *)&server_addr,sizeof(server_addr))<0)
    {
     perror("bind error");
     exit(1);
    }

    if(listen(sfd, 1024)<0)
    {
     perror("listen error:");
     exit(1);
    }
    
    while(1)
    {
        cfd=accept(sfd,(struct sockaddr *)&client_addr,&len);           
        if((cfd<0)&&(errno!=EAGAIN))
        {
            printf("accept failed with errno %d\n", errno);
            exit(4);
        }

        runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        memset(pArgs, 0, sizeof(runtime_args));
        //int err=recv(cfd, pArgs, sizeof(runtime_args), 0);
        runtime_args args_array[REQUEST_NUM];
        int err=recv(cfd, args_array, REQUEST_NUM*sizeof(runtime_args), 0);
        if(err>0)
        {
            for(int i=0; i<REQUEST_NUM; i++)
            {
                printf("cfd[%d]=%d\n", i, args_array[i].cfd);
                memcpy(pArgs, args_array+i, sizeof(runtime_args));
                char file_content[MAX_LINE];
                pArgs->sender=sender;
                handler->put_job(handler, (void*)pArgs);
            }
        }
        else if(err<0)
        {
            printf("recv: %s\n", strerror(errno));
            fflush(stdout);		
        }
        free(pArgs);
        close(cfd);
    }
    delete_workers(handler);
    delete_workers(sender);
	return 0;
}
