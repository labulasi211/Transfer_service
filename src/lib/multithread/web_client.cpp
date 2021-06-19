/**\web_client.cpp
 * 简要介绍:从web端获取http报文
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include<unistd.h>

#include <multithread/service_thread.h>
#include<message_queue/music_msg.h> 

/**面向web的客户端函数
 * @brief 就是通过与web端进行tcp连接,并接受web端的http报文,对报文进行相应的响应,除此之外,还需要将http中的数据断作为参数
 *        传递给回调函数
 * @param[in] arg中包含了回调函数首地址和一些需要的参数
 * */
void *web_client(void *arg)
{
    // 格式化输入参数格式
    __service_thread_arg thread_arg=*(__service_thread_arg*)arg;

    // 向web端进行连接
    int socket;

    while(1)
    {

    }
}