#ifndef _SERVICE_THREAD_H
#define _SERVICE_THREAD_H

#include <pthread.h>
#include <sys/types.h>
#include <socket/socket.h>
#include <signal.h>

// 声明创建线程需要传递的变量参数
typedef struct __DATA_ARG
{
    // 消息队列ID
    int msg_id;
    // 需要使用到的套接字地址
    int *socket_point;
    // 需要使用到的端括号
    int using_port;
    // 定义对方的url
    char url[MAX_URL_LEN];
    // 定义对方的ip
    char ip[IP_ADDR_LEN];
    // 定义对方的port
    int remote_port;
} __data_arg;

// 定义创建线程时的传递参数
typedef struct __SERVICE_THREAD_ARG
{
    // 需要传递的数据参数
    __data_arg database;
    // 回调函数指针,其中第一个函数是接受到的数据,第二个是相应线程传递的参数
    int (*callback)(void *str, __data_arg arg);
} __service_thread_arg;

// 多线程函数声明
// 初始化线程
pthread_t init_thread(__service_thread_arg *arg);
// 反初始化线程
int deinit_thread(pthread_t tid);

// 面向web端的线程
void *web_client(void *arg);
// 面向android端的线程
void *android_service(void *arg);

#endif
