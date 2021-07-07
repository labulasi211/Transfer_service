#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define FIND_MUSIC "../music/%d_%s_%s.mp3\r\n"
#define TIME_NOW (time(NULL))

int main(void)
{
    const char *delim = "?";
    char buffer[200] = {0};
    char *p = NULL;
    printf(FIND_MUSIC, 1,"1","1");
    return 0;
}

// const char* p;
// p is a pointer to const char.
// char const *p;
// p is a pointer to char const.
// char* const p;
// p is a const pointer to char.

// 没有const*怎么一说
// 变量是从右往左修饰