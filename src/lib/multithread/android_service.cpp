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

// 定义一个android一次性可以传输的文件数目/10的值
#define MAX_TRANSFER_MUSIC_NUM (10)

// 定义一个用于android端的专门变量，用于多android端连接
typedef struct ADNROID_TRANSFER_DATA
{
    // 定义当前android端接收到的的消息的音乐文件个数，主要用于传输
    // 音乐总数
    int total_music_num;
    // 音乐文件列表集合s
    struct __MUSIC_INFO *music_total_list[MAX_TRANSFER_MUSIC_NUM];
    // 传输文件保留时间戳
    time_t time_start;
} android_transfer_data;

// 定义一个用于android端网络需要使用到的所有参数
typedef struct ANDROID_TOTAL_DATA
{
    // 设备ID
    int android_id;
    // 设备名称
    char android_name[MAX_DEVICE_NAME_LEN];
    // 该android端的套接字
    int android_socket;
    // 该android端的网络状态
    int android_state;
    // 该android的周期
    time_t time_start;
    // 该andoird端需要传输的数据
    android_transfer_data transfer_data;
    // 该设备的传输线程id
    pthread_t tid;
} android_total_data;

typedef struct ANDROID_ADMIN
{
    // 定义最大连接数变量
    int max_connect_num;
    int connected_num;
    // 定义当前接收到需要传输音乐消息的位置
    int send_position;
    // 定义一个管理android端的一个指针
    android_total_data *android_list;
} android_admin;

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

// 定义一个本文件的全局变量
int location_socket;

// 定义改变方式
#define UP |=
#define DOWN &=

// 定义超时计数值
#define TIMEOUT_COUNT (60)
#define TIME_NOW (time(NULL))
// 定义select超时时间
#define ACCEPT_UTIMEOUT (1000)
#define RECV_UTIMEOUT (100)
#define ACCEPT_TIMEOUT (1)
#define RECV_TIMEOUT ACCEPT_TIMEOUT

// 定义应答协议中的状态字符串长度
#define RESPONSE_LEN (2)
// 定义应答协议中的状态字符串
const char RESPONSE_TYPE_OK_CODE[RESPONSE_LEN] = {0X04, 0X01};
const char RESPONSE_TYPE_TRANSfER_CODE[RESPONSE_LEN] = {0X04, 0X02};

// 接收消息函数
int recv_msg(int msg_id, android_admin android);
// 判断消息列表中是否有消息
int msg_empty(const __music_msg music_msg);
// 判断当前是否有客户端连接上*
int connect_verify(const int socket);
// 服务器端套接字初始化
int service_init(__socket_arg net_arg);
// 服务器端连接线程,支持多设备连接
void *service_connect(void *android);
// 服务器端断开连接
int service_disconnect(int disconnect_socket);
// 接收安卓端状态数据线程
void *recv_response(void *android);
// 创建传输音乐文件线程
int creat_transfer_music_thread(android_admin android);
// 删除消息内容
int transfer_data_delete(android_transfer_data *transfer_data);
// 对接收到的消息进行响应
int response_android(android_total_data *android_arg, char *recv_buffer);
// 发送响应
int send_response(int recv_socket, const char *send_buffer, int len);
// 设置socket为非阻塞状态
int set_socket_nonblock(int set_socket);
//  android端的管理变量初始化
int android_init(android_admin *const android, const int max_connected_num);
// 对android的周期进行判断
int android_change(android_admin *android);
// 从android_admin变量中找到一个空闲的android_total_data数据储存区的位置
int android_find_free_time(android_admin android);
// 传输线程
void *transfer_data(void *arg);

/** 传输线程函数
 * @brief 传输参数中所定义的文件
 * @param[in] arg 指向需要传输参数的android_total_data变量
 * */
void *transfer_data(void *arg)
{
    // 设置本线程是分离式的
    pthread_detach(pthread_self());

    // 获取传输列表
    android_total_data *android_arg = (android_total_data *)arg;
    // 当期所传输文件数量
    int music_count = 1;
    // 定义文件表示符
    int fp=0;
    // 定义当前文件传输掉的字节数
    int file_len = 0;
    // 当前传输的音乐文件所在的位置
    int position_ln = 0;
    int position_col = 0;
    // 定义发送缓存区
    char *send_buffer[SEND_BUF_SIZE] = {0};
    // 传输数据
    while (music_count <= android_arg->transfer_data.total_music_num)
    {
        // 当还有文件需要传输时
        // 判断当前是否处于发送状态
        if (android_arg->android_state != ANDROID_RECVING)
        {
            // 不在该状态下不传输文件
            sleep(1);
            continue;
        }
        // 传输文件
        fp = fp ? fp : open(android_arg->transfer_data.music_total_list[position_ln][position_col].file_path, O_RDONLY);
        if (fp == ERROR_APPEAR)
        {
            // 读取文件失败
            printf("OPRN FILE ERROE.\r\n");
            continue;
        }
        // 开始传输该文件
        // 读取文件
        if (0 >= read(fp, send_buffer, SEND_BUF_SIZE))
        {
            // 表示当前文件已经全部读取或者读取失败
            // 跳转到下一个文件
            fp = 0;
            // 传输文件数加1
            music_count++;
            // 修改位置
            position_col++;
            // 一个判断标准，通过对该行中的音乐文件的id进行一个判断，就可以知道之后还有没有歌曲了
            if (android_arg->transfer_data.music_total_list[position_ln][position_col].id == 0)
            {
                position_col = 0;
                position_ln++;
            }
        }
        // 将读取到的数据发送出去
        else
        {
            if (0 > send(android_arg->android_socket, send_buffer, SEND_BUF_SIZE, 0))
            {
                // 表示发送错误
                printf("SEND ERROR.\r\n");
            }
        }
    }
    // 结束线程
    pthread_exit(0);
}

/** 查找空闲函数
 * @brief 从android_admin变量中找到一个空闲的android_total_data数据储存区的位置
 * @param[in] android 指向android端管理变量
 * @param[out] int 返回查找结果
 * */
int android_find_free_time(android_admin android)
{
    // 查找空闲位置
    for (int i = 0; i < android.max_connect_num; i++)
    {
        if (android.android_list[i].android_socket)
        {
            return i;
        }
    }
    // 没有空闲位置时
    return -1;
}

/** android端管理变量初始化
 * @brief 对android端的管理变量进行初始化，规定最大连接数量
 * @param[in] android 指向android端管理变量
 * @param[in] max_connected_num 最大连接数量，主要用于确认最大的连接数量的
 * @param[out] int 初始化结果
 * */
int android_change(android_admin *android)
{
    // 有设备连接就判断是否需要根据时间周期来对相应的设备进行修改
    for (int i = 0; i < android->max_connect_num; i++)
    {
        // 遍历设备数据列表
        // 判断该设备是否有效
        if (android->android_list[i].android_socket == INVALID_SOCKET)
        {
            // 跳过
            continue;
        }
        // 对其android进行判断
        if (TIMEOUT_COUNT <= difftime(android->android_list[i].time_start, TIME_NOW))
        {
            // 当期大于时间周期时，将其断开，并清除其中的一些数据
            // 不管是在线还是下线都可以
            if (ERROR_APPEAR == service_disconnect(android->android_list[i].android_socket))
            {
                // 断开失败
                printf("SHUTDOWN %d ERROR.\r\n", android->android_list[i].android_socket);
            }
            // 对数据进行清除
            android->connected_num--;
            for (int j = 0; android->android_list[i].transfer_data.music_total_list[j] != NULL; j++)
            {
                // 清除歌曲列表
                free(android->android_list[i].transfer_data.music_total_list[j]);
                android->android_list[i].transfer_data.music_total_list[j] = NULL;
            }
            memset(&android->android_list[i], 0, sizeof(ANDROID_TOTAL_DATA));
            android->android_list[i].android_socket = INVALID_SOCKET;
        }
        else if (TIMEOUT_COUNT <= difftime(android->android_list[i].transfer_data.time_start, TIME_NOW))
        {
            // 设备响应没超时，还需要查看需要传输的数据是否超时
            transfer_data_delete(&android->android_list[i].transfer_data);
        }
    }
    // 判断当前是否有设备连接
    if (android->connected_num == 0)
    {
        // 没有设备连接的话
        return ERROR_APPEAR;
    }
    // 还有设备连接
    return SUCCESSFUL;
}

/** android端管理变量初始化
 * @brief 对android端的管理变量进行初始化，规定最大连接数量
 * @param[in] android 指向android端管理变量
 * @param[in] max_connected_num 最大连接数量，主要用于确认最大的连接数量的
 * @param[out] int 初始化结果
 * */
int android_init(android_admin *android, const int max_connected_num)
{
    // 对android端的变量进行初始化
    android->max_connect_num = max_connected_num;
    android->connected_num = 0;
    android->send_position = -1;
    android->android_list = (android_total_data *)malloc(sizeof(android_total_data) * max_connected_num);
    if (android->android_list == NULL)
    {
        // 表示初始化失败
        printf("ANDROID INIT ERROR.\r\n");
        return ERROR_APPEAR;
    }
    // 初始化成功
    // 很多初始变量都是0,直接让其全为0
    memset(android->android_list, 0, sizeof(android_total_data) * max_connected_num);
    for (int i = 0; i < max_connected_num; i++)
    {
        // 对套接字进行初始化
        android->android_list[i].android_socket = INVALID_SOCKET;
    }
    return SUCCESSFUL;
}

/** 传输数据
 * @brief 对数据进行传输
 * @param[in] send_buffer 指向发送缓存区，用于发送
 * @param[in] len 发送数据的长度
 * @param[out] int 返回传输状态
 * */
int send_response(int recv_socket, const char *send_buffer, int len)
{
    // 通过send函数进行发送
    return send(recv_socket, send_buffer, len, 0) > 0 ? SUCCESSFUL : ERROR_APPEAR;
}

/** 文件传输函数
 * @brief 通过对消息的判断，实现一定量的文件传输，只传送一定量
 * @param[in] music_arg 指向消息结构体的指针，主要用于储存传递过来的消息
 * @param[in] recv_socket 需要传输的套接字文件描述符
 * @param[out] int 返回传输状态
 * */
int creat_transfer_music_thread(android_admin android)
{
    // 定义返回变量
    int res = ERROR_APPEAR;
    // 定义发送缓存区
    char send_buffer[SEND_BUF_SIZE] = {0};

    // 对需要传输的android端创建线程
    for (int i = 0; i < android.max_connect_num; i++)
    {
        // 遍历整个设备列表
        if (android.android_list[i].android_socket == INVALID_SOCKET)
        {
            // 跳过
            continue;
        }
        // 判断是否需要创建线程
        if (android.android_list[i].android_state == ANDROID_RECVING)
        {
            if (android.android_list[i].tid == INEXISTENCE_THREAD_ID)
            {
                // 当前需要创建一个线程来传输数据
                if (ERROR_APPEAR == pthread_create(&android.android_list[i].tid, NULL, transfer_data, (void *)&android.android_list[i]))
                {
                    // 创建失败
                    printf("CREAT %d TRANSFER THREAD ERROR.\r\n", android.android_list[i].android_socket);
                }
            }
        }
        else if (android.android_list[i].tid != INEXISTENCE_THREAD_ID && android.android_list[i].transfer_data.time_start == 0)
        {
            // 需要判断该tid是否还存在
            if (SUCCESSFUL == pthread_kill(android.android_list[i].tid, 0))
            {
                // 表示还存在
                // 发送终止消息
                pthread_cancel(android.android_list[i].tid);
            }
            // 对tid清零
            android.android_list[i].tid = INEXISTENCE_THREAD_ID;
        }
    }
    // 表示成功
    return SUCCESSFUL;
}

/** 对接收到的消息进行响应
 * @brief 对接收到的消息进行响应的响应
 * @param[in] android_arg android端参数变量指针
 * @param[in] recv_buffer 接收缓存区的地址，用于保存接收到的数据
 * @param[out] int 返回响应状态
 * */
int response_android(android_total_data *android_arg, char *recv_buffer)
{
    // 表示响应返回值
    int res = SUCCESSFUL;
    // 求和数
    int sum = 0;
    // 对接收消息的第一位进行判断
    switch (recv_buffer[0])
    {
    case 0x00:
        // 表示没有消息发过来的
        break;
    case 0x01:
        // 判断接收到的消息是否出错
        for (int i = 1; i <= 10; i++)
        {
            // 求和判断
            sum += recv_buffer[i];
        }
        if (sum != recv_buffer[11])
        {
            return ERROR_APPEAR;
        }
        // 上下线
        if (recv_buffer[1] == 0x01)
        {
            // 表示上线
            printf("ONLINE.\r\n");
            android_arg->android_state UP TO_ONLINE;
            android_arg->time_start = time(NULL);
            android_arg->android_id = recv_buffer[10];
        }
        else if (recv_buffer[1] == 0x00)
        {
            // 表示下线
            printf("%d:OFFLINE.\r\n",android_arg->android_socket);
            android_arg->android_state DOWN TO_OFFLINE;
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
            printf("%d:TRANSFER START.\r\n", android_arg->android_socket);
            android_arg->android_state DOWN TO_ONLINE;
        }
        // 判断当前响应类型是否是准备好啦并且当前该设备也在线
        else if (recv_buffer[1] == 0x01)
        {
            // 表示可以向android端发送数据
            printf("%d:TRANSFER OFF.\r\n", android_arg->android_socket);
            android_arg->android_state UP TO_RECVING;
            if (android_arg->android_state == ANDROID_UNDEFINED)
            {
                // 如果出现了未定义的值
                android_arg->android_state DOWN TO_OFFLINE;
            }
        }
        break;
    default:
        // 表示接收到了无法识别的消息
        res = ERROR_APPEAR;
    }
    // 表示响应成功
    recv_buffer[0] = 0;
    return res;
}

/** 接收响应函数
 * @brief 从android端接收响应，并保存在相应的缓存区内
 * @param[in] android android端管理变量指针
 * */
void *recv_response(void *android)
{
    // 获取android端管理变量
    // 不需要改变连接数量这个值
    android_admin android_arg = *(android_admin *)android;

    // 定义select中需要使用到的参数
    fd_set readfds;
    struct timeval timeout;
    int maxfd;
    int res;
    // 定义接收缓存区
    char recv_buffer[RECV_BUF_SIZE] = {0};

    // 超时时间的设置
    timeout.tv_usec = 0;
    timeout.tv_sec = RECV_TIMEOUT;
    // 主循环体
    while (1)
    {
        // 同样的通过select实现一定时间阻塞的接收
        // 判断套接字是否为空
        if (android_arg.connected_num == 0)
        {
            // 如果为空
            continue;
        }

        // 进行更新
        maxfd = -1;
        FD_ZERO(&readfds);
        for (int i = 0; i < android_arg.max_connect_num; i++)
        {
            if (android_arg.android_list[i].android_socket != INVALID_SOCKET)
            {
                // 将连接上了的socket添加到readfds中
                FD_SET(android_arg.android_list[i].android_socket, &readfds);
                if (android_arg.android_list[i].android_socket > maxfd)
                {
                    // 更新maxfd
                    maxfd = android_arg.android_list[i].android_socket;
                }
            }
        }

        // select判断接收响应
        // select中是对过程作出判断还是结果作出判断
        res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (res == ERROR_APPEAR)
        {
            // 表示select中发生了错误
            printf("RECV SELECT ERROR.\r\n");
            continue;
        }
        if (res == 0)
        {
            // 表示select超时
            continue;
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
                for (int j = 0; j < android_arg.connected_num; j++)
                {
                    // 查找与i相同的套接字的位置
                    if (android_arg.android_list[j].android_socket == i)
                    {
                        // 对该套接字的响应进行处理
                        if (ERROR_APPEAR == response_android(&android_arg.android_list[j], recv_buffer))
                        {
                            // 表示android端传输过来了一些不在协议内的数据
                            printf("RESPONSE ERROR.\r\n");
                        }
                        else
                        {
                            android_arg.android_list[j].time_start = TIME_NOW;
                        }
                    }
                }
            }
        }
        // 表示发生了一些出乎意料的错误
        printf("RECV RESPONSE ERROR.\r\n");
    }
}

/** 删除消息内容函数
 * @brief 将消息队列中的列表指针指向的储存区格式化，并将其他消息内容初始化
 * @param[in] music_arg 指向消息结构体的指针，主要用于储存传递过来的消息
 * @param[out] int 返回删除状态
 * */
int transfer_data_delete(android_transfer_data *transfer_data)
{
    for (int j = 0; j < MAX_TRANSFER_MUSIC_NUM; j++)
    {
        // 遍历清除消息列表数据
        if (transfer_data->music_total_list[j] == NULL)
        {
            // 清除完成
            break;
        }
    }
    memset(transfer_data, 0, sizeof(android_transfer_data));
    return SUCCESSFUL;
}

/** 接收消息函数
 * @brief 主要就是从消息队列中读取消息并保存在相应的变量当中,当相应的变量中有数据时不读取
 * @param[in] msg_id 消息队列文件描述符,用于消息队列的读取
 * @param[in] music_msg 指向需要保存消息内容的变量,用于消息内容的保存
 * @param[out] int 函数状态返回值,当为ERROR_APPEAR时,表示接收中出现了错误 
 * */
int recv_msg(int msg_id, android_admin android)
{
    // 定义一个消息结构体变量
    __music_msg music_msg;
    // 接收到的消息对应的位置
    int position;

    // 初始化
    msg_init(&music_msg);

    // 从消息队列中接收消息
    if (0 > msgrcv(msg_id, (void *)&music_msg, MSG_SIZE, MSG_TYPE, IPC_NOWAIT))
    {
        // 表示没有接收到消息
        return ERROR_APPEAR;
    }
    // 表示接收到了消息
    // 需要将接收到的消息中有用的部分储存到相应的位置
    for (int position = 0; position < android.max_connect_num; position++)
    {
        // 遍历整个列表
        if (android.android_list[position].android_socket != INVALID_SOCKET &&
            android.android_list[position].android_id == music_msg.device_id)
        {
            break;
        }
    }
    // 如果该设备不在连接的设备当中
    if (position == android.max_connect_num)
    {
        // 不再连接的设备之中
        // 报错
        printf("THIS DEVICE INEXISTENCE.\r\n");
        return ERROR_APPEAR;
    }
    // 在的话，对其进行接收
    // 更新时间
    android.android_list[position].transfer_data.time_start = TIME_NOW;
    for (int i = 0; i < MAX_TRANSFER_MUSIC_NUM; i++)
    {
        // 查找列表中空闲的位置
        if (android.android_list[position].transfer_data.music_total_list[i] == NULL)
        {
            // 还有空闲位置的话
            android.android_list[position].transfer_data.music_total_list[i] = music_msg.music_info_list;
            android.android_list[position].transfer_data.total_music_num += music_msg.music_num;
            // 返回成功
            return SUCCESSFUL;
        }
    }
    printf("HAVE NOT SPACE TO RECV MSG.\r\n");
    // 没有空闲位置了
    return ERROR_APPEAR;
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
 * @brief 用于对信道的监听,确保android端的连接
 * @param[in] android android管理变量
 * */
void *service_connect(void *android)
{
    // 获取android管理变量
    android_admin *android_arg = (android_admin *)android;
    // 连接android端的套接字
    int connect_socket;
    // 定义连接成功需要连接的位置
    int position = -1;

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
    while (1)
    {
        // 判断是否已经到达最大连接数
        if (android_arg->connected_num == android_arg->max_connect_num)
        {
            // 到达来将不再监听
            sleep(1);
            continue;
        }
        // 进行select判断
        res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (res == ERROR_APPEAR)
        {
            // 表示select时发生了错误
            printf("SELECT ERROR.\r\n");
            continue;
        }
        if (res == 0)
        {
            // 表示select时超时
            sleep(1);
            continue;
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
            continue;
        }
        // 表示连接成功
        position = android_find_free_time(*android_arg);
        if (position == ERROR_APPEAR)
        {
            // 表示已经满了
            printf("ANDROID DEVICE CONNECT ERROR.\r\n");
            continue;
        }
        // 表示连接上了
        android_arg->android_list[position].android_socket = connect_socket;
        android_arg->connected_num++;
        android_arg->android_list[position].time_start = TIME_NOW;
    }
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
    strcpy(location_net_arg.url, "");
    strcpy(location_net_arg.ip, "0.0.0.0");
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
    if (INVALID_SOCKET == listen(location_socket, MAX_CONNECTED_NUM))
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
    // 定义android端管理变量
    android_admin android;
    // 定义两个线程id
    pthread_t connect_thread_id;
    pthread_t recv_thread_id;

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

    // 将本地socket赋值给全局变量
    location_socket = *socket_arg.socket_point;

    // 初始化两个线程
    // 一个是连接线程，一个是接收响应线程
    if (SUCCESSFUL != pthread_create(&connect_thread_id, NULL, service_connect, (void *)&android))
    {
        // 表示创建出错
        printf("CREAT THREAD ERROR.");
        exit(1);
    }
    if (SUCCESSFUL != pthread_create(&recv_thread_id, NULL, recv_response, (void *)&android))
    {
        // 表示创建出错
        printf("CREAT THREAD ERROR.");
        exit(1);
    }

    while (1)
    {
        // 判断当前是否有android端连接进来
        // 对android的周期进行判断，改变android的状态
        if (ERROR_APPEAR == android_change(&android))
        {
            // 没有安卓端的连接
            // 跳过之后的操作
            sleep(1);
            continue;
        }

        // 接收消息队列中的消息
        // 判断是否接收到消息
        if (SUCCESSFUL == recv_msg(msg_id, android))
            // 表示现在有消息过来,需要向android端传输数据
            if (ERROR_APPEAR == send_response(android.android_list[android.send_position].android_socket, RESPONSE_TYPE_TRANSfER_CODE, RESPONSE_LEN))
            {
                // 发送失败
                printf("SEND RESPONSE FAIL.\r\n");
            }
        // 传输音乐文件
        creat_transfer_music_thread(android);
    }
}
