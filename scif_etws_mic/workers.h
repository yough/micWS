#define _GNU_SOURCE
#define MAX_LINE 1024
#define true 1
#define false 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

typedef struct _job
{
    void * arg;
    struct _job *next;
} job;

typedef struct _workers
{
    //data
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    job *job_queue_head;
    job *job_result_queue_head;
    int is_shutdown;
    pthread_t *thread_id;

    int max_thread_num;
    int cur_job_queue_size;
    int cur_job_result_queue_size;

    //methods
    int (*process) (void *arg, void* result);
    void (*init)(struct _workers *this, int max_thread_num);
    int (*is_all_zero)(struct _workers *this, void * sample);
    int  (*put_job)(struct _workers *this, void *arg);
    int  (*destory)(struct _workers *this);
    int (*get_result)(struct _workers *this, void *out_arg);
    void (*print_job_queue)(struct _workers *this);
    void (*print_job_result_queue)(struct _workers *this);
} workers;


workers *new_workers(int (*process)(void *arg, void *result), int max_thread_num);
extern int delete_workers(workers *this);
