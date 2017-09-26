#include"common.h"

int load_execute_dynamic_page(char *file_path, char * arg_name, char * arg_value, char *file_content)
{
    void * libm_handle=NULL;
    float (*cosf_method)(float);
    char *errorInfo;
    float result=0;

    libm_handle=dlopen(file_path, RTLD_LAZY);
    if(!libm_handle)
    {
        print("open error: %s\n", dlerror());
        return 0;
    }

    cosf_method=dlsym(libm_handle, "cosf");

    errorInfo=dlerror();
    if(errorInfo!=NULL)
    {
        print("Dlsym Error: %s.\n", errorInfo);
        return 0;
    }

    long long t1=get_time();
    float args=atoi(arg_value); 
    result=(*cosf_method)(args);
    long long t2=get_time();

    print("time=%lld\n", t2-t1);
    dlclose(libm_handle);

    sprintf(file_content, "cpu: %f", result);
    return 0;
}

int load_static_page(char *file_path, char * file_content)
{
        memset(file_content, 0, MAX_LINE);
        int fd=open(file_path, O_RDONLY); 
        read(fd, file_content, MAX_LINE);
        close(fd); 
    
        return 0;
}
