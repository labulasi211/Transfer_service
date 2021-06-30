/**\callback_func.cpp
 * 简要介绍:主要是实现android和web端线程的回调任务
 * 详细说明:其中web端线程的回调函数主要就是实现对http报文的处理,并且将提取到的信息传输到消息列表中
 *         android端的回调函数主要就是实现对android端中的数据编写成日志
 * 作者:曾晨
 * 完成度:未完成
 * */

#include<stdio.h>
#include<string.h>

#include<callback/callback_func.h>
#include<message_queue/music_msg.h>

/**android回调函数
 * @brief 测试
 * @param[in] arg 调用回调函数需要的参数
 * @param[in] str 调用该函数的日志信息
 * */
int android_callback_func(void* str,__data_arg arg)
{
    printf("singer:%s\r\n\r\n",(char*)str);
    return OK;
}

/**web回调函数
 * @brief 现在只是进行打印
 * @param[in] arg 调用回调函数需要的参数
 * @param[in] str 调用该函数的日志信息
 * */
int web_callback_func(void*str, __data_arg arg)
{
    printf("%s\r\n\r\n",(char*)str);
    return OK;
}