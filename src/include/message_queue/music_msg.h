#ifndef _MUSIC_MSG_H
#define _MUSIC_MSG_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// 定义一些字符串的限制
#define MAX_TITLE_LEN (128)
#define MAX_INTRO_LEN (1024)
#define MAX_SINGER_LEN MAX_TITLE_LEN

// 定义关于消息队列的一些宏
#define MSG_TYPE (1)
#define MSG_TYPE_INVAILD (0)
#define MSG_SIZE (sizeof(char*)+sizeof(int))

// 声明歌曲信息结构体
struct __MUSIC_INFO
{
    int id;
    char title[MAX_TITLE_LEN];
    char intro[MAX_INTRO_LEN];
    char singer[MAX_SINGER_LEN];
};

// 声明消息队列消息结构体
typedef struct __MUSIC_MSG
{
    long msg_type;
    int music_num;
    struct __MUSIC_INFO *music_info_list;
} __music_msg;

// 初始化消息队列
int init_msg(key_t key);
// 反初始化消息队列
int deinit_msg(key_t key);

#endif