#ifndef QEMU_DRIVER_H
#define QEMU_DRIVER_H

#include "../driver-hypervisor.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unordered_map>


class QemuDriver : public HypervisorDriver {
private:
    std::unordered_map<std::string, bool> domains; // 模拟虚拟机的运行状态

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
