#include "common.h"

int read_config(int * port, char * path, char * ip_addr, int * thread_pool_size)
{
	char buf[50]; FILE *fp=fopen("./config.ini", "r");
	if(fp==NULL)
	{
		print("%s\n", strerror(errno));
		return -1;
	}
	while(fgets(buf, 50, fp)!=NULL)
	{
		if(buf[strlen(buf)-1]!='\n')
		{
			print("%s\n", "error in config.ini");
			return -1;
		}
		else buf[strlen(buf)-1]='\0';
		if(strstr(buf, "port")==buf)
		{
			char * p=strchr(buf, ':');
			if(p==NULL)
			{
				print("config.ini expect ':'\n");
				return -1;
			}

			*port=atoi(p+2);
			if(*port<=0)
			{
				print("error port\n");
				return -1;
			}
		}
		else if(strstr(buf, "root-path")==buf)
		{
			char * p=strchr(buf, ':');
			if(p==NULL)
			{
				print("config.ini expect ':'\n");
				return -1;
			}
			p=p+2;

			strcpy(path, p);
		}
		else if(strstr(buf, "ip-addr")==buf)
		{
			char * p=strchr(buf, ':');
			if(p==NULL)
			{
				print("config.ini expect ':'\n");
				return -1;
			}
			p=p+2;

			strcpy(ip_addr, p);
		}
		else if(strstr(buf, "thread-pool-size")==buf)
		{
			char * p=strchr(buf, ':');
			if(p==NULL)
			{
				print("config.ini expect ':'\n");
				return -1;
			}
			*thread_pool_size=atoi(p+2);
			if(*thread_pool_size<=0)
			{
				print("error port\n");
				return -1;
		    }
        }
		else
		{
			print("error in config\n");
			return -1;
		}
	}
    fclose(fp); return 0;
}

int write_log(void *arg, void *result)
{
    char *str=(char*)arg;
	time_t t;
	if(time(&t)==-1)
	{
		perror("fail to time");
		return -1;
	}

	struct tm *cal;
	cal=localtime(&t);
	char buf[MAX_LINE];
	int m=sprintf(buf, "[%d-%02d-%02d %02d:%02d:%02d] %s\n", cal->tm_year+1900, cal->tm_mon+1, cal->tm_mday, cal->tm_hour, cal->tm_min, cal->tm_sec, str);
	buf[m]='\0';

	int fd=open("./ws.log", O_WRONLY|O_APPEND|O_CREAT);
	if(fd==-1)
	{
        print("%s\n", strerror(errno));
		return -1;
	}

	int n=write(fd, buf, m);
	if(n==-1)
	{
        print("%s\n", strerror(errno));
		return -1;
	}
	close(fd);
    memset(result, 0, MAX_LINE);
	return 0;
}

long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long l=tv.tv_sec*1000*1000+tv.tv_usec;
    return l;
}
