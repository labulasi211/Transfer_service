/**\android_serivce.cpp
 * 简要介绍:实现与android之间的tcp通信,实现一些文件传输功能
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <string.h>
#include<unistd.h>
#include <stdio.h>
#include <sys/socket.h>

#include <multithread/service_thread.h>
#include<message_queue/music_msg.h>

void *android_service(void *arg) 
{
    // 对参数中的数据进行格式化
    __service_thread_arg thread_arg = *(__service_thread_arg *)arg;
    // 定义消息内容
    __music_msg music_msg;
    memset((void *)(&music_msg), 0, sizeof(music_msg));

    while (1)
    {
        // 接收消息队列中的消息
        if (msgrcv(thread_arg.msg_id, (void *)&music_msg, sizeof(music_msg), 1, 0) > 0)
        {
            printf("recv:\r\n");
            for (int i = 0; i < music_msg.music_num; i++)
            {
                printf("id:%d\r\nmusic_name:%s\r\nmusic_intro:%s\r\n"\
                , music_msg.music_info_list[i].id, music_msg.music_info_list[i].title\
                , music_msg.music_info_list[i].intro);
                thread_arg.callback((void*)music_msg.music_info_list[i].singer,thread_arg.msg_id);
            }
        }
        else
        {
            printf("recv fail!\r\n");
        }
        sleep(1);
    }
}