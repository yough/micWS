#include<stdio.h>
#include<unistd.h>
#include<sys/time.h>

long long get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long l=tv.tv_sec*1000*1000+tv.tv_usec;
    return l/1000;
}

float cosf(float f)
{
    //print("f in cpu: %f\n", f);
    long long l1=get_time();
    float sum=0;
    for(int i=0; i<2000000; i++)
    {
        sum+=f;
    }
    //printf("time in cosf=%lld\n", get_time()-l1);
    //print("sum in cpu: %f\n", sum);
    return sum;
}
