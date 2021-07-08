/**\callback_func.cpp
 * 简要介绍:主要是实现android和web端线程的回调任务
 * 详细说明:其中web端线程的回调函数主要就是实现对http报文的处理,并且将提取到的信息传输到消息列表中
 *         android端的回调函数主要就是实现对android端中的数据编写成日志
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <callback/callback_func.h>
#include <message_queue/music_msg.h>
#include <socket/socket.h>


/** android回调函数
 * @brief 测试
 * @param[in] arg 调用回调函数需要的参数
 * @param[in] str 调用该函数的日志信息
 * */
int android_callback_func(void *str, __data_arg arg)
{
    printf("singer:%s\r\n\r\n", (char *)str);
    return OK;
}

/**web回调函数
 * @brief 通过对web端接收到的请求字符串进行处理,打包之后发送给面向android端的线程
 * @param[in] arg 调用回调函数需要的参数
 * @param[in] str 调用该函数的日志信息
 * */
int web_callback_func(void *str, __data_arg arg)
{
    printf("web callback:%s\r\n\r\n", (char *)str);
    // 对接收到的请求进行处理
    // 复制请求内容，防止对请求内容造成破坏
    char *query = NULL;
    query = (char *)malloc(sizeof(char) * (strlen((char *)str) + 1));
    if (query == NULL)
    {
        // 当内存分配错误时
        printf("MALLOC ERROR.\r\n");
        return REQUESR_ENTITY_TOO_LARGE;
    }
    strcpy(query, (char *)str);

    // 对内容进行解析
    char *pos = NULL;
    const char *delim = "?";
    struct __MUSIC_INFO music_info;
    struct __MUSIC_INFO *music_info_list = NULL;
    __music_msg music_arg;
    char device_name[MAX_DEVICE_NAME_LEN] = {0};
    long device_id = 0;

    // 定义返回值
    int res = BAD_REQUEST;

    // 对消息变量进行初始化
    msg_init(&music_arg);
    // 定义当前音乐文件个数
    int num = 0;
    music_info_list = (struct __MUSIC_INFO *)malloc(sizeof(struct __MUSIC_INFO) * MAX_MUSIC_NUM);
    // 进行解析，并赋值给消息变量当中
    if (pos = strtok(query, delim))
    {
        do
        {
            // 解析出需要的参数
            sscanf(pos,
                   "id=%d&name=%s&title=%s&%*s&updateTime=%*s&deviceID=%ld&deviceName=%s",
                   &music_info.id, music_info.singer, music_info.title, &device_id, device_name);
            // 获得文件地址
            sprintf(music_info.file_path, FIND_MUSIC, music_info.id, music_info.title, music_info.singer);
            // 判断该文件是否存在
            if (ERROR_APPEAR == access(music_info.file_path, R_OK))
            {
                // 当没有找到的时候就跳过这条请求
                continue;
            }
            // 存在，获取需要传输到那个设备上的信息
            if (music_arg.device_id == 0)
            {
                // 或者是新的开始的
                // 对向前消息确定一个发送对象
                music_arg.device_id = device_id;
                strcpy(music_arg.device_name, device_name);
            }
            // 判断当前消息是否是与需要发送的消息是同一个设备
            if (music_arg.device_id == device_id)
            {
                // 是的话就将该音乐添加到音乐列表当中
                memcpy(&music_info_list[num++], &music_info, sizeof(music_info));
                // 只要有一个音乐文件是可以找到的，就表示成功
                res = OK;
            }
            // 判断当前是否需要发送消息
            // 当音乐列表存满的是否需要发送，并且当但前需要发送的音乐不属于当前消息发送的设备时
            if (num == MAX_MUSIC_NUM || device_id != music_arg.device_id)
            {
                // 将当前的消息发送出去
                music_arg.music_num = num;
                music_arg.music_info_list = music_info_list;
                if (ERROR_APPEAR == msgsnd(arg.msg_id, &music_arg, MSG_SIZE, IPC_NOWAIT))
                {
                    // 当消息发送失败时
                    printf("MSG SEND ERROR.\r\n");
                    return BAD_REQUEST;
                }
                msg_init(&music_arg);
                music_info_list = (struct __MUSIC_INFO *)malloc(sizeof(struct __MUSIC_INFO) * MAX_MUSIC_NUM);
                if (device_id != music_arg.device_id)
                {
                    // 如果不是的话，还需要进行其他操作
                    music_arg.device_id = device_id;
                    strcpy(music_arg.device_name, device_name);
                    memcpy(&music_info_list[num++], &music_info, sizeof(music_info));
                }
            }

        } while (pos = strtok(NULL, delim));
    }
    // 如果当前消息结构里面有数据的话，就将其也发送出去
    if (num == 0)
    {
        // 表示没有数据要发
        // 释放当前列表储存区
        free(music_info_list);
        // 判断是否是一个音乐文件都没有找到
        if (res == BAD_REQUEST)
        {
            // 没有
            res = NOT_FOUND;
        }
    }
    // 表示还有消息要发送
    else
    {
        // 将当前的消息发送出去
        music_arg.music_num = num;
        music_arg.music_info_list = music_info_list;
        if (ERROR_APPEAR == msgsnd(arg.msg_id, &music_arg, MSG_SIZE, IPC_NOWAIT))
        {
            // 当消息发送失败时
            printf("MSG SEND ERROR.\r\n");
            return BAD_REQUEST;
        }
    }

    return res;
}