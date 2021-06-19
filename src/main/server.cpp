/**\Transfer_service
 * 简要说明:承担"中间件"作用
 * 详细说明:承接 web 后端音乐管理系统与 android 前端设备,将一部分通信以及文件传输交由 c 语言写成的中转服务来完成
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<signal.h>

// 自己编写的头文件
#include <callback/callback_func.h>
#include <multithread/service_thread.h>
#include <message_queue/music_msg.h>

// 定义了TCP客户端和服务端使用的端口号
#define SERVICE_PORT 80
#define CLIENT_PORT 81

// 定义两个线程之间的通信方式为消息队列
// 原因在于消息队列可以很好的对复杂的数据类型进行传输
// 定义消息队列关键字
#define MSG_KEY (1233)

// 定义面向web的客户端线程和面向Android的服务端线程
pthread_t android_service_id = 0;
pthread_t web_client_id = 0;

// 定义面向web和android的线程需要传递的参数
__service_thread_arg web_arg;
__service_thread_arg android_arg;

//创建两个线程,分别是面向web端的和android端的
int creat_service_thread(void);
// 结束两个创建的线程
int cancel_service_thread(void);
// 退出处理函数
void exit_handle(int signo);


/**退出处理函数
 * @brief 主要是对异常退出时的一些收尾工作
 * @param[in] 信号输入
 * */
void exit_handle(int signo)
{
   printf("exit!\r\nsigno:%d\r\n",signo);
   exit(0); 
}

/**初始化并创建面向web和android的两个线程函数
 * @brief 创建面向web和android的两个线程,并对两者需要的参数进行相应的初始化
 * @param[out] 创建线程的状态
 * */
int creat_service_thread(void)
{
    // 返回结果
    int ret = -1;

    // 初始化需要传递的参数
    web_arg.msg_id = init_msg(key_t(MSG_KEY));
    android_arg.msg_id = web_arg.msg_id;
    // 回调函数
    web_arg.callback = web_callback_func;
    android_arg.callback = android_callback_func;

    // 判断消息队列是否创建成功
    if(web_arg.msg_id==-1)
    {
        printf("MSG ERROR!\r\n");
        return ret;
    }

    // 创建线程
    ret = pthread_create(&web_client_id, NULL, web_client, (void *)&web_arg);
    if (0 == ret)
    {
        ret = pthread_create(&android_service_id, NULL, android_service, (void *)&android_arg);
    }
    return ret;
}

/**反初始化线程函数
 * @brief 销毁线程
 * @param[out] 返回销毁状态，成功或失败
 * */
int cancel_service_thread(void)
{
    if (0 == deinit_thread(web_client_id))
    {
        if (0 == deinit_thread(android_service_id))
        {
            return 0;
        }
    }
    return -1;
}

int main(void)
{
    // 对一些异常的处理
    signal(SIGINT,exit_handle);
    // 初始化线程
    if (-1 == creat_service_thread())
    {
        printf("Creat thread error!\r\n");
        exit(1);
    }

    sleep(30);

    // 反初始化线程
    if (-1 == cancel_service_thread())
    {
        printf("Destroy thread error!\r\n");
    }
    deinit_msg((key_t)MSG_KEY);
}