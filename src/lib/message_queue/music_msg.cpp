/**\music_msg.cpp
 * 简要介绍:主要是实现关于消息队列的一些操作
 * 作者:曾晨
 * 完成度:未完成
 * */

#include<message_queue/music_msg.h>


/**初始化消息队列函数
 * @brief 创建消息队列
 * @param[out] 返回消息队列ID，失败则返回-1
 * */
int init_msg(key_t key)
{
    return msgget(key,0666|IPC_CREAT);
}

/**反初始化消息队列函数
 * @brief 销毁消息队列
 * @param[out] 返回销毁状态
 * */
int deinit_msg(key_t key)
{
    return msgctl(key,IPC_RMID,0);
}