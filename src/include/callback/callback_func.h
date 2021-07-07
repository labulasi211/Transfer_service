#ifndef _CALLBACK_FUNC_H
#define _CALLBACK_FUNC_H

#include <multithread/service_thread.h>
#include<transfer_service/transfer_service.h>

// 面向web端的回调函数
int web_callback_func(void* str,__data_arg arg);
// 面向android端的回调函数
int android_callback_func(void* str,__data_arg arg);

#endif