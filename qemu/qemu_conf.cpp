#include "qemu_conf.h"
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <stdexcept> 

QemuDriverConfig::QemuDriverConfig() : configDir("./temp/domains"), qmpSocketDir("./temp/unix_sockets"), qemuEmulator("/usr/bin/qemu-system-x86_64") {
    if ( access(configFilePath.c_str(), F_OK) == 0 ) {
        return;
    }

    // 如果对应路径不存在，则创建
    createDirectoryIfNotExists(configDir);
    createDirectoryIfNotExists(qmpSocketDir);
}

std::string QemuDriverConfig::getConfigDir() const {
    return configDir;
}

std::string QemuDriverConfig::getQmpSocketDir() const {
    return qmpSocketDir;
}

std::string QemuDriverConfig::getQemuEmulator() const {
    return qemuEmulator;
}
