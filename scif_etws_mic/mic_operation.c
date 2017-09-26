#include "common.h"
#define REQUEST_NUM 1000

runtime_args args_array[REQUEST_NUM];
int count=0;
int connectMIC(void *args, void *result)
{
	scif_epd_t epd; int err=0;
	int req_pn=22, con_pn;
	int msg_size=30;
	struct scif_portID portID;
	portID.node=1; portID.port=21;
	struct timeval tv, tv1;

    if(count==999)
    {
        if((epd=scif_open())<0)
        {
            printf("scif_open failed with error %d\n", (int)epd);
            exit(1);
        }

        if((con_pn=scif_bind(epd, req_pn))<0)
        {
            printf("scif_bind in connectMIC: %s, con_pn=%d\n", strerror(errno), con_pn);
            exit(2);
        }
        
        if(scif_connect(epd, &portID)<0)
        {
            if(ECONNREFUSED==errno)
            {
                printf("scif_connect retry: %s\n", strerror(errno));
            }
            printf("scif_connect in connectMIC: %s\n", strerror(errno));
        }	

        //runtime_args *pArgs=(runtime_args*)args;	
        //err=scif_send(epd, pArgs, sizeof(runtime_args), 1);
        runtime_args *pArgs=(runtime_args*)args;	
        //printf("count==9: %s\n", pArgs->root_path);
        err=scif_send(epd, args_array, REQUEST_NUM*sizeof(runtime_args), 1);
        if(err<0)
        {
            printf("scif_send failed with err %s\n", strerror(errno));
            fflush(stdout);
        }
       
        scif_close(epd);
        count=0;
    }
    else 
    {
        runtime_args *pArgs=(runtime_args*)args;	
        //printf("count=%d: %s\n", count, pArgs->root_path);
        args_array[count++]= *pArgs;
    }

	return err;
}	

void server_routine(void* args)
{
    workers *writer=args; 

    int i=0;
	int epd, newepd;
	int req_pn=42, con_pn;
	int backlog=1024;
	struct scif_portID portID;
	portID.node=1;
	portID.port=41;
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
            printf("scif_accept failed with errno %s\n", strerror(errno));
            exit(4);
        }
        int block=1;  	
        runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        //int err=scif_recv(newepd, pArgs, sizeof(runtime_args), block);
        runtime_args args_array[REQUEST_NUM];
        int err=scif_recv(newepd, args_array, REQUEST_NUM*sizeof(runtime_args), 1);
        if(err>0)
        {
            for(int i=0; i<REQUEST_NUM; i++)
            {
                memcpy(pArgs, args_array+i, sizeof(runtime_args));
                char file_content[MAX_LINE];
                int efd=pArgs->efd; 
                int cfd=pArgs->cfd;
                strcpy(file_content, pArgs->root_path);
                print("file_content in server_routine:%s\n", file_content);
               
                writer_args *pWriterArgs=(writer_args*)malloc(sizeof(writer_args));
                pWriterArgs->cfd=cfd;
                pWriterArgs->efd=efd;
                strcpy(pWriterArgs->return_result, file_content); 
                writer->put_job(writer, pWriterArgs); 

            }
        }
        else if(err<0)
        {
            printf("scif_recv: %s\n", strerror(errno));
            fflush(stdout);		
        }
        scif_close(newepd);
    }
	scif_close(epd);
}
