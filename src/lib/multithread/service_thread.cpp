/**\service_thread.cpp
 * 简要介绍:主要承担多线程的一些操作
 * 详细介绍:对多线程的一些操作进行封装,使整个程序模块分离
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <multithread/service_thread.h>

/**初始化线程函数
 * @brief 创建线程
 * @param[out] 返回线程ID
 * */
pthread_t init_thread(__service_thread_arg *arg)
{
    return 0; 
}

/**反初始化线程函数
 * @brief 销毁线程
 * @param[out] 返回销毁状态，成功或失败
 * */
int deinit_thread(pthread_t tid)
{
    int ret=-1;

    // 发送终止命令给tid线程
    ret=pthread_cancel(tid);
    // 等待线程退出
    if(0==ret)
    {
        ret=pthread_join(tid,NULL);
    }
    return ret;
}