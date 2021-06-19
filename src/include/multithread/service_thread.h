#ifndef _SERVICE_THREAD_H
#define _SERVICE_THREAD_H

#include <pthread.h>

// 定义创建线程时的传递参数
typedef struct __SERVICE_THREAD_ARG
{
    // 消息队列ID
    int msg_id;
    // 回调函数指针,其中第一个函数是接受到的数据,第二个是相应线程传递的参数
    void (*callback)(void *str, int arg);
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
