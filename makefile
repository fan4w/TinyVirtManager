CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
LDFLAGS = 

TARGET = vir_manager

SRCS = main.cpp virConnect.cpp virDomain.cpp driver-hypervisor.cpp \
       qemu/qemu_driver.cpp qemu/qemu_conf.cpp qemu/qemu_monitor.cpp \
       tinyxml/tinyxml2.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 修改规则以正确处理子目录中的.cpp文件
%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf conf/*.o qemu/*.o monitor/*.o tinyxml/*.o

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run help
