#ifndef __SOCKET_H
#define __SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// 定义连接时对方的默认端口号
#define CONNECT_PORT (80)

// 定义一些错误标识
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

// 一些缓存区大小的定义
#define MAX_LISTEN_NUM (1)
#define RECV_BUF_SIZE (100)
#define SEND_BUF_SIZE (100)
#define LONGEST_FILE_NAME (100)

// 定义下载地址符串个数、保存文件路径字符串个数、文件名字符串个数、下载任务数的最大值
#define MAX_URL_LEN (200)
#define MAX_PATH_LEN (100)
#define MAX_FILE_NAME_LEN (100)
#define MAX_FILE_POSITION_LEN (200)

// 定义最大http报文头部中content-typ类型字符串大小
#define MAX_CONTENT_TYPE_LEN (128)
// 定义IPv4地址字符串长度
#define IP_ADDR_LEN (16)
// 定义域字符串大小
#define DOMAIN_LEN (64)
// 定义http请求报文字符串长度
#define HTTP_HEADER_LEN (2048)
// 定义http内容部分接收字符串长度
#define HTTP_CONTENT_LEN (2048)

#endif