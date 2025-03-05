#ifndef QEMU_CONF_H
#define QEMU_CONF_H

#include <string>

const std::string configFilePath = "./temp/mylibvirt.conf";

class QemuDriverConfig {
private:
    std::string configDir;
    std::string qmpSocketDir;
    std::string qemuEmulator;
    bool createDirectoryIfNotExists(const std::string& path) const;
public:
    QemuDriverConfig();
    std::string getConfigDir() const;
    std::string getQmpSocketDir() const;
    std::string getQemuEmulator() const;
};

#endif