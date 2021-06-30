/**\web_client.cpp
 * 简要介绍:从web端获取http报文
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <multithread/service_thread.h>
#include <message_queue/music_msg.h>



/**面向web的客户端函数
 * @brief 就是通过与web端进行tcp连接,并接受web端的http报文,对报文进行相应的响应,除此之外,还需要将http中的数据断作为参数
 *        传递给回调函数
 * @param[in] arg中包含了回调函数首地址和一些需要的参数
 * */
void *web_client(void *arg)
{
    // 初始化函数
    http_response_status_string_init();
    // 格式化输入参数格式
    __service_thread_arg thread_arg = *(__service_thread_arg *)arg;
    // 定义请求字符串缓存变量,用于保存请求字符串
    char *request_data = NULL;
    // 定义http报文响应状态码
    int http_response_mun = 400;
    // 接收参数结果变量
    int ret = -1;

    // 通过网络参数进行连接
    if (-1 == client_connect(&thread_arg.database.socket_arg))
    {
        // 连接失败
        printf("CONNECT ERROR!\r\n");
        exit(1);
    }

    // 连接成功
    puts("CONNECT SUCCESSFUL!\r\n");

    // 轮循对web端接口进行接收数据,对数据内容进行解析,解析得到相应需要的内容之后提交给回调函数
    while (1)
    {
        // 对web端接口进行http报文接收
        ret == http_recv(*thread_arg.database.socket_arg.socket_point, &request_data);
        // 对结果进行判断
        if (ret == 0)
        {
            // 表示http请求报文内容接收成功
            // 调用回调函数来对内容进行处理
            http_response_mun = thread_arg.callback(request_data, thread_arg.database);
            // 根据状态码进行http报文响应
        }
        else if (ret == -2)
        {
            // 说明发送过来的数据不符合要求或则没有接收到数据
            sleep(1);
        }
    }

    // 程序退出
    exit(0);
}