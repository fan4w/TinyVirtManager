#include "qemu_conf.h"
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <stdexcept> 

QemuDriverConfig::QemuDriverConfig() {
    configDir = configManager->getValue("qemu.config_dir", "./temp/domains");
    qmpSocketDir = configManager->getValue("qemu.qmp_socket_dir", "./temp/unix_sockets");
    qemuEmulator = configManager->getValue("qemu.qemu_emulator", "/usr/bin/qemu-system-x86_64");
    
    if ( !access(configDir.c_str(), F_OK) ) {
        createDirectoryIfNotExists(configDir);
    }
    if ( !access(qmpSocketDir.c_str(), F_OK) ) {
        createDirectoryIfNotExists(qmpSocketDir);
    }

    return;
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
