#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "workers.h"
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <scif.h>
#define MAX_LINE 1024
#define REQUEST_NUM 1000

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
    //print("f in mic: %f\n", f);
    long long l1=get_time();
    float sum=0;
    int i;
    for(i=0; i<2000000000; i++)
    {
        sum+=f;
    }
    printf("time in cosf=%lld\n", get_time()-l1);
    //print("sum in mic: %f\n", sum);
    return sum;
}

int handler_routine(void *arg, void *result)
{
    runtime_args * pArgs=(runtime_args*)malloc(sizeof(runtime_args));
    memcpy(pArgs, arg, sizeof(runtime_args));
    free(arg);
	long long l1=get_time();
	float sum=cosf(3);
	long long l2=get_time();
	//printf("time=%lld\n", l2-l1); 
    sprintf(pArgs->root_path, "mic: %f", sum);
    pArgs->sender->put_job(pArgs->sender, (void*)pArgs);
    free(pArgs);
	return 0;
}

runtime_args args_array[REQUEST_NUM];
int count=0;
int sender_routine(void *arg, void *result)
{
    runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
    pArgs=(runtime_args*)arg;
	scif_epd_t epd; int err=0;
	int req_pn=41, con_pn;
	int block=1, node=0;
	struct scif_portID portID;
	portID.node=node; portID.port=42;
    if(count==999)
    {
        if((epd=scif_open())<0)
        {
            printf("scif_open failed with error %d\n", (int)epd);
            exit(1);
        }
        if((con_pn=scif_bind(epd, req_pn))<0)
        {
            printf("scif_bind: %s\n", strerror(errno));
            exit(2);
        }
        if(scif_connect(epd, &portID)<0)
        {
            if(ECONNREFUSED==errno)
            {
                printf("scif_connect retry: %s\n", strerror(errno));
            }
            printf("scif_connect: %s\n", strerror(errno));
            //exit(3);
        }	
        //err=scif_send(epd, pArgs, sizeof(runtime_args), block);
        err=scif_send(epd, args_array, REQUEST_NUM*sizeof(runtime_args), block);
        if(err<0)
        {
            printf("scif_send failed with err %s\n", strerror(errno));
            fflush(stdout);
        }
        scif_close(epd);
    }
    else 
    {
        //printf("count=%d: %s\n", count, pArgs->root_path);
        args_array[count++]= *pArgs;
    }
	return 0;
}	

int main(int argc, char **argv)
{
	int epd, newepd;
	int req_pn=21, con_pn;
	int backlog=1024;
	struct scif_portID portID;
	portID.node=0; portID.port=22;
    workers *handler=new_workers(handler_routine, 1000);
//    workers *handler=new_workers(handler_routine, 228);
    workers *sender=new_workers(sender_routine, 1);

	if((epd=scif_open())<0) 
	{
		printf("scif_open failed with error %d \n", errno);
		exit(1);
	}
	if((con_pn=scif_bind(epd, req_pn))<0)
	{
		printf("scif_bind failed with error %d \n", errno);
		exit(2);
	}
	if((scif_listen(epd, backlog))<0)
	{
		printf("scif_listen failed with error %d\n", errno);
		exit(3);
	}
    while(1)
    {
        if(((scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC))<0)&&(errno!=EAGAIN))
        {
            printf("scif_accept failed with errno %d\n", errno);
            exit(4);
        }
        int msg_size=64;   	
        runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        if(pArgs==0)
        {
            free(pArgs);	
            perror("malloc failed");
        }
        memset(pArgs, 0x00, sizeof(runtime_args));
        runtime_args args_array[REQUEST_NUM];
        //int err=scif_recv(newepd, pArgs, sizeof(runtime_args), 1);
        int err=scif_recv(newepd, args_array, REQUEST_NUM*sizeof(runtime_args), 1);
        if(err>0)
        {
            for(int i=0; i<REQUEST_NUM; i++) 
            {
                memcpy(pArgs, args_array+i, sizeof(runtime_args));
                char file_content[MAX_LINE];
                strcpy(file_content, pArgs->root_path);
                printf("file_content in handler main: %s\n", file_content);
                pArgs->sender=sender;
                handler->put_job(handler, (void*)pArgs);
            }
        }
        else if(err<0)
        {
            printf("scif_recv: %s\n", strerror(errno));
            fflush(stdout);		
        }
        free(pArgs);
        scif_close(newepd);
    }
    delete_workers(handler);
    delete_workers(sender);
	scif_close(epd);
	return 0;
}
