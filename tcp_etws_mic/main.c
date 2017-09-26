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
    workers *handler=new_workers(handler_request, 24);
    workers *sender=new_workers(connectMIC, 1);
    workers *writer=new_workers(write_response, 1);
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)server_routine, (void*)writer);

    printf("Listening on: %s:%d:/%s [%d]\n", ip_addr, port, root_path, thread_pool_size);

    struct epoll_event events[MAX_EVENT_NUM];
	while(true)
	{
        int n=epoll_wait(efd, events, MAX_EVENT_NUM, -1);
        print("There is/are %d events arriving.\n", n);
        if(n==-1)
        {
            print("epoll failure: %s\n", strerror(errno));
            continue;
        }

        for(int i=0; i<n; i++)
        {
            do_event(&events[i], lfd, efd, root_path, handler, sender, writer);
        }
	}
    close(lfd);
    delete_workers(handler);
    delete_workers(sender);
    delete_workers(writer);
	return 0;
}
