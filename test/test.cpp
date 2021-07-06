#include<stdio.h>
#include<time.h>

#define TIME_NOW (time(NULL))
void test(char*);

int main(void)
{
    time_t time1=TIME_NOW;
    printf("%ld\r\n",time1);
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