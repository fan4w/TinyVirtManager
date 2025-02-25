#include "qemu_conf.h"
#include <fstream>
#include <unistd.h>

QemuDriverConfig::QemuDriverConfig() : configDir("./temp/domains"), qmpSocketDir("./temp/unix_sockets") {
    if ( access(configFilePath.c_str(), F_OK) == 0 ) {
        return;
    }
    // TODO: 处理配置文件存在的情况，需要解析配置文件
}