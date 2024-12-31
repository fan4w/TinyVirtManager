CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
LDFLAGS = 

TARGET = vir_manager

SRCS = main.cpp virConnect.cpp virDomain.cpp qemu/qemu_driver.cpp driver-hypervisor.cpp tinyxml/tinyxml2.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run help
