#include "workers.h"

void *thread_routine_execution(void *arg);

void init(workers* this, int max_thread_num)
{
    pthread_mutex_init(&(this->queue_lock), NULL);
    pthread_cond_init(&(this->queue_ready), NULL);

    this->job_queue_head=NULL;
    this->job_result_queue_head=NULL;
    this->max_thread_num=max_thread_num;
    this->cur_job_queue_size=0;
    this->cur_job_result_queue_size=0;
    this->is_shutdown=0; //1关闭，0不关

    this->thread_id=(pthread_t*)malloc(max_thread_num*sizeof(pthread_t));
    for(int i=0; i<max_thread_num; i++)
    {
        int ret=pthread_create(&(this->thread_id[i]), NULL, thread_routine_execution, (void*)this);
        if(ret==-1) printf("pthread: %s\n", strerror(errno));
    }
}

void print_job_queue(workers *this)
{
    printf("print result queue[%d]:\n", this->cur_job_queue_size);
    job *p=this->job_queue_head;
    int i=0;
    while(p!=NULL)
    {
        i++;
        printf("%d. %s\n", i, (char*) p->arg);
        p=p->next;
    }
}

void print_job_result_queue(workers *this)
{
    job *p=this->job_result_queue_head;
    int i=0;
    printf("print result queue[%d]:\n", this->cur_job_result_queue_size);
    while(p!=NULL)
    {
        i++;
        printf("%d. %s\n", i, (char*) p->arg);
        p=p->next;
    }
}

int add_job(workers *this, void *arg)
{
    /*if(arg!=NULL)
         printf("%s\n", (char*)arg);
    else printf("NULL\n"); */

    job* new_job=(job*)malloc(sizeof(job));
    new_job->arg=(void*) malloc(MAX_LINE);
    memset(new_job->arg, 0, MAX_LINE);
    memcpy(new_job->arg, arg, MAX_LINE);
    new_job->next=NULL;

    pthread_mutex_lock(&(this->queue_lock));
    job *p=this->job_queue_head;
    if(p!=NULL)
    {
        while(p->next!=NULL)
        {
            p=p->next;
        }
        p->next=new_job;
    }
    else 
    {
        this->job_queue_head=new_job;
    }

    assert(this->job_queue_head!=NULL);
    this->cur_job_queue_size++;
    pthread_cond_signal(&(this->queue_ready));
    pthread_mutex_unlock(&(this->queue_lock));
    return 0;
}

int destory(workers *this)
{
    if(this->is_shutdown) return -1;
    this->is_shutdown=1;

    pthread_cond_broadcast(&(this->queue_ready));

    for(int i=0; i<this->max_thread_num; i++)
    {
        pthread_join(this->thread_id[i], NULL);
    }
    free(this->thread_id);

    job * head=NULL;
    while(this->job_queue_head!=NULL)
    {
        head=this->job_queue_head;
        this->job_queue_head=this->job_queue_head->next;
        free(head->arg);
        free(head);
    }

    while(this->job_result_queue_head!=NULL)
    {
        head=this->job_result_queue_head;
        this->job_result_queue_head=this->job_result_queue_head->next;
        free(head->arg);   free(head);
    }

    pthread_mutex_destroy(&(this->queue_lock));
    pthread_cond_destroy(&(this->queue_ready));

    return 0;
}

void *thread_routine_execution(void *arg)
{
    workers *this=(workers*)arg;
    //printf("starting thread 0x%x\n", (unsigned int)pthread_self());
    while(1)
    {
        pthread_mutex_lock(&(this->queue_lock));
        while(this->cur_job_queue_size==0&&this->is_shutdown==false)
        {
            //printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
            pthread_cond_wait(&(this->queue_ready), &(this->queue_lock));
        }

        if(this->is_shutdown==true)
        {
            pthread_mutex_unlock(&(this->queue_lock));
            //printf("thread 0x%x will exit\n", (unsigned int)pthread_self());
            pthread_exit(NULL);
        }
        //printf("thread 0x%x is starting to work.\n", (unsigned int)pthread_self());
        assert(this->cur_job_queue_size!=0);
        assert(this->job_queue_head!=NULL);

        this->cur_job_queue_size--;
        job *p=this->job_queue_head;
        this->job_queue_head=p->next;
        pthread_mutex_unlock(&(this->queue_lock));

        void *p_result=(void*)malloc(MAX_LINE); 
        memset(p_result, 0, MAX_LINE);
        this->process(p->arg, p_result);
        free(p); p=NULL;

        if(this->is_all_zero(this, p_result)==0) 
        {
            free(p_result);
            p_result=NULL;
            continue;
        }

        //add result to result queueu
        pthread_mutex_lock(&(this->queue_lock));
        job* job_result=(job*)malloc(sizeof(job));
        job_result->arg=(void*)malloc(MAX_LINE);
        memset(job_result->arg, 0, MAX_LINE);
        memcpy(job_result->arg, p_result, MAX_LINE);
        job_result->next=NULL;
        free(p_result); p_result=NULL;

        job *p_job_result=this->job_result_queue_head;
        if(p_job_result!=NULL)
        {
            while(p_job_result->next!=NULL)
            {
                p_job_result=p_job_result->next;
            }
            p_job_result->next=job_result;
        }
        else 
        {
            this->job_result_queue_head=job_result;
        }

        assert(this->job_result_queue_head!=NULL);
        this->cur_job_result_queue_size++;
        pthread_mutex_unlock(&(this->queue_lock));
    }
    pthread_exit(NULL); 
}

int get_result(workers *this, void *out_arg)
{
    pthread_mutex_lock(&(this->queue_lock));
    job* p=this->job_result_queue_head;
    if(p!=NULL)
    {
        this->job_result_queue_head=p->next;
        this->cur_job_result_queue_size--;
        memcpy(out_arg, p->arg, MAX_LINE);
        free(p->arg); 
        free(p);
    }
    else 
    {
        memset(out_arg, 0,  MAX_LINE);
    }
    pthread_mutex_unlock(&(this->queue_lock));
    return 0; 
}

int is_all_zero(workers *this, void * sample)
{
    void *pattern=(void*)malloc(MAX_LINE);
    memset(pattern, 0, MAX_LINE);
    return memcmp(sample, pattern, MAX_LINE);
}

workers *new_workers(int (*process)(void *arg, void* result), int max_thread_num)
{
    workers *this=(workers*)malloc(sizeof(workers));

    this->process=process;
    this->put_job=add_job;
    this->destory=destory;
    this->get_result=get_result;
    this->print_job_queue=print_job_queue;
    this->print_job_result_queue=print_job_result_queue;
    this->is_all_zero=is_all_zero;
    this->init=init;

    this->init(this, max_thread_num);    

    return this;
}

int delete_workers(workers *this)
{
    destory(this);
    return 0;
}

