// TODO: 这一部分应该完全使用 config manager 来管理配置文件
#ifndef DRIVER_CONF_H
#define DRIVER_CONF_H

#include <string>
#include "./config_manager.h"

// const std::string configFilePath = "./temp/mylibvirt.conf";

class DriverConfig {
protected:
    std::string configDir;          // 配置目录
    // std::string configFilePath;     // 配置文件路径
    ConfigManager* configManager;   // 配置管理器
    bool createDirectoryIfNotExists(const std::string& path) const;
public:
    DriverConfig();
    std::string getConfigDir() const;
    // std::string getLogDir() const {
    //     return "./temp/logs";
    // }
};

#endif