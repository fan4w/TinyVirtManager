#ifndef QEMU_DOMAIN_H
#define QEMU_DOMAIN_H

#include "../conf/domain_conf.h"
#include "qemu_monitor.h"
#include <string>
#include <memory>

// QEMU特定的域定义
class qemuDomainDef : public virDomainDef {
public:
    // QEMU特有配置
    std::string qmpSocketPath;    // QMP套接字路径
    std::string monitorSocketPath; // 监控套接字路径
    bool enableKVM;               // 是否启用KVM
    
    // 其他QEMU特定配置
};

// QEMU特定的域对象
class qemuDomainObj : public virDomainObj {
public:
    // QEMU特有运行时数据
    std::shared_ptr<QemuMonitor> monitor;  // QMP监控对象
    
    // 构造函数
    qemuDomainObj() {
        // 初始化QEMU特有资源，例如QMP socket
    }
    
    // 析构函数
    ~qemuDomainObj() override {
        // 清理QEMU特有资源
    }
    
};

#endif // QEMU_DOMAIN_H