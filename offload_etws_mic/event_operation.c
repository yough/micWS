#include "common.h"
#define BATCH_SIZE 100

typedef struct _paraList
{
    int efd;
    int cfd;
    char file_path[100];
    char file_content[100];
    char arg_name[5];
    char arg_value[5];
} paraList;
__attribute__((target(mic))) paraList request_array[BATCH_SIZE];
int paraCount=0;

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
    char file_path[100];
    memset(file_path, 0, 100);

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
            strcpy(request_array[paraCount].file_path, file_path);
            strcpy(request_array[paraCount].arg_name,  arg_name);
            strcpy(request_array[paraCount].arg_value, arg_value);
            request_array[paraCount].efd=efd;
            request_array[paraCount].cfd=cfd;
            paraCount++;
            if(paraCount==BATCH_SIZE)
            {
                #pragma offload target(mic:0), inout(request_array)
                for(int i=0; i<BATCH_SIZE; i++)
                {
                    if(strlen(request_array[i].arg_name)==0)
                    {
                        load_static_page(request_array[i].file_path, request_array[i].file_content);
                    }
                    else
                    {
                        load_execute_dynamic_page(request_array[i].file_path, request_array[i].arg_name, request_array[i].arg_value, request_array[i].file_content);
                    }
                }

                for(int i=0; i<BATCH_SIZE; i++)
                {
                    write_response(request_array[i].efd, request_array[i].cfd, request_array[i].file_content); 
                    printf("file_path[%d]=%s, file_content=%s\n", i, request_array[i].file_path, request_array[i].file_content);
                }
                paraCount=0;
            }
        }
    }
    else if(ret==-1) 
    {
        shut_remove_conn(cfd, efd); 
        memset(result, 0, MAX_LINE);
    }
}

