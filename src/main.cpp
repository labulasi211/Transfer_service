/**\Transfer_service
 * 简要说明:承担"中间件"作用
 * 详细说明:承接 web 后端音乐管理系统与 android 前端设备,将一部分通信以及文件传输交由 c 语言写成的中转服务来完成
 * 作者:曾晨
 * 完成度:未完成
 * */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/msg.h>

// 定义了TCP客户端和服务端使用的端口号
#define SERVICE_PORT 80
#define CLIENT_PORT 81


// 声明面向web的客户端线程和面向Android的服务端线程
pthread_t service_id=0;
pthread_t client_id=0;
