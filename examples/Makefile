# 定义编译器和编译选项
CXX := g++
CXXFLAGS := -std=c++98 -Wall -Wextra -g
LDFLAGS := 

# 定义源文件和目标文件
SRC_DIR := ..
MONITOR_SRC := $(SRC_DIR)/monitor/monitor.cpp
MONITOR_HDR := $(SRC_DIR)/monitor/monitor.h
EXAMPLES_SRC := $(SRC_DIR)/examples/monitor_example.cpp

# 定义目标文件
TEST_EXEC := unix_socket_test

# 默认目标
all: $(TEST_EXEC)

# 编译测试可执行文件
$(TEST_EXEC): $(MONITOR_SRC) $(EXAMPLES_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(MONITOR_SRC) $(EXAMPLES_SRC) $(LDFLAGS)

# 清理目标文件
clean:
	rm -f $(TEST_EXEC)

# 运行测试
run: $(TEST_EXEC)
	./$(TEST_EXEC)

.PHONY: all clean run
