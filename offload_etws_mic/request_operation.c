#include "common.h"

int write_error()
{

}

void write_response(int efd, int cfd, char *file_content)
{
        char header[512];
        memset(header, 0, 512);
        strcpy(header, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");

        char response[1024];
        memset(response, 0, 1024);
        strcpy(response, header);
        strcat(response, file_content);

        int ret=write(cfd, response, sizeof(response));
        if (ret==-1) 
        {
            if(errno==EAGAIN||errno==ECONNRESET) 
            {
                reset_oneshot(efd, cfd);
            }
            print("write: %s\n", strerror(errno));
            shut_remove_conn(cfd, efd);
        }
        else if(ret>=0) shut_remove_conn(cfd, efd);
/*
        int fd=open("./index.html", O_RDONLY); 
        ssize_t len=sendfile(cfd, fd, 0, 1024); 
        close(fd); 
*/
}

int read_request(int efd, int cfd, char *request)
{
    int ret=read(cfd, request, MAX_LINE);
    if (ret==-1) 
    {
        if(errno==EAGAIN||errno==ECONNRESET) 
        {
            reset_oneshot(efd, cfd);
            return -2;
        }
        print("read: %s\n", strerror(errno));
        return -1;
    }
    else if(ret==0) 
    {
        return -1;
    }
    if(strlen(request)==0) return -2;
	return 0;
}

int parse_request(char* request, char *root_path, char *file_path, char *arg_name, char *arg_value)
{
	if(strstr(request, "GET")!=request) 
	{
		print("wrong request %s: %s\n", request, strerror(errno));
		return -1;
	}

	if((request[4]=='/')&&(request[5]==' '))
    {
		strcat(file_path, "/index.html");
    }
	else
	{
		strtok(&request[4], " ");
		strcat(file_path, &request[4]);

        char *p=strstr(file_path, "?");
        char *q=strstr(file_path, "=");

        if(p!=NULL&&q!=NULL)
        {
            char *pp=p+1; 
            char *pq=q+1; 
            *p='\0'; *q='\0';

            strcpy(arg_name, pp); 
            strcpy(arg_value, pq);
        }
        print("file_path=%s, arg_name=%s, arg_value=%s\n", file_path, arg_name, arg_value);
    }

    char path[MAX_LINE];
    memset(path, 0, MAX_LINE);
    strcpy(path, root_path);
    strcat(path, file_path);
    strcpy(file_path, path);

    return 0;
}

