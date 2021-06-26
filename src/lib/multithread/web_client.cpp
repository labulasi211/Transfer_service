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
// 接收Http报文
int http_recv(char **content);
// 对http请求报文头部的接收
int http_header_recv(int web_socket, char **header);
// http请求头部的分解
int parse_http_request(char *header, __http_request_title *title);
// 对http请求报文内容的接收
int http_content_recv(int web_socket, char **content, __http_request_title title);
// 对http请求报文的响应
int http_response(int web_socket, int http_response_mun);

// 定义web端套接字地址
sockaddr_in web_addr;

/** 接收http请求报文头部
 * @brief 接收http请求报文头部,将头部提交给调用的函数
 * @param[in] header http请求报文头部字符串指针的指针,主要是用于字符串的保留
 * @param[out] int 返回接收状态
 * */
int http_header_recv(int web_socket, char **header)
{
    // 定义函数返回结果
    int ret = -1;
    // 定义接收缓存区
    char recv_buffer[RECV_BUF_SIZE] = {0};
    // 定义头部请求字符串指针头部
    char *request_header;

    // 初始化头部请求字符串指针头部
    request_header = (char *)malloc(HTTP_HEADER_LEN * sizeof(char));
    memset(request_header, 0, HTTP_HEADER_LEN);

    // 定义接收到的数据长度
    int len = 0;
    // 定义总长度,用于记录个部分的总长度
    int total_lenght = 0;
    // 定义当前缓存变量储存大小
    int mem_size = 0;
    // 定义可能需要使用到的临时缓存区指针
    char *temp;

    // 接收数据前先对需要使用到的数据初始化
    total_lenght = 0;
    len = 0;
    mem_size = HTTP_HEADER_LEN;
    memset(recv_buffer, 0, RECV_BUF_SIZE);

    // 接收http报文请求头部
    while ((len = recv(web_socket, recv_buffer, RECV_BUF_SIZE, MSG_NOSIGNAL)) > 0)
    {
        // 表示有接收到数据
        if (total_lenght + len > mem_size)
        {
            // 当当前储存区的大小比请求头总长度小时
            // 对储存区进行扩大
            mem_size *= 2;
            temp = (char *)realloc(request_header, sizeof(char) * mem_size);
            if (temp == NULL)
            {
                // 当重新分配失败时,直接退出
                printf("REALLOC ERROR!\r\n");
                ret = -1;
                break;
            }
            request_header = temp;
            temp = NULL;
        }

        // 对接收到的数据与现有的进行拼接
        recv_buffer[len] = '\0';
        strcat(request_header, recv_buffer);

        // 判断请求头是否结束
        if (strcmp(&request_header[strlen(request_header) - 4], "\r\n\r\n"))
        {
            // 说明请求头部已经结束
            ret = 0;
            *header = request_header;
            return ret;
        }
        // 还没有结束继续
        total_lenght += len;
    }
    // 对使用到的堆进行释放
    free(request_header);
    return ret;
}

/** 接收http报文
 * @brief 对接收到的http报文进行解析,将文件请求提交给回调函数,但是只接收POST请求报文,其他类新的都返回400
 * @param[in] content 指向用来储存请求字符串的地址
 * */
int http_recv(int web_socket, char **content)
{
    // 定义函数返回值
    int ret = -1;
    // 定义http相应返回值  Bad Request
    int http_response_mun = 400;
    // 定义请求头部和请求内容字符串缓存指针
    char *request_header;
    char *request_content;
    // 定义解析http请求头部字段变量
    __http_request_title title;

    // 对title进行初始化
    title.Content_length = title.Content_Type = 0;
    memset(title.Host, 0, sizeof(title.Host));

    // 对http请求头部进行接收
    ret = http_header_recv(web_socket, &request_header);

    // 对http头部进行分析
    if (ret == 0)
    {
        // 表示前面http报文头部接收成功
        // 对接收的http报文请求头部进行分析
        http_response_mun = parse_http_request(request_header, &title);
    }
    else
    {
        // 表示没有接收到消息,直接返回
        return ret;
    }

    if (http_response_mun == OK)
    {
        // 当上面的头部接收和分析都没有问题,就进行对请求内容进行接收
        ret = http_content_recv(web_socket, &request_content, title);
    }
    else
    {
        // 表示http请求不符合要求,不对内容进行接收
    }

    // 对接收的状态进行判断
    if (ret == 0)
    {
        // 接收成功
        // 将得到的http请求内容地址传递回去
        *content = request_content;
    }
    else
    {
        // 表示接收http请求内容时出错了,作出相应的响应
        if(-1==http_response(web_socket,http_response_mun))
        {
            // 表示发送响应错误
            printf("HTTP RESPONSE ERROR!\r\n");
        }
    }

    return ret;
}

/** 获取ip地址
 * @brief 通过域名得到相应的ip地址
 * @param[in] domain 指向域名变量的指针
 * @param[in] ip_addr 指向IP地址字符串字符串的指针
 * */
int get_ip_addr(char *domain, char *ip_addr)
{
    // 通过专门的函数来获取相应的IP地址
    struct hostent *host = gethostbyname(domain);
    // 判断ip地址是否成功获得
    if (host == NULL)
    {
        // 获取失败
        memset((void *)ip_addr, 0, IP_ADDR_LEN);
        return -1;
    }
    // 其中在struct hostent 结构体中的h_addr_list数组中储存的就是相应
    // IP地址,一个网址可能有多个IP地址,这里只用第一个IP地址
    strcpy(ip_addr, inet_ntoa(*(struct in_addr *)host->h_addr));
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
    int ret = -1;
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
        // 通过用户自己设定的参数进行赋值
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

    // 完成连接
    puts("connect...");
    if (*net_arg_p->socket_point = connect(*(net_arg_p->socket_point), (sockaddr *)&web_addr, sizeof(web_addr)) == SOCKET_ERROR)
    {
        return -1;
    }

    return 0;
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
    // 定义请求字符串缓存变量,用于保存请求字符串
    char *request_data = NULL;
    // 定义http报文响应状态码
    int http_response_mun=400;

    // 通过网络参数进行连接
    if (-1 == client_connect(&thread_arg.database))
    {
        // 连接失败
        printf("CONNECT ERROR!\r\n");
        exit(1);
    }

    // 连接成功
    puts("CONNECT SUCCESSFUL!\r\n");

    // 轮循对web端接口进行接收数据,对数据内容进行解析,解析得到相应需要的内容之后提交给回调函数
    while (1)
    {
        // 对web端接口进行http报文接收
        if (0 == http_recv(*thread_arg.database.socket_point, &request_data))
        {
            // 表示http请求报文内容接收成功
            // 调用回调函数来对内容进行处理
            http_response_mun=thread_arg.callback(request_data, thread_arg.database);
            // 根据状态码进行http报文响应

        }
        else
        {
            // 说明发送过来的数据不符合要求或则没有接收到数据
            sleep(1);
        }
    }

    // 程序退出
    exit(0);
}