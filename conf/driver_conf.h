#ifndef DRIVER_CONF_H
#define DRIVER_CONF_H

#include <string>
#include "./config_manager.h"

// const std::string configFilePath = "./temp/mylibvirt.conf";

class DriverConfig {
protected:
    std::string configDir;
    std::string configFilePath;
    ConfigManager* configManager;
    bool createDirectoryIfNotExists(const std::string& path) const;
public:
    DriverConfig();
    std::string getConfigDir() const;
    std::string getLogDir() const {
        return "./temp/logs";
    }
};

#endif