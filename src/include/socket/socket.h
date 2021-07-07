#ifndef __SOCKET_H
#define __SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<transfer_service/transfer_service.h>

// 定义连接时对方的默认端口号
#define CONNECT_PORT (7800)

// 定义一些错误标识
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

// 一些缓存区大小的定义
#define MAX_LISTEN_NUM (5)
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
// 定义http请求报文字符串长度
#define HTTP_CONTENT_LEN (2048)
// 定义http内容部分接收字符串长度
#define HTTP_CONTENT_LEN (2048)
// 定义http方式字符串的长度
#define HTTP_METHOD_LEN (10)
// 定义hrttp请求内容方式的长度
#define HTTP_CONTENT_TYPE_LEN (1024)
// 定义http响应开始行长度
#define HTTP_RESPONSE_STSRT_LINE_LEN (256)
// 定义http响应代码字符串数据长度
#define HTTP_RESPONSE_STRING_LEN (35)
// 定义http响应代码数组大小
#define HTTP_RESPONSE_CODE_MUN (150)

// 定义http响应状态码
#define OK (200)
#define BAD_REQUEST (400)
#define METHOD_NOT_ALLOWED (405)
#define REQUESR_ENTITY_TOO_LARGE (413)
#define NOT_FOUND (404)


// 通过散列的方式来得到相应的字符串
#define GET_HTTP_RESPONSE_STRING(X) (X/100>4?100+X%100:( X/100-1 )*15+X%100)

// 定义socket中需要使用到的参数
typedef struct __SOCKET_ARG
{
    // 需要使用到的套接字地址
    int *socket_point;
    // 需要使用到的端括号
    int using_port;
    // 定义对方的url
    char url[MAX_URL_LEN];
    // 定义对方的ip
    char ip[IP_ADDR_LEN];
    // 定义对方的port
    int remote_port;
}__socket_arg;

// 定义http报文头部字段名数据类型
typedef struct __HTTP_REQUEST_HEADER
{
    // 需要使用到的请求头部字段数据
    char HTTP_method[HTTP_METHOD_LEN];
    char url[MAX_URL_LEN];
    float HTTP_version;
    char Host[MAX_URL_LEN];
    char Content_Type[HTTP_CONTENT_LEN];
    long Content_length;
} __http_request_header;

// 与web客户端连接函数
int client_connect(__socket_arg *net_arg_p);
// web套接字赋值函数
int addr_init(sockaddr_in *addr_p, __socket_arg *net_arg_p);
// url解析函数
int parse_url(sockaddr_in *addr_p, char *url);
// 从域名中获得ip地址
int get_ip_addr(char *domain, char *ip_addr);
// 接收Http报文
int http_recv(int web_socket,char **content);
// 对http请求报文头部的接收
int http_header_recv(int web_socket, char **header);
// http请求头部的分解
int parse_http_request(const char *header, __http_request_header *title);
// 对http请求报文内容的接收
int http_content_recv(int web_socket, char **content, __http_request_header title);
// 对http请求报文的响应
int http_response(int web_socket, int http_response_mun);
//  找出str2字符串在str1字符串中第一次出现的位置(不区分大小写)
const char *strstri(const char *str1, const char *str2);
// http响应状态字符串初始化
int http_response_status_string_init(void);

#endif