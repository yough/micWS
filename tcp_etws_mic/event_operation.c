#include "common.h"

void do_event(struct epoll_event *evp, int lfd, int efd, char* root_path, workers *handler, workers* sender, workers* writer) 
{
    int cfd = evp->data.fd;
    uint32_t events = evp->events;

    if(cfd==lfd)
    {
        accept_add_fd(lfd, efd, evp);
    }
    else if(events & EPOLLIN) 
    {
        cfd = evp->data.fd;
        runtime_args *pArgs;
        pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        pArgs->efd=efd; pArgs->cfd=cfd;
        strcpy(pArgs->root_path, root_path);
        pArgs->sender=sender;
        runtime(pArgs, handler, writer);
    } 
    else if(events & EPOLLHUP) 
    {
        cfd = evp->data.fd;
        close(cfd);
    }
}

int handler_request(void *args, void *result)
{
    char root_path[MAX_LINE];
    memset(root_path, 0, MAX_LINE);
    runtime_args *p=(runtime_args*)malloc(sizeof(runtime_args));
    memcpy(p, args, sizeof(runtime_args));

    int efd=p->efd; 
    int cfd=p->cfd;
    workers *writer=p->sender;
    strcpy(root_path, p->root_path);

    char file_content[MAX_LINE];
    memset(file_content, 0, MAX_LINE);

    load_execute_dynamic_page(root_path, "age", "23", file_content);

    writer_args *pWriterArgs=(writer_args*)malloc(sizeof(writer_args));
    pWriterArgs->cfd=cfd;
    pWriterArgs->efd=efd;
    strcpy(pWriterArgs->return_result, file_content); 
    writer->put_job(writer, pWriterArgs); 
    return 0;
}

int flag=0;
int runtime(void *args, workers * handler, workers *writer)
{
    char root_path[MAX_LINE];
    memset(root_path, 0, MAX_LINE);
    runtime_args *p=(runtime_args*)malloc(sizeof(runtime_args));
    memcpy(p, args, sizeof(runtime_args));

    int efd=p->efd; 
    int cfd=p->cfd;
    workers *sender=p->sender;
    strcpy(root_path, p->root_path);
    char file_path[MAX_LINE];
    memset(file_path, 0, MAX_LINE);

    char request[MAX_LINE];
    memset(request, 0, MAX_LINE);

    int ret=read_request(efd, cfd, request); 
    if(ret==0) 
    {
        char arg_name[5]={0};
        char arg_value[5]={0};
        ret=parse_request(request, root_path, file_path, arg_name, arg_value);
        if(ret==0) 
        {
            print("file_path=%s, %s=%s\n", file_path, arg_name, arg_value);
            if(flag==0)
            {//host
                runtime_args* pArgs=(runtime_args*)malloc(sizeof(runtime_args));
                pArgs->efd=efd; pArgs->cfd=cfd;
                pArgs->sender=writer;
                strcpy(pArgs->root_path, file_path);
                handler->put_job(handler, pArgs);                       
                flag=1;
	        }
            else if(flag==1)
            {//mic
                runtime_args* pArgs=(runtime_args*)malloc(sizeof(runtime_args));
                pArgs->efd=efd; pArgs->cfd=cfd;
                strcpy(pArgs->root_path, file_path);
                sender->put_job(sender, pArgs);                       
                flag=2;
            }
            else if(flag==2)
            {//mic
                runtime_args* pArgs=(runtime_args*)malloc(sizeof(runtime_args));
                pArgs->efd=efd; pArgs->cfd=cfd;
                strcpy(pArgs->root_path, file_path);
                sender->put_job(sender, pArgs);                       
                flag=3;
            }
            else if(flag==3)
            {//mic
                runtime_args* pArgs=(runtime_args*)malloc(sizeof(runtime_args));
                pArgs->efd=efd; pArgs->cfd=cfd;
                strcpy(pArgs->root_path, file_path);
                sender->put_job(sender, pArgs);                       
                flag=0;
            } 
        }
    }
    else if(ret==-1) 
    {
        shut_remove_conn(cfd, efd); 
    }
    free(p);
    free(args);
    return 0;
}
