# 分别定义 C 和 C++ 编译器
CC = gcc
CXX = g++

# 分离编译标志和链接标志
CFLAGS = -I ./NtyCo/core/
CXXFLAGS = -I ./NtyCo/core/
LDFLAGS = -L ./NtyCo/ -lntyco

# 源文件列表
SRCS = kv_mem.c kv_store_array.c kv_store.cpp ntyco_server.c

# 正确生成目标文件列表
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.cpp=.o)

APP = kv
SUBDIR = ./NtyCo/

all: $(SUBDIR) $(APP)

# 简化子目录构建
$(SUBDIR):
	$(MAKE) -C $@

# 使用 C++ 编译器链接（因为包含 C++ 代码）
$(APP): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# C 文件编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# C++ 文件编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 添加清理规则
clean:
	rm -f $(APP) $(OBJS)
	$(MAKE) -C $(SUBDIR) clean

.PHONY: all clean