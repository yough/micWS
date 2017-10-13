#include "common.h"

int main(void)
{
	int lfd, cfd, efd; 
    int port, thread_pool_size; int ret;
    char ip_addr[IP_ADDR_LEN];
    char root_path[MAX_LINE];

    memset(root_path, 0, MAX_LINE);
    memset(ip_addr, 0, IP_ADDR_LEN);

    signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	read_config(&port, root_path, ip_addr, &thread_pool_size);

    lfd=init_socket(ip_addr, port);
    efd=init_epoll(lfd);
    workers *logger=new_workers(write_log, 1);
    workers *handler=new_workers(runtime, 1);
    workers *handler_cpu=new_workers(handler_on_cpu, 24);

    printf("Listening on: %s:%d:/%s [%d]\n", ip_addr, port, root_path, thread_pool_size);

    struct epoll_event events[MAX_EVENT_NUM];
	while(true)
	{
        int n=epoll_wait(efd, events, MAX_EVENT_NUM, -1);
        print("There is/are %d events arriving.\n", n);
        char *str="this is a logger test.\n";
    //    logger->put_job(logger, (void*)str);
        if(n==-1)
        {
            print("epoll failure: %s\n", strerror(errno));
            continue;
        }

        for(int i=0; i<n; i++)
        {
            do_event(&events[i], lfd, efd, root_path, handler, handler_cpu);
        }
	}
    close(lfd);
    delete_workers(logger);
    delete_workers(handler);
	return 0;
}
