#include "common.h"

int flag=0;

int connectMIC(void *args, void *result)
{
    int cfd, len;
    struct sockaddr_in server_addr;
    len = sizeof(server_addr);

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family     =AF_INET;
    if(flag==0)
    {//node20
        server_addr.sin_addr.s_addr=inet_addr("172.16.16.25");
        flag=1;
    }
    else if(flag==1)
    {//node23
        server_addr.sin_addr.s_addr=inet_addr("172.16.16.28");
        flag=0;
    }
    else if(flag==2)
    {//node25
        server_addr.sin_addr.s_addr=inet_addr("172.16.16.17");
        flag=0;
    }
    else if(flag==3)
    {//ibcu06
        server_addr.sin_addr.s_addr=inet_addr("202.4.157.33");
        flag=4;
    }
    else if(flag==4)
    {//ibcu06
        server_addr.sin_addr.s_addr=inet_addr("202.4.157.34");
        flag=0;
    }
    server_addr.sin_port       =htons(7758);

    if((cfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
        perror("socket error in connectMIC:");
        exit(1);
    }

    if(connect(cfd,(struct sockaddr *)&server_addr,len)<0)
    {
        perror("connect error in connectMIC:");
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

	int lfd, cfd, efd, len;
    struct sockaddr_in client_addr, server_addr;

    if((lfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
    {
        perror("socket error in server_routine:");
        exit(1);
    }

    len = sizeof(client_addr);
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(7759);

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
            print("epoll failure: %s\n", strerror(errno));
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
                    char file_content[MAX_LINE];
                    int epfd=pArgs->efd; 
                    int cpfd=pArgs->cfd;
                    strcpy(file_content, pArgs->root_path);
                    print("file_content=%s\n", file_content);
                   
                    writer_args *pWriterArgs=(writer_args*)malloc(sizeof(writer_args));
                    pWriterArgs->cfd=cpfd;
                    pWriterArgs->efd=epfd;
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
            else if(ev & EPOLLHUP)
            {
                cfd=events[i].data.fd;
                close(cfd);
            }
        }
    }
    close(lfd); 
}
