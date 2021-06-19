/**\web_client.cpp
 * 简要介绍:从web端获取http报文
 * 作者:曾晨
 * 完成度:未完成
 * */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <multithread/service_thread.h>
#include <message_queue/music_msg.h>

// 线程结束处理函数
void end_handle(int signo);
// 与web客户端连接函数
int client_connect(__data_arg *net_arg_p);
// web套接字赋值函数
int addr_init(sockaddr_in *addr_p, __data_arg *net_arg_p);
// url解析函数
int parse_url(sockaddr_in *addr_p, char *url);
// 从域名中获得ip地址
int get_ip_addr(char *domain, char *ip_addr);

// 定义web端套接字地址
sockaddr_in web_addr;

/** 获取ip地址
 * @brief 通过域名得到相应的ip地址
 * @param[in] domain 指向域名变量的指针
 * @param[in] ip_addr 指向IP地址字符串字符串的指针
 * */
int get_ip_addr(char* domain,char* ip_addr)
{
    // 通过专门的函数来获取相应的IP地址
    struct hostent *host=gethostbyname(domain);
    // 判断ip地址是否成功获得
    if(host==NULL)
    {
        // 获取失败
        memset((void*)ip_addr,0,IP_ADDR_LEN);
        return -1;
    }
    // 其中在struct hostent 结构体中的h_addr_list数组中储存的就是相应
    // IP地址,一个网址可能有多个IP地址,这里只用第一个IP地址
    strcpy(ip_addr,inet_ntoa(*(struct in_addr*)host->h_addr));
    return 0;
}
/** url解析函数
 * @brief 对url的解析得到web端相应的地址和端口号
 * @param[in] addr_p 指向地址变量的指针
 * @param[in] url 指向url字符串的指针
 * @param[out] int 返回函数状态
 * */
int parse_url(sockaddr_in *addr_p, char *url)
{
    // 通过url解析出域名和端口号,只对这两个进行解析
    // 定义返回状态值
    int ret=-1;
    // 初始化一个变量,用于计数
    int amount = 0;
    // 初始化域开始位置变量,用于确定url中的域名的部分从那开始
    int start = 0;
    // 定义字符串指针,用于各个解析字符串的赋值
    char *pos;
    // 初始化端口号
    int port = CONNECT_PORT;
    // 定义域名
    char domain[MAX_URL_LEN];
    // 定义ip地址字符串
    char ip_addr[IP_ADDR_LEN];
    // 定义http协议,用于url解析
    char *patterns[] = {"http://", "https://", NULL};

    // 获得域名在url开始的位置,并保存在start中
    for (int i = 0; patterns[i]; i++)
    {
        if (strncmp(url, patterns[i], strlen(patterns[i])) == 0)
        {
            start = strlen(patterns[i]);
            break;
        }
    }

    // 从url中的解析出域名,这里处理时域名后面的端口好会保留
    strcpy(domain, &url[start]);
    pos = strstr(domain, ":");
    if (pos != 0)
    {
        // 格式化赋值得到端口号
        sscanf(pos, ":%d", &port);
        // 将端口号从域名中删除
        *pos = '\0';
    }
    // 通过domain获得ip地址
    get_ip_addr(domain, ip_addr);

    addr_p->sin_port = htons(port);
    addr_p->sin_addr.s_addr = inet_addr(ip_addr);
    // 判断其中是否是有效的
    if (addr_p->sin_addr.s_addr != INADDR_NONE)
    {
        ret = 0;
    }
    return ret;
}

/** 地址初始化函数
 * @brief 通过对ip的解析和port的解析或者对url的解析得到web端相应的地址和端口号
 * @param[in] net_arg_p 指向连接时可能需要使用到的参数
 * @param[in] addr_p 指向地址变量的指针
 * @param[out] int 连接成功返回套接字,连接失败则返回错误码
 * */
int addr_init(sockaddr_in *addr_p, __data_arg *net_arg_p)
{
    // 定义函数状态返回值
    int ret = -1;
    // 对地址中的协议进行定义,这里都使用IPv4
    addr_p->sin_family = AF_INET;
    // 一般以url中的地址和端口号为更高的解析优先级
    // 当url无法解析时,或者没有url时,才会使用已经定义好的IP和PORT
    if (0 == parse_url(addr_p, net_arg_p->url))
    {
        // 表示已经从url中解析得到了地址,并进行了相应的赋值操作
        ret = 0;
    }
    else
    {
        // 将net_arg中的关于web端的地址参数进行了赋值
        if (net_arg_p->remote_port == 0)
        {
            // 使用默认端口号
            net_arg_p->remote_port = CONNECT_PORT;
        }
        addr_p->sin_port = htons(net_arg_p->remote_port);
        addr_p->sin_addr.s_addr = inet_addr(net_arg_p->ip);
        // 判断其中是否是有效的
        if (addr_p->sin_addr.s_addr != INADDR_NONE)
        {
            ret = 0;
        }
    }
    return ret;
}

/** 客户端TCP连接函数
 * @brief 通过对socket的一些操作,使之可以与web服务器端连接
 * @param[in] net_arg_p 指向连接时可能需要使用到的参数
 * @param[out] int 连接成功返回套接字,连接失败则返回错误码
 * */
int client_connect(__data_arg *net_arg_p)
{
    // 创建套接字
    // 使用IPv4,面向连接,使用TCP连接
    int service_socket;
    service_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (service_socket == INVALID_SOCKET)
    {
        // 创建失败
        printf("SOCK INITIALIZATION ERROR\r\n");
        return -1;
    }

    // 初始化web套接字地址
    memset((void *)&web_addr, 0, sizeof(web_addr));

    // 对地址变量进行赋值
    // 与socket中的一些参数要一致
    if (-1 == addr_init(&web_addr, net_arg_p))
    {
        // 赋值错误
        printf("ADDR INITIALIZATION ERROR!\r\n");
        return -1;
    }
}

/**面向web的客户端函数
 * @brief 就是通过与web端进行tcp连接,并接受web端的http报文,对报文进行相应的响应,除此之外,还需要将http中的数据断作为参数
 *        传递给回调函数
 * @param[in] arg中包含了回调函数首地址和一些需要的参数
 * */
void *web_client(void *arg)
{
    // 格式化输入参数格式
    __service_thread_arg thread_arg = *(__service_thread_arg *)arg;

    // 向web端进行连接
    int socket;

    while (1)
    {
    }
}