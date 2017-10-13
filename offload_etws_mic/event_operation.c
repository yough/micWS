#include "common.h"
#define BATCH_SIZE 200

__attribute__((target(mic))) paraList request_array[BATCH_SIZE];
int paraCount=0;

void do_event(struct epoll_event *evp, int lfd, int efd, char* root_path, workers *handler, workers *handler_cpu) 
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
        runtime_args *pArgs=(runtime_args*)malloc(sizeof(runtime_args));
        pArgs->efd=efd; pArgs->cfd=cfd;
        strcpy(pArgs->root_path, root_path);
        pArgs->fun=(workers*)malloc(sizeof(workers));
        pArgs->fun=handler_cpu;

        runtime(pArgs, NULL);
    } 
    else if (events & EPOLLHUP) 
    {
        cfd = evp->data.fd;
        close(cfd);
    }
}

int flag=0;
int runtime(void *args, void * result)
{
    char root_path[MAX_LINE];
    memset(root_path, 0, MAX_LINE);
    runtime_args *p=(runtime_args*)args;
    int efd=p->efd; 
    int cfd=p->cfd;
    workers *handler_cpu=p->fun;
    strcpy(root_path, p->root_path);
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
            paraList pl;
            strcpy(pl.file_path, file_path);
            strcpy(pl.arg_name,  arg_name);
            strcpy(pl.arg_value, arg_value);
            pl.efd=efd; pl.cfd=cfd;

/*            if(flag==0)
            {//host
                handler_cpu->put_job(handler_cpu, (void*)&pl);
                flag=1;
            }
            else if(flag==1)
            {//mic
                handler_on_mic(pl);
                flag=2;
            }
            else if(flag==2)
            {//mic
                handler_on_mic(pl);
                flag=3;
            }
            else if(flag==3)
            {//mic
                handler_on_mic(pl);
                flag=0;
            }
*/
            handler_cpu->put_job(handler_cpu, (void*)&pl);
            //handler_on_mic(pl);

        }
    }
    else if(ret==-1) 
    {
        shut_remove_conn(cfd, efd); 
        memset(result, 0, MAX_LINE);
    }
    free(args);
}

int handler_on_mic(paraList pl)
{
    strcpy(request_array[paraCount].file_path, pl.file_path);
    strcpy(request_array[paraCount].arg_name,  pl.arg_name);
    strcpy(request_array[paraCount].arg_value, pl.arg_value);
    request_array[paraCount].efd=pl.efd;
    request_array[paraCount].cfd=pl.cfd;
    paraCount++;
    if(paraCount==BATCH_SIZE)
    {
        #pragma offload target(mic:0), inout(request_array)
        #pragma omp parallel for
        for(int i=0; i<BATCH_SIZE; i++)
        {
            if(strlen(request_array[i].arg_name)==0)
            {
                execute_static_page_on_mic(request_array[i].file_path, request_array[i].file_content);
            }
            else
            {
                execute_dynamic_page_on_mic(request_array[i].file_path, request_array[i].arg_name, request_array[i].arg_value, request_array[i].file_content);
            }
        }

        for(int i=0; i<BATCH_SIZE; i++)
        {
            write_response(request_array[i].efd, request_array[i].cfd, request_array[i].file_content); 
            print("file_path[%d]=%s, file_content=%s\n", i, request_array[i].file_path, request_array[i].file_content);
        }
        paraCount=0;
    }
}

int handler_on_cpu(void *ppl, void *result)
{
    paraList *pl=(paraList*)ppl;
    print("file_path=%s, %s=%s\n", pl->file_path, pl->arg_name, pl->arg_value);
    if(strlen(pl->arg_name)==0)
    {
        load_static_page(pl->file_path, pl->file_content);
    }
    else
    {
        load_execute_dynamic_page(pl->file_path, pl->arg_name, pl->arg_value, pl->file_content);
    }

    print("content=%s\n", pl->file_content);
    write_response(pl->efd, pl->cfd, pl->file_content); 
}
