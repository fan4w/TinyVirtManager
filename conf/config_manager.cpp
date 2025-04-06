#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

static std::mutex configMtx;

ConfigManager* ConfigManager::Instance() {
    static ConfigManager instance;
    return &instance;
}

bool ConfigManager::init(const std::string& configPath) {
    configFilePath = configPath;
    return parseConfigFile();
}

bool ConfigManager::parseConfigFile() {
    std::ifstream file(configFilePath);
    if ( !file.is_open() ) {
        std::cerr << "Failed to open config file: " << configFilePath << std::endl;
        return false;
    }

    std::string line;
    while ( std::getline(file, line) ) {
        // 跳过空行
        if ( line.empty() ) {
            continue;
        }

        // 处理注释：移除行内注释部分
        size_t commentPos = line.find('#');
        if ( commentPos != std::string::npos ) {
            // 如果#在行首，跳过整行
            if ( commentPos == 0 ) {
                continue;
            }
            // 否则只取注释前的部分
            line = line.substr(0, commentPos);
        }

        // 如果处理完注释后行为空，则跳过
        if ( line.find_first_not_of(" \t") == std::string::npos ) {
            continue;
        }

        std::istringstream is_line(line);
        std::string key;
        if ( std::getline(is_line, key, '=') ) {
            std::string value;
            if ( std::getline(is_line, value) ) {
                // 去除首尾空格
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                configMap[key] = value;
            }
        }
    }
    return true;
}

std::string ConfigManager::getValue(const std::string& key, const std::string& defaultValue) const {
    auto it = configMap.find(key);
    if ( it != configMap.end() ) {
        return it->second;
    }
    return defaultValue;
}

int ConfigManager::getIntValue(const std::string& key, int defaultValue) const {
    auto it = configMap.find(key);
    if ( it != configMap.end() ) {
        try {
            return std::stoi(it->second);
        }
        catch ( ... ) {
            return defaultValue;
        }
    }
    return defaultValue;
}