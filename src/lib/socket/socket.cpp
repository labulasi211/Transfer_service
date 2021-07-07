/**\socket.cpp
 * 简要介绍:关于socket的一些函数
 * 作者:曾晨
 * 完成度:未完成
 * */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <socket/socket.h>

// 定义http响应代码字符串数组
char RESPONSE_STATUS_CODE[HTTP_RESPONSE_CODE_MUN][HTTP_RESPONSE_STRING_LEN] = {0};

/** 对http响应字符串初始化
 * @brief 对http响应字符串初始化
 * @param[out] int 返回接收状态
 * */
int http_response_status_string_init(void)
{
    // 对全局变量数组赋值
    strcpy(RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(OK)], "OK");
    strcpy(RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(BAD_REQUEST)], "Bad Request");
    strcpy(RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(METHOD_NOT_ALLOWED)], "Method Not Allowed");
    strcpy(RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(REQUESR_ENTITY_TOO_LARGE)], "Request Entity Too Large");
    strcpy(RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(NOT_FOUND)], "Not Found");
    return 0;
}
/** 对http请求进行响应
 * @brief 根据输入的参数中的响应代码发送响应的数据
 * @param[in] web_socket 面向web端的套接字
 * @param[in] http_response_mun 响应代码
 * @param[out] int 返回接收状态
 * */
int http_response(int web_socket, int http_response_mun)
{
    // 定义http报文响应首字段长度
    char response[HTTP_RESPONSE_STSRT_LINE_LEN] = {0};
    // 定义发送长度
    int len = 0;

    //得到需要发送的字符串
    sprintf(response, "HTTP/1.1 %d %s", http_response_mun, RESPONSE_STATUS_CODE[GET_HTTP_RESPONSE_STRING(http_response_mun)]);

    // 发送
    len = send(web_socket, response, sizeof(response), MSG_DONTWAIT);
    // 判断是否发送成功
    if (len == SOCKET_ERROR)
    {
        // 发送失败
        return -1;
    }
    return 0;
}

/** 对http请求报文内容的接收
 * @brief 根据求情头中的要求对http内容进行接收,这里之允许application/x-www-form-urlencoded方式
 * @param[in] web_socket 面向web端的套接字
 * @param[in] content 用来保存接收内容的指向字符串指针的指针
 * @param[out] int 返回接收状态
 * */
int http_content_recv(int web_socket, char **content, __http_request_header title)
{
    // http内容的接收
    // 定义函数返回值
    int ret = BAD_REQUEST;
    // 定义接收数据长度
    int total_length;
    // 定义本次接收的数据长度
    int len;
    // 定义接收缓存区指针
    char recv_buffer[RECV_BUF_SIZE] = {0};

    // 定义接收字符串指针储存区
    char *request_content = NULL;
    request_content = (char *)malloc(sizeof(char) * HTTP_CONTENT_LEN);
    memset(request_content, 0, HTTP_CONTENT_LEN);
    // 定义接收字符串指针储存区的大小
    int memsize;
    // 定义临时字符串变量
    char *temp;

    // 对需要使用到的数据初始化
    total_length = 0;
    len = 0;
    memsize = HTTP_CONTENT_LEN;
    memset(recv_buffer, 0, RECV_BUF_SIZE);
    temp = NULL;

    while ((len = recv(web_socket, (void *)recv_buffer, RECV_BUF_SIZE, MSG_DONTWAIT)) > 0)
    {
        // 表示有接收到数据
        ret = OK;
        // 接收到数据
        if (total_length + len > memsize)
        {
            // 扩大接收数据储存区大小
            memsize += HTTP_CONTENT_LEN;
            temp = (char *)realloc(request_content, memsize * sizeof(char));
            if (temp == NULL)
            {
                // 表示当前内存不够
                printf("REALLOC ERROR!\r'n");
                ret = BAD_REQUEST;
                break;
            }
            request_content = temp;
            temp = NULL;
        }
        // 对接收数据与原有的数据进行拼接
        recv_buffer[len] = '\0';
        strcat(request_content, recv_buffer);
    }

    // 对接收的数据长度与请求头部中的数据长度进行判断
    if (total_length == title.Content_length)
    {
        // 表示接收成功
        *content = request_content;
        ret = OK;
        return ret;
    }
    else if (ret == OK)
    {
        // 表示请求头中发生轮错误
        ret = BAD_REQUEST;
        printf("RECV CONTENT ERROR!\r\n");
    }
    // 表示接收时发生了错误
    free(request_content);
    ret = BAD_REQUEST;
    return ret;
}

/** 查找字符串函数(不区分大小写)
 * @brief 找出str2字符串在str1字符串中第一次出现的位置(不区分大小写)
 * @param[in] str1 需要在该字符串上查找的字符串
 * @param[in] str2 需要查找的字符串
 * @param[out] char* 返回str2在str1上的位置
 * */
const char *strstri(const char *str1, const char *str2)
{
    // 主要是通过不区分大小的字符串比较strncasecmp来实现的
    int len = strlen(str2);
    const char *str = NULL;

    if (len == 0)
    {
        // 当str2为空字符串时
        return NULL;
    }

    // 遍历str1字符串查找str2字符串
    str = str1;
    while (*str == '\0')
    {
        if (0 == strncasecmp(str, str2, len))
        {
            // 表示查找到第一次出现str2的位置
            return str;
        }
        ++str;
    }

    // 表示没有找到响应的str2字符串
    return NULL;
}

/** 对http请求报文头部进行解析,
 * @brief 对http请求包围进行解析,得到需要用到的请求头部信息,以及对请求的的内容进行判断是否符合自己的要求,这里除POST请求外,其他不兼容
 * @param[in] header http请求报文头部字符串指针的指针,主要是用于字符串的保留
 * @param[in] title 用于保存请求头部上的部分有用的信息,用于对内容的接收部分
 * @param[out] int 返回响应状态码
 * */
int parse_http_request(const char *header, __http_request_header *title)
{
    // 对头部进行解析,将可能用的到的数据进行保存
    // 定义字符串指针,用于对http报文的读取
    const char *pos = NULL;

    // 对http请求方法进行读取
    pos = header;
    // sscanf可以以\r\n为结束标志符
    // 这里是区分大小写的
    if (3 != sscanf(pos, "%s %s HTTP/%f", title->HTTP_method, title->url, &title->HTTP_version))
    {
        // 表示出现了出乎意料的错误
        printf("HTTP START LINE ERROR!\r\n");
        return BAD_REQUEST;
    }

    // 表示三个参数都成功赋值
    if (0 != strcmp(title->HTTP_method, "POST"))
    {
        // 表示请求方法不对
        return METHOD_NOT_ALLOWED;
    }

    // 解析host
    pos = strstri(header, "host:");
    if (pos != NULL && 1 != sscanf(pos, "%*s %s", title->Host))
    {
        // 表示赋值的时候出现轮错误
        printf("HOST ERROR!\r\n");
    }

    // 解析Content-Type
    pos = strstri(header, "content-type:");
    if (pos != NULL && 1 != sscanf(pos, "%*s %s", title->Content_Type))
    {
        printf("CONTENT-TYPE ERROR!\r\n");
    }

    // 解析Content-length
    pos = strstri(header, "content-length:");
    if (pos != NULL && 1 != sscanf(pos, "%*s %ld", &title->Content_length))
    {
        printf("CONTENT-LENGTH ERROR!\r\n");
    }
    return OK;
}

/** 接收http请求报文头部
 * @brief 接收http请求报文头部,将头部提交给调用的函数
 * @param[in] header http请求报文头部字符串指针的指针,主要是用于字符串的保留
 * @param[out] int 返回接收状态
 * */
int http_header_recv(int web_socket, char **header)
{
    // 定义函数返回结果
    int ret = -2;
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

    // 接收http报文请求头部,一次只接受一个字符串,防止将请求内容也接收
    while ((len = recv(web_socket, recv_buffer, 1, MSG_DONTWAIT)) > 0)
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
                ret = BAD_REQUEST;
                break;
            }
            request_header = temp;
            temp = NULL;
        }

        // 对接收到的数据与现有的进行拼接
        recv_buffer[len] = '\0';
        strcat(request_header, recv_buffer);

        // 判断请求头是否结束
        if (0 == strcmp(&request_header[strlen(request_header) - 4], "\r\n\r\n"))
        {
            // 说明请求头部已经结束
            ret = OK;
            *header = request_header;
            printf("--%s--", *header);
            fflush(stdout);
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
 * @param[out] int http报文返回值
 * */
int http_recv(int web_socket, char **content)
{
    // 定义函数返回值
    // 以-2为初始值主要用于判断错误是否是无数据传输还是接收时出错了
    // 返回状态响应码
    int ret = -2;
    // 定义http相应返回值  Bad Request
    int http_response_mun = BAD_REQUEST;
    // 定义请求头部和请求内容字符串缓存指针
    char *request_header;
    char *request_content;
    // 定义解析http请求头部字段变量
    __http_request_header title;

    // 对title进行初始化
    memset(&title, 0, sizeof(title));

    // 对http请求头部进行接收
    ret = http_header_recv(web_socket, &request_header);

    // 对http头部进行分析
    if (ret == OK)
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
        // 对结果返回值进行赋值
        // 表示主函数不需要在做什么
        ret = http_response_mun;
    }

    // 对接收的状态进行判断
    if (ret == OK)
    {
        // 接收成功
        // 将得到的http请求内容地址传递回去
        *content = request_content;
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
    // 先判断当前url是否是有值的
    if(0==strlen(url))
    {
        // 直接返回ERROR
        return ERROR_APPEAR;
    }
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
    const char *patterns[] = {"http://", "https://", NULL};

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
int addr_init(sockaddr_in *addr_p, __socket_arg *net_arg_p)
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
int client_connect(__socket_arg *net_arg_p)
{
    // 创建套接字
    // 定义web端套接字地址
    sockaddr_in web_addr;
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
    if (SOCKET_ERROR == connect(service_socket, (sockaddr *)&web_addr, sizeof(web_addr)))
    {
        return -1;
    }
    *net_arg_p->socket_point = service_socket;
    return 0;
}
