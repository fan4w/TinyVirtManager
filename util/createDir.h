#ifndef CREATE_DIR_H
#define CREATE_DIR_H

#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdexcept>
#include "../log/log.h"

inline bool createDirectoryIfNotExists(const std::string& path) {
    // 检查目录是否存在
    struct stat st;
    if ( stat(path.c_str(), &st) == 0 ) {
        if ( S_ISDIR(st.st_mode) ) {
            // 目录已存在，无需创建
            return true;
        }
        else {
            // 路径存在但不是目录
            throw std::runtime_error("Path exists but is not a directory: " + path);
        }
    }

    // 递归创建目录，权限设置为 0755 (rwxr-xr-x)
    // 创建嵌套目录需要逐级创建
    size_t pos = 0;
    std::string current_path;

    // 处理相对路径和绝对路径
    if ( path[0] == '/' ) {
        current_path = "/";
        pos = 1;
    }

    while ( (pos = path.find('/', pos)) != std::string::npos ) {
        current_path = path.substr(0, pos);
        if ( !current_path.empty() && stat(current_path.c_str(), &st) != 0 ) {
            if ( mkdir(current_path.c_str(), 0755) != 0 && errno != EEXIST ) {
                throw std::runtime_error("Failed to create directory: " + current_path + ", error: " + strerror(errno));
            }
        }
        pos++;
    }

    // 创建最终目录
    if ( mkdir(path.c_str(), 0755) != 0 && errno != EEXIST ) {
        throw std::runtime_error("Failed to create directory: " + path + ", error: " + strerror(errno));
    }

    // std::cout << "Created directory: " << path << std::endl;
    LOG_INFO("Created directory: %s", path.c_str());
    return true;
}

#endif // CREATE_DIR_H