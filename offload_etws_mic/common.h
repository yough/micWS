#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <sys/epoll.h>
#include "workers.h"

//#define _DEBUG
#ifdef _DEBUG
#define print(format,...) printf("[%s, %03d, %s]: "format"" ,  __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define print(format,...)
#endif

#define true  1
#define false 0
#define MAX_LINE 1024
#define QUEUE_LENGTH 1024
#define MAX_EVENT_NUM 10240
#define IP_ADDR_LEN 30

extern char root_path[MAX_LINE];

//assistant function
extern int write_log(void *arg, void *result);
extern int read_config(int * port, char * path, char * ip_addr, int * thread_pool_size);
extern long long get_time();
extern int return_rand_num(int max);

//request operation
extern int read_request(int efd, int cfd, char *request);
extern int parse_request(char* request, char *root_path, char *file_path, char *arg_name, char *arg_value);
extern void write_response(int efd, int cfd, char *file_content);
extern int write_error();

typedef struct _runtime_args
{
    int efd;
    int cfd;
    char root_path[MAX_LINE];
    workers *fun;
} runtime_args;

typedef struct _paraList
{
    int efd;
    int cfd;
    char file_path[100];
    char file_content[100];
    char arg_name[5];
    char arg_value[5];
} paraList;

//dynamic and static page operation
extern int load_execute_dynamic_page(char *page, char* arg_name,char * arg_value, char *file_content);
extern int load_static_page(char *file_path, char * file_content);
extern __attribute__((target(mic))) int execute_dynamic_page_on_mic(char *file_path, char * arg_name, char * arg_value, char *file_content);
extern __attribute__((target(mic))) int execute_static_page_on_mic(char *file_path, char * file_content);
//extern int handler_on_mic(int efd, int cfd, char *file_path, char *arg_name, char *arg_value);
extern int handler_on_cpu(void *ppl, void *result);
extern int handler_on_mic(paraList pl);

//socket operation
int init_socket(char *address, int port);

//epoll operation
extern int set_non_blocking(int fd);
extern void add_fd(int epfd, int fd, int enable_et);
extern void del_fd(int epfd, int fd, int oneshot);
extern void mod_fd(int epfd, int fd, int oneshot);
extern void reset_oneshot(int efd, int fd);
extern int shut_remove_conn(int connfd, int efd);
extern int init_epoll(int sockfd);
extern int accept_add_fd(int sockfd, int efd, struct epoll_event *evp); 

//event operation
extern void do_event(struct epoll_event *evp, int lfd, int efd, char* root_path, workers *handler, workers *handler_cpu);
extern int runtime(void *args, void *result);
extern void time_scheduler(void *p);

