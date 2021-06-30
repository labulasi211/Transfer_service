#Makefile
#子Makefile,用于生成libsocket库文件的最终文件
#并会生成相应的中间过渡文件(*.o)

#找到该目录下的.c和.cpp文件
SRC := $(wildcard *.c)
SRC += $(wildcard *.cpp)

#将.c和.cpp文件的后缀名改为.o,并用OBJ表示
OBJ := $(patsubst %.c,%.o,$(SRC))
OBJ := $(patsubst %.cpp,%.o,$(SRC))

#得到一个以文件夹名的静态库文件
CURRENT_DIR := $(shell pwd)
PARENT_DIRNAME := $(word $(words $(subst /, ,$(CURRENT_DIR))),$(subst /, ,$(CURRENT_DIR)))
TARGET := lib$(PARENT_DIRNAME).a

#得到的中间过渡文件的完整路径用TAR_OBJ表示
TAR_OBJ := $(patsubst %,$(OBJ_DIR)/%,$(OBJ))

#定义生成静态库的模式规则
$(TARGET):$(TAR_OBJ)
	$(AR) -r $@ $^

#定义生成中间过渡文件的模式规则
#这个需要将头文件包含进来
#只能实现cpp文件
$(foreach i,$(TAR_OBJ),$(i)):$(OBJ_DIR)/%.o:%.cpp
	-$(CXX) $(CXXFLAGES) -c $^ $(INCPATH) -o $@
