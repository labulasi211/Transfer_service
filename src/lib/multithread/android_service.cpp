/**\android_serivce.cpp
 * 简要介绍:实现与android之间的tcp通信,实现一些文件传输功能
 * 作者:曾晨
 * 完成度:未完成
 * */

#include<stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>

#include <multithread/service_thread.h>
#include <message_queue/music_msg.h>
#include <socket/socket.h>

// 定义android端的几个状态
enum ANDROID_STATE
{
    ANDROID_OFFLINE = 0,
    ANDROID_ONLINE,
    ANDROID_RECVING,
};

// 定义超时计数值
#define TIMEOUT_COUNT (60)
#define TIME_NOW (time(NULL))
// 定义一个专门的判断宏
#define SUCCESSFUL (0)
#define ERROR_APPEAR (-1)

// 定义应答协议中的状态字符串长度
#define RESPONSE_LEN (2)
// 定义应答协议中的状态字符串
const char const RESPONSE_TYPE_OK_CODE[RESPONSE_LEN] = {0X04, 0X01};
const char const RESPONSE_TYPE_TRANSfER_CODE[RESPONSE_LEN] = {0X04, 0X02};

// 接收消息函数
int recv_msg(int msg_id, __music_msg *music_msg);
// 判断消息列表中是否有消息
int msg_empty(const __music_msg music_msg);
// 判断当前是否有客户端连接上
int connect_verify(const int socket);
// 服务器端套接字初始化
int service_init(__socket_arg net_arg);
// 服务器端连接,当前只支持对一个客服端连接
int service_connect(int location_socket, int *android_socket);
// 服务器端断开连接
int service_disconnect(__socket_arg net_arg);
// 接收安卓端状态数据
int recv_response(char *recv_buffer);
// 传输音乐文件
int transfer_music(__music_msg music_msg, int socket);
// 删除消息内容
int msg_delete(__music_msg *music_msg);
// 对接收到的消息进行响应
int response_android(int *state, char *recv_buffer);
// 发送响应
int send_response(const char *send_buffer, int len);

/** 服务器连接函数
 * @brief 用于对信道的监听,确保android端的连接,并且对当前的连接状态进行一个判断
 * @param[in] net_arg_p 网络变量指针,这里只用与对套接字额文件描述符进行表示
 * @param[out] int 表示当前网络状态,连接与未连接
 * */
int service_connect(int location_socket, int *android_socket);
{
    // 判断当前套接字
}

/** 本地套接字初始化
 * @brief 对本地服务器端套接字的初始化，并将初始化之后的套接字赋值给主函数中的套接字变量
 * @param[in] net_arg_n 网络变量指针,这里只用与对套接字额文件描述符进行表示
 * @param[out] int 表示判断结果
 * */
int service_init(__socket_arg net_arg)
{
    // 定义本地套接字
    int location_socket=0;
    // 定义网络变量
    __socket_arg location_net_arg;
    // 定义套接字地址变量
    sockaddr_in location_addr;

    // 初始化网络变量
    strcpy(location_net_arg.url,"lacalhost");
    strcpy(location_net_arg.ip,"");
    location_net_arg.remote_port=0;
    location_net_arg.using_port=CONNECT_PORT;

    // 获取本机地址
    if(ERROR_APPEAR==addr_init(&location_addr,&location_net_arg))
    {
        // 获取本机地址失败
        printf("GET LOCATION ADDR ERROR.\r\n");
        return ERROR_APPEAR;
    }

    // 创建套接字
    location_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(location_socket==INVALID_SOCKET)
    {
        // 创建本地套接字失败
        printf("LOCATION SOCK ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 绑定套接字
    if(SOCKET_ERROR==bind(location_socket,(sockaddr *)&location_addr,sizeof(location_addr)))
    {
        // 绑定失败
        printf("BIND ERROR.\r\n");
        return ERROR_APPEAR;
    }

    // 表示当前初始化套接字没有问题
    *net_arg.socket_point=location_socket;
    return SUCCESSFUL;
}

/** 判断消息内容是否为空
 * @brief 用于判断消息内容变量当中是否有消息内容,主要用于对消息的一个判断
 * @param[in] music_msg 指向需要保存消息内容的变量,用于消息内容的保存
 * @param[out] int 表示判断结果
 * */
int msg_empty(const __music_msg music_msg)
{
    // 只需要对msg_type进行一个判断就可以
    return music_msg.msg_type == MSG_TYPE_INVAILD ? SUCCESSFUL : ERROR_APPEAR;
}

/** 接收消息函数
 * @brief 主要就是从消息队列中读取消息并保存在相应的变量当中,当相应的变量中有数据时不读取
 * @param[in] msg_id 消息队列文件描述符,用于消息队列的读取
 * @param[in] music_msg 指向需要保存消息内容的变量,用于消息内容的保存
 * @param[out] int 函数状态返回值,当为ERROR_APPEAR时,表示接收中出现了错误 
 * */
int recv_msg(int msg_id, __music_msg *music_msg)
{
    // 对消息的接收
    if (music_msg->msg_type == MSG_TYPE_INVAILD)
    {
        // 表示消息内容变量当中没有消息
        return msgrcv(msg_id, (void *)&music_msg, MSG_SIZE, MSG_TYPE, IPC_NOWAIT) >= 0 ? SUCCESSFUL : ERROR_APPEAR;
    }
    // 表示没有正确的接收到消息
    return ERROR_APPEAR;
}

/** android_service
 * @brief 主要实现面向andoid端的数据的传送,实现一些基本的tcp数据传送的功能
 * @param[in] arg 是一个数据结构变量,主要是通过从主函数中获得相应的参数
 * */
void *android_service(void *arg)
{
    // 对参数中的数据进行格式化
    __service_thread_arg thread_arg = *(__service_thread_arg *)arg;
    // 定义消息结构体
    __music_msg music_msg;
    // 定义消息ID
    int msg_id = thread_arg.database.msg_id;
    // 定义网络变量
    __socket_arg socket_arg = thread_arg.database.socket_arg;
    // android端状态,初始化为0,表示下线,而1表示上线.
    int android_state = ANDROID_OFFLINE;
    // 定义接收缓存区
    char recv_buffer[RECV_BUF_SIZE] = {0};
    // 定义时间记录值
    time_t time_start;

    // 初始化消息结构体
    music_msg.msg_type = MSG_TYPE_INVAILD;
    music_msg.music_num = 0;
    music_msg.music_info_list = NULL;

    // 本地服务器套接字初始化
    if (ERROR_APPEAR == service_init(socket_arg))
    {
        // 初始化本地套接字失败
        // 之后的操作都没办法实现
        exit(1);
    }

    while (1)
    {
        // 对端口进行监听,查看是否有客服端连接
        if (SUCCESSFUL == service_connect(socket_arg))
        {
            // 连接成功之后开始计数
            time_start = TIME_NOW;
        }
        else
        {
            // 没有安卓端的连接
            // 跳过之后的操作
            continue;
        }

        // 查看是否超时
        while (TIMEOUT_COUNT > difftime(TIME_NOW, time_start))
        {
            // 接收消息队列中的消息
            recv_msg(msg_id, &music_msg);
            // 对当前消息状态和连接状态进行判断
            if (android_state == ANDROID_ONLINE && ERROR_APPEAR == msg_empty(music_msg))
            {

                // 表示现在有消息过来,需要向android端传输数据
                if (ERROR_APPEAR == send_response(RESPONSE_TYPE_TRANSfER_CODE, RESPONSE_LEN))
                {
                    // 发送失败
                    printf("SEND RESPONSE FAIL.\r\n");
                }
            }
            // 接收响应
            if (SUCCESSFUL == recv_response(recv_buffer))
            {
                // 时间计数值清零
                time_start = TIME_NOW;
            }
            // 对接收到响应进行响应
            if (ERROR_APPEAR == response_android(&android_state, recv_buffer))
            {
                // 向android端的响应做出响应失败
                printf("RESPONSE ANDROID FAIL.\r\n");
            }
        }
        // 超时,断开连接
        service_disconnect(socket_arg);
    }
}