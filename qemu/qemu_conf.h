#ifndef QEMU_CONF_H
#define QEMU_CONF_H

#include <string>

const std::string configFilePath = "./temp/mylibvirt.conf";

class QemuDriverConfig {
private:
    std::string configDir;
    std::string qmpSocketDir;
public:
    QemuDriverConfig();
    std::string getConfigDir() const {
        return configDir;
    }
};

#endif