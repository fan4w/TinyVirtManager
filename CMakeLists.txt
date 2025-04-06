cmake_minimum_required(VERSION 3.10)
project(myLibvirt VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 找到所有需要编译的源文件
file(GLOB VIRSH_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/myVirsh.cpp"
)

# 找到需要链接的库文件源码
file(GLOB LIB_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/virConnect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/virDomain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/driver-hypervisor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/qemu/qemu_conf.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/qemu/qemu_driver.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/qemu/qemu_monitor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/conf/config_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/conf/driver_conf.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/log/log.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/log/buffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyxml/tinyxml2.cpp"
)

# 将库源码编译为静态库
add_library(virlib STATIC ${LIB_SOURCES})

# 添加可执行文件并链接到静态库
add_executable(myVirsh ${VIRSH_SOURCES})
target_link_libraries(myVirsh PRIVATE virlib)

# 添加编译选项
target_compile_options(myVirsh PRIVATE -Wall -Wextra)
target_compile_options(virlib PRIVATE -Wall -Wextra)

# 安装目标
install(TARGETS myVirsh DESTINATION bin)