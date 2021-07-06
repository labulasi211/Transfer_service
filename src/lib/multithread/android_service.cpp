/**\android_serivce.cpp
 * 简要介绍:实现与android之间的tcp通信,实现一些文件传输功能
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
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
    ANDROID_UNDEFINED,
    ANDROID_RECVING,
};
// 定义改变状态的几个方式
enum CHANGE_ANDROID_STATE
{
    TO_OFFLINE = 0,
    TO_ONLINE,
    TO_RECVING,
};

// 定义改变方式
#define UP |=
#define DOWN &=

// 定义超时计数值
#define TIMEOUT_COUNT (60)
#define TIME_NOW (time(NULL))
// 定义一个专门的判断宏
#define SUCCESSFUL (0)
#define ERROR_APPEAR (-1)
// 定义select超时时间
#define ACCEPT_TIMEOUT (1)
#define RECV_UTIMEOUT (100)

// 定义应答协议中的状态字符串长度
#define RESPONSE_LEN (2)
// 定义应答协议中的状态字符串
const char const RESPONSE_TYPE_OK_CODE[RESPONSE_LEN] = {0X04, 0X01};
const char const RESPONSE_TYPE_TRANSfER_CODE[RESPONSE_LEN] = {0X04, 0X02};

// 接收消息函数
int recv_msg(int msg_id, __music_msg *music_msg);
// 判断消息列表中是否有消息
int msg_empty(const __music_msg music_msg);
// 判断当前是否有客户端连接上*
int connect_verify(const int socket);
// 服务器端套接字初始化
int service_init(__socket_arg net_arg);
// 服务器端连接,当前只支持对一个客服端连接
int service_connect(const int location_socket, int *android_socket);
// 服务器端断开连接
int service_disconnect(int disconnect_socket);
// 接收安卓端状态数据
int recv_response(int android_socket, char *recv_buffer);
// 传输音乐文件*
int transfer_music(__music_msg music_msg, int recv_socket);
// 删除消息内容
int msg_delete(__music_msg *music_msg);
// 对接收到的消息进行响应
int response_android(int *state, char *recv_buffer);
// 发送响应
int send_response(const char *send_buffer, int len);
// 设置socket为非阻塞状态
int set_socket_nonblock(int set_socket);

/** 文件传输函数
 * @brief 通过对消息的判断，实现一定量的文件传输，只传送一定量
 * @param[in] music_arg 指向消息结构体的指针，主要用于储存传递过来的消息
 * @param[in] recv_socket 需要传输的套接字文件描述符
 * @param[out] int 返回传输状态
 * */
int transfer_music(__music_msg music_msg, int recv_socket)
{
    // 定义当前发送的第几个文件
    static int position = 0;
    // 定义文件总数目
    static int music_num = 0;
    // 定义当前需要发送列表的指针
    static struct __MUSIC_INFO *music_info_list = NULL;
    // 定义当前传输的文件的位置
    static long int file_len = 0;
    // 定义文件描述符
    static int fp = NULL;

    // 定义发送缓存区
    char send_buffer[SEND_BUF_SIZE] = {0};

    // 对music_msg进行解析，得到需要传输的相应文件的位置
    if (music_msg.msg_type == MSG_TYPE)
    {
        // 当有消息时，表示需要开始发送新的消息
        // 获取需要传输消息中需要使用的变量
        position = 0;
        music_info_list = (struct __MUSIC_INFO *)malloc(sizeof(struct __MUSIC_INFO) * music_msg.music_num);
        if (music_info_list == NULL)
        {
            // 表示内存分配失败
            printf("MUSIC INFO LIST MALLOC ERROR.\r\n");
            return ERROR_APPEAR;
        }
        memcpy(music_info_list, music_msg.music_info_list, sizeof(struct __MUSIC_INFO) * music_msg.music_num);
        file_len = 0;
        fp = NULL;
        music_num = music_msg.music_num;
    }

    // 传输一定数据
    if (music_info_list != NULL)
    {
        // 对文件的读取
        fp = fp ? fp : open(music_info_list[position].file_path, O_RDONLY);
        // 读取文件
        if (0 >= read(fp, send_buffer, SEND_BUF_SIZE))
        {
            // 表示当前文件已经全部读取或者读取失败
            // 跳转到下一个文件
            fp = NULL;
            position++;
            // 判断当前是否已经传输完成
            if (position == music_num)
            {
                // 表示已经传输完成
                // 不需要在传输了
                return ERROR_APPEAR;
            }
        }
        // 将读取到的数据发送出去
        else
        {
            if (0 >send(recv_socket, send_buffer, SEND_BUF_SIZE, 0))
            {
                // 表示发送错误
                return ERROR_APPEAR;
            }
        }
        // 表示发送成功
        return SUCCESSFUL;
    }
    // 表示发生了错误
    printf("TRANSFER ERROR.\r\n");
    return ERROR_APPEAR;
}

/** 对接收到的消息进行响应
 * @brief 对接收到的消息进行响应的响应
 * @param[in] state android端的状态指针，主要用于对android命令的响应，上线和下线或者传输
 * @param[in] recv_buffer 接收缓存区的地址，用于保存接收到的数据
 * @param[out] int 返回响应状态
 * */
int response_android(int *state, char *recv_buffer)
{
    // 对接收消息的第一位进行判断
    switch (recv_buffer[0])
    {
    case 0x01:
        // 上下线
        if (recv_buffer[1] == 0x01)
        {
            // 表示上线
            *state UP TO_ONLINE;
        }
        else if (recv_buffer[1] == 0x00)
        {
            // 表示下线
            *state DOWN TO_OFFLINE;
        }
        break;
    // 心跳
    case 0x02:
        break;
    case 0x03:
        // 下载传输
        // 判断当前响应类型是否是异常并且当前正在发送信息
        if (recv_buffer[1] == 0x00)
        {
            // 表示当前需要停止发送
            // 将当前状态重新赋值为在线
            *state DOWN TO_ONLINE;
        }
        // 判断当前响应类型是否是准备好啦并且当前该设备也在线
        else if (recv_buffer[1] == 0x01)
        {
            // 表示可以向android端发送数据
            *state UP TO_RECVING;
            if (*state == ANDROID_UNDEFINED)
            {
                // 如果出现了未定义的值
                *state DOWN TO_OFFLINE;
            }
        }
        break;
    default:
        // 表示接收到了无法识别的消息
        return ERROR_APPEAR;
    }
    // 表示响应成功
    return SUCCESSFUL;
}

/** 接收响应函数
 * @brief 从android端接收响应，并保存在相应的缓存区内
 * @param[in] recv_buffer 接收缓存区地址，用于保存接收到的数据
 * @param[in] android_socket android端的地址，用于读取数据
 * @param[out] int 返回删除状态
 * */
int recv_response(int android_socket, char *recv_buffer)
{
    // 同样的通过select实现一定时间阻塞的接收
    // 判断套接字是否为空
    if (android_socket == NULL)
    {
        // 如果为空
        printf("SOCKET RECV ERROR.\r\n");
        return ERROR_APPEAR;
    }

    // 定义select中需要使用到的参数
    fd_set readfds;
    struct timeval timeout;
    int maxfd;
    int res;

    // 进行初始化
    FD_ZERO(&readfds);
    FD_SET(android_socket, &readfds);
    timeout.tv_usec = RECV_UTIMEOUT;
    timeout.tv_sec = 0;
    maxfd = android_socket;

    // select判断接收响应
    // select中是对过程作出判断还是结果作出判断
    res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
    if (res == ERROR_APPEAR)
    {
        // 表示select中发生了错误
        printf("RECV SELECT ERROR.\r\n");
        return ERROR_APPEAR;
    }
    if (res == 0)
    {
        // 表示select超时
        return ERROR_APPEAR;
    }

    // 表示接收到了消息
    // 遍历整个文件描述标识符，确定是那个是真正的接收到数据的socket
    // 主要是为了之后多个android端时更好的进行修改
    for (int i = 0; i <= maxfd; i++)
    {
        // 判断该描述符是否在readfd中
        if (!FD_ISSET(i, &readfds))
        {
            // 表示当前i对应的文件描述符不在readfds中
            continue;
        }

        // 表示当前i对应的文件描述符就是android_socket
        if (recv(i, (void *)recv_buffer, RESPONSE_LEN, 0) > 0)
        {
            // 表示当前接收成功
            return SUCCESSFUL;
        }
    }
    // 表示发生了一些出乎意料的错误
    printf("RECV RESSPONSE ERROR.\r\n");
    return ERROR_APPEAR;
}

/** 删除消息内容函数
 * @brief 将消息队列中的列表指针指向的储存区格式化，并将其他消息内容初始化
 * @param[in] music_arg 指向消息结构体的指针，主要用于储存传递过来的消息
 * @param[out] int 返回删除状态
 * */
int msg_delete(__music_msg *music_msg)
{
    // 类似初始化操作
    music_msg->msg_type = MSG_TYPE_INVAILD;
    music_msg->music_num = 0;
    free(music_msg->music_info_list);
    music_msg->music_info_list = NULL;
    return SUCCESSFUL;
}

/** 接收消息函数
 * @brief 主要就是从消息队列中读取消息并保存在相应的变量当中,当相应的变量中有数据时不读取
 * @param[in] msg_id 消息队列文件描述符,用于消息队列的读取
 * @param[in] music_msg 指向需要保存消息内容的变量,用于消息内容的保存
 * @param[out] int 函数状态返回值,当为ERROR_APPEAR时,表示接收中出现了错误 
 * */
int recv_msg(int msg_id, __music_msg *music_msg)
{
    // 判断music_arg中是否已经有消息
    if (music_msg->msg_type == MSG_TYPE)
    {
        // 表示其中已经有消息了
        return SUCCESSFUL;
    }

    // 当其中没有消息的话
    // 从消息队列中接收消息
    return msgrcv(msg_id, (void *)music_msg, MSG_SIZE, MSG_TYPE, IPC_NOWAIT) > 0 ? SUCCESSFUL : ERROR_APPEAR;
}

/** 断开连接函数
 * @brief 通过相应的socket来断开与其的连接
 * @param[in] disconnect_socket 需要断开连接的套接字
 * @param[out] int 断开状态
 * */
int service_disconnect(int disconnect_socket)
{
    // 只需要通过shutdown来断开连接就可以了
    return shutdown(disconnect_socket, SHUT_RDWR);
}

/** 服务器连接函数
 * @brief 用于对信道的监听,确保android端的连接,并且对当前的连接状态进行一个判断
 * @param[in] net_arg_p 网络变量指针,这里只用与对套接字额文件描述符进行表示
 * @param[out] int 表示当前网络状态,连接与未连接
 * */
int service_connect(const int location_socket, int *android_socket)
{
    // 判断当前android端的套接字是否有效
    if (*android_socket != INVALID_SOCKET)
    {
        // 表示当前已经连接上了android端，不需要在连接，返回
        return SUCCESSFUL;
    }

    // 定义建立连接上的套接字
    int connect_socket = 0;
    // 通过select实现非阻塞式连接
    fd_set readfds;
    struct timeval timeout;
    int maxfd;
    int res;

    // 对select中需要使用到的函数进行初始化
    FD_ZERO(&readfds);
    maxfd = location_socket;
    FD_SET(location_socket, &readfds);
    timeout.tv_sec = ACCEPT_TIMEOUT;
    timeout.tv_usec = 0;

    // 进行select判断
    res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
    if (res == ERROR_APPEAR)
    {
        // 表示select时发生了错误
        printf("SELECT ERROR.\r\n");
        return ERROR_APPEAR;
    }
    if (res == 0)
    {
        // 表示select时超时
        return ERROR_APPEAR;
    }

    // 判断当前是否本地套接字是否存在与readfds中
    if (FD_ISSET(location_socket, &readfds))
    {
        // 此时表示readfd中有文件描述符可用，也就是location_socket
        connect_socket = accept(location_socket, NULL, NULL);
    }
    else
    {
        // 出现了未知的错误
        printf("CONNECT ANDROID SOCKET ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 表示连接成功
    *android_socket = connect_socket;
    return SUCCESSFUL;
}

/** 将套接字设置为非阻塞式
 * @brief 将套接字设置非阻塞式，用于之后的非阻塞式读取信息
 * @param[in] set_socket 套接字变量，为需要设置为非阻塞式
 * @param[out] int 表示判断结果
 * */
int set_socket_nonblock(int set_socket)
{
    // 定义文件描述符状态旗标
    int flags;
    // 读取套接字的文件描述符状态旗标
    flags = fcntl(set_socket, F_GETFL, 0);
    if (flags == ERROR_APPEAR)
    {
        // 当获取出错时
        printf("F_GETFL ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 对套接字文件描述符状态旗标进行修改为非阻塞式
    if (ERROR_APPEAR == fcntl(set_socket, F_SETFL, O_NONBLOCK | flags))
    {
        // 设置失败
        printf("F_SETFL ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 最终表示成功
    return SUCCESSFUL;
}

/** 本地套接字初始化
 * @brief 对本地服务器端套接字的初始化，并将初始化之后的套接字赋值给主函数中的套接字变量
 * @param[in] net_arg_n 网络变量指针,这里只用与对套接字额文件描述符进行表示
 * @param[out] int 表示判断结果
 * */
int service_init(__socket_arg net_arg)
{
    // 定义本地套接字
    int location_socket = 0;
    // 定义网络变量
    __socket_arg location_net_arg;
    // 定义套接字地址变量
    sockaddr_in location_addr;

    // 初始化网络变量
    strcpy(location_net_arg.url, "lacalhost");
    strcpy(location_net_arg.ip, "");
    location_net_arg.remote_port = 0;
    location_net_arg.using_port = CONNECT_PORT;

    // 将location_socket设置为非阻塞式的
    if (SOCKET_ERROR == set_socket_nonblock(location_socket))
    {
        // 表示设置非阻塞式失败
        printf("SET SOCKET NONBLOCK ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 获取本机地址
    if (ERROR_APPEAR == addr_init(&location_addr, &location_net_arg))
    {
        // 获取本机地址失败
        printf("GET LOCATION ADDR ERROR.\r\n");
        return ERROR_APPEAR;
    }

    // 创建套接字
    location_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (location_socket == INVALID_SOCKET)
    {
        // 创建本地套接字失败
        printf("LOCATION SOCK ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 绑定套接字
    if (SOCKET_ERROR == bind(location_socket, (sockaddr *)&location_addr, sizeof(location_addr)))
    {
        // 绑定失败
        printf("BIND ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 监听套接字
    if (INVALID_SOCKET == listen(location_socket, MAX_LISTEN_NUM))
    {
        // 表示启动监听发生了错误
        printf("LISTEN ERROR.\r\n");
        return ERROR_APPEAR;
    }

    // 表示当前初始化套接字没有问题
    *net_arg.socket_point = location_socket;
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
    // 定义android端的套接字
    int android_socket;
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
    // android端的套接字初始化
    android_socket = INVALID_SOCKET;

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
        if (SUCCESSFUL == service_connect(*socket_arg.socket_point, &android_socket))
        {
            // 连接成功之后开始计数
            time_start = TIME_NOW;
        }
        else
        {
            // 没有安卓端的连接
            // 跳过之后的操作
            sleep(1);
            continue;
        }

        // 查看是否超时
        while (TIMEOUT_COUNT > difftime(TIME_NOW, time_start))
        {
            // 对当前消息状态和连接状态进行判断
            if (android_state == ANDROID_ONLINE)
            {

                // 接收消息队列中的消息
                // 只在线的时候读消息，主要是防止在发送数据时，将原先的消息给替代了
                recv_msg(msg_id, &music_msg);
                // 判断是否接收到消息
                if (ERROR_APPEAR == msg_empty(music_msg))
                    // 表示现在有消息过来,需要向android端传输数据
                    if (ERROR_APPEAR == send_response(RESPONSE_TYPE_TRANSfER_CODE, RESPONSE_LEN))
                    {
                        // 发送失败
                        printf("SEND RESPONSE FAIL.\r\n");
                    }
            }
            // 接收响应
            if (SUCCESSFUL == recv_response(android_socket, recv_buffer))
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
            // 判断当前android端的状态是否处于接收状态
            if (android_state == ANDROID_RECVING)
            {
                // 处于接收状态时，传输文件
                // 只传输一定量的数据，确保消息可以快速的接收到并进行处理
                if (SUCCESSFUL == transfer_music(music_msg, android_socket))
                {
                    // 传输成功，时间计数值清零
                    time_start = TIME_NOW;
                    // 将消息也进行清除
                    if (music_msg.msg_type == MSG_TYPE)
                    {
                        if (ERROR_APPEAR == msg_delete(&music_msg))
                        {
                            // 表示删除失败
                            printf("DELETE MSG ERROR.\r\n");
                        }
                    }
                }
                else
                {
                    // 表示传输的时候出错了或者已经传输完成
                    // 不再传输
                    android_state DOWN TO_ONLINE;
                }
            }
        }
        // 超时,断开连接
        service_disconnect(android_socket);
    }
}