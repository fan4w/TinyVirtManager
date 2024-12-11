#ifndef QEMU_DRIVER_H
#define QEMU_DRIVER_H

#include "../driver-hypervisor.h"
#include <iostream>
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
    void createVM(const std::string& name, int memory, int vcpus) override;
    void startVM(const std::string& name) override;
    void stopVM(const std::string& name) override;
    std::string getInfo() const override;
};

#endif // QEMU_DRIVER_H
