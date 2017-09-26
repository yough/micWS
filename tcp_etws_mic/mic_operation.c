#include "common.h"

int connectMIC(void *args, void *result)
{
    int cfd, len;
    struct sockaddr_in server_addr;
    len = sizeof(server_addr);

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=inet_addr("172.31.1.1");
    server_addr.sin_port       =htons(7758);

    if((cfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
     perror("socket error:");
     exit(1);
    }

    if(connect(cfd,(struct sockaddr *)&server_addr,len)<0)
    {
     perror("connect in connectMIC");
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

void server_routine(void* args)
{
    workers *writer=args; 

	int sfd, cfd, len;
    struct sockaddr_in client_addr, server_addr;

    if((sfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
     perror("socket error:");
     exit(1);
    }

    len = sizeof(client_addr);
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(7759);

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
        int err=recv(cfd, pArgs, sizeof(runtime_args), 0);
        if(err>0)
        {
            char file_content[MAX_LINE];
            int pefd=pArgs->efd; 
            int pcfd=pArgs->cfd;
            strcpy(file_content, pArgs->root_path);
           
            writer_args *pWriterArgs=(writer_args*)malloc(sizeof(writer_args));
            pWriterArgs->cfd=pcfd;
            pWriterArgs->efd=pefd;
            strcpy(pWriterArgs->return_result, file_content); 
            writer->put_job(writer, pWriterArgs);
        }
        else if(err<0)
        {
            printf("scif_recv: %s\n", strerror(errno));
            fflush(stdout);		
        }
        free(pArgs);
        close(cfd);
    }
}
