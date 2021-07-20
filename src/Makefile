#Makefile
#通过嵌套Makefile实现程序的编译
#兼容.c和.cpp文件
#为每一个库源文件都生成相应的库
#使用静态库,以至于在运行的时候更方便

#定义变量
#定义CXX作为编译器
CXX = g++
#定义CXXFLAGES作为编译器优化选项参数
CXXFLAGES := -g -O0

#定义LINK作为连接器
LINK = g++
#定义LFLAGS作为连接器的优化选项参数
LFLAGS := -g

#定义安装命令变量
INSTALL_FILE := install -m 755 -p
#定义安装路径变量
INSTALL_PATH := ../build

#定义最终目标变量
OUT_PUT := server

#定义中间变量文件
OBJ := server.o multithread.o message_queue.o callback.o

#定义当前文件路径变量
TOP_DIR := $(PWD)

#定义生成的最终可执行文件的完整路径
OUT_PUT_DIR := $(TOP_DIR)/$(OUT_PUT)

#创建obj文件,用来储存中间件
$(shell mkdir obj)
#定义obj路径变量
OBJ_DIR := $(TOP_DIR)/obj

#定义源文件路径变量
BIN_DIR := $(TOP_DIR)/main

#定义各个库文件路径变量
LIB_PATH := $(TOP_DIR)/lib
LIB_MULTITHREAD_DIR := $(LIB_PATH)/multithread
LIB_MESSAGE_QUEUE_DIR := $(LIB_PATH)/message_queue
LIB_CALLBACK_DIR := $(LIB_PATH)/callback
LIB_SOCKET_DIR := $(LIB_PATH)/socket
#最终server所依赖的库
LINK_LIB := $(LIB_MULTITHREAD_DIR)/libmultithread.a \
			$(LIB_MESSAGE_QUEUE_DIR)/libmessage_queue.a \
			$(LIB_CALLBACK_DIR)/libcallback.a\
			$(LIB_SOCKET_DIR)/libsocket.a

#定义头文件路径,需要在前面加上"-I"
INCPATH := -I$(TOP_DIR)/include

#定义子Makefile文件路径,用于运行这些子Makefile
SUB_DIR := $(LIB_SOCKET_DIR)\
		   $(LIB_MULTITHREAD_DIR) \
		   $(LIB_MESSAGE_QUEUE_DIR) \
		   $(LIB_CALLBACK_DIR) \
		   $(BIN_DIR)


#传递给子Makefile中的文件
export CXX OBJ TOP_DIR OBJ_DIR BIN_DIR\
		LIB_CALLBACK_DIR LIB_MESSAGE_QUEUE_DIR \
		LIB_MULTITHREAD_DIR INCPATH\
		CXXFLAGES

#定义make伪目标
.PHONY:clean cleanall all install

#all目标
all:$(SUB_DIR) $(OUT_PUT)

#跳转到相应的子Makefile生成最终目标所需要的文件
$(SUB_DIR):ECHO
	-make -C $@

#进行打印操作
ECHO:
	@echo $(SUB_DIR)
	@echo begin complie

#最终目标生成模式规则,其中需要依赖obj中的server.o文件和lib中的
#libmultithread.a和libmessage_queue.a和libcallback.a静态库
#生成之后就对垃圾文件进行删除
$(OUT_PUT):$(OBJ_DIR)/server.o
	@echo building
	$(LINK) $(LFLAGS) $^ $(LINK_LIB) -o $@ -lpthread
	rm -rf $(OBJ_DIR)

#删除编译生成的垃圾文件,不包括最终文件和库文件
clean:
	find . -name "*.o" | xargs rm -f
	rm -rf $(OBJ_DIR)
	rm -f $(OUT_PUT)

#删除所有编译生成的文件
cleanall:
	find . -name "*.o" -o -name "*.a" | xargs rm -f
	rm -rf $(OBJ_DIR)
	rm -f $(OUT_PUT)
	rm -f $(INSTALL_PATH)/$(OUT_PUT)

#定义install目标,将最终文件安装到build文件中
install:
	$(INSTALL_FILE) $(OUT_PUT_DIR) $(INSTALL_PATH)