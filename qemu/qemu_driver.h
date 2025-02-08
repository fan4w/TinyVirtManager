#ifndef QEMU_DRIVER_H
#define QEMU_DRIVER_H

#include "../driver-hypervisor.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>


class QemuDriver : public HypervisorDriver {
private:
    std::unordered_map<std::string, std::string> domainSockets; // 存储虚拟机的socket
public:
    // 构造函数与析构函数
    QemuDriver();
    ~QemuDriver();

    // 实现虚拟机操作
    void domainCreate(std::shared_ptr<VirDomain> domain) override;
    void domainCreateXML(const std::string& xmlDesc) override;

    void domainDestroy(std::shared_ptr<VirDomain> domain) override;

    int domainGetState(std::shared_ptr<VirDomain> domain) override;
};

#endif // QEMU_DRIVER_H
