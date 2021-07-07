#ifndef _MUSIC_MSG_H
#define _MUSIC_MSG_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <transfer_service/transfer_service.h>

// 定义一些字符串的限制
#define MAX_TITLE_LEN (128)
#define MAX_INTRO_LEN (1024)
#define MAX_SINGER_LEN MAX_TITLE_LEN
#define MAX_FILE_PATH_LEN MAX_INTRO_LEN
#define MAX_DEVICE_NAME_LEN MAX_TITLE_LEN

// 定义关于消息队列的一些宏
#define MSG_TYPE (1)
#define MSG_TYPE_INVAILD (0)
#define MAX_MUSIC_NUM (10)

// 定义消息结构体大小
#define MSG_SIZE (sizeof(__music_msg) - sizeof(long))
// 查找音乐文件
#define FIND_MUSIC "../music/%d_%s_%s.mp3"

// 声明歌曲信息结构体
struct __MUSIC_INFO
{
    int id;
    char title[MAX_TITLE_LEN];
    char intro[MAX_INTRO_LEN];
    char singer[MAX_SINGER_LEN];
    char file_path[MAX_FILE_PATH_LEN];
};

// 声明消息队列消息结构体
typedef struct __MUSIC_MSG
{
    long msg_type;
    char device_name[MAX_DEVICE_NAME_LEN];
    long device_id;
    int music_num;
    struct __MUSIC_INFO *music_info_list;
} __music_msg;


// 初始化消息队列
int init_msg(key_t key);
// 反初始化消息队列
int deinit_msg(key_t key);

#endif