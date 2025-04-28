#ifndef QEMU_CONF_H
#define QEMU_CONF_H

#include <string>
#include "../conf/driver_conf.h"

class QemuDriverConfig : public DriverConfig {
private:
    std::string configDir;
    std::string qmpSocketDir;
    std::string qemuEmulator;
    bool openGraphics;
    // bool createDirectoryIfNotExists(const std::string& path) const;
public:
    QemuDriverConfig();
    std::string getConfigDir() const;
    std::string getQmpSocketDir() const;
    std::string getQemuEmulator() const;
    std::string getLogDir() const {
        return "./temp/log";
    }
    bool isOpenGraphics() const {
        return openGraphics;
    }
};

#endif