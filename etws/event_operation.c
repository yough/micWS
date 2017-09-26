#include "common.h"

void do_event(struct epoll_event *evp, int lfd, int efd, char* root_path, workers *handler) 
{
    int cfd = evp->data.fd;
    uint32_t events = evp->events;

    if (cfd==lfd)
    {
        accept_add_fd(lfd, efd, evp);
    }
    else if (events & EPOLLIN) 
    {
        cfd = evp->data.fd;
        runtime_args *pArgs;
        pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        pArgs->efd=efd; pArgs->cfd=cfd;
        strcpy(pArgs->root_path, root_path);

        handler->put_job(handler, (void*)pArgs);
/*
        usleep(1000);

        void *arg=malloc(MAX_LINE);
        for(handler->get_result(handler, arg); handler->is_all_zero(handler, arg); handler->get_result(handler, arg))
        {
            char file_content[MAX_LINE];
            runtime_args *p=arg;
            int efd=p->efd; 
            int cfd=p->cfd;
            strcpy(file_content, p->root_path);
            write_response(efd, cfd, file_content);
        }
        free(arg); */
	free(pArgs);
    } 
    else if (events & EPOLLHUP) 
    {
        cfd = evp->data.fd;
        close(cfd);
    }
}

int runtime(void *args, void * result)
{
    char root_path[MAX_LINE];
    memset(root_path, 0, MAX_LINE);
    runtime_args *p=args;
    int efd=p->efd; 
    int cfd=p->cfd;
    strcpy(root_path, p->root_path);
    free(args);
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
            char file_content[MAX_LINE];
            memset(file_content, 0, MAX_LINE);
            if(strlen(arg_name)==0)
            {
                load_static_page(file_path, file_content);
            }
            else
            {
                load_execute_dynamic_page(file_path, arg_name, arg_value, file_content);
            }

            print("content=%s\n", file_content);
            write_response(efd, cfd, file_content); 
            /*runtime_args* pArgs=(runtime_args*)malloc(sizeof(runtime_args));
            pArgs->efd=efd; pArgs->cfd=cfd;
            strcpy(pArgs->root_path, file_content);
            memcpy(result, pArgs, MAX_LINE); */
        }
    }
    else if(ret==-1) 
    {
        shut_remove_conn(cfd, efd); 
        memset(result, 0, MAX_LINE);
    }
}

