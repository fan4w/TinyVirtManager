#ifndef DRIVER_HYPERVISOR_H
#define DRIVER_HYPERVISOR_H

#include <string>
#include <memory>
#include <stdexcept>
// #include "qemu/qemu_driver.h"

class HypervisorDriver {
public:
    virtual ~HypervisorDriver() = default;

    // 定义通用接口
    // 这里用memory和vcpus模拟实际会传递的参数
    virtual void createVM(const std::string& name, int memory, int vcpus) = 0;
    virtual void startVM(const std::string& name) = 0;
    virtual void stopVM(const std::string& name) = 0;
    virtual std::string getInfo() const = 0;

    // 以下为仿照Libvirt中driver-hypervisor的定义的接口，有些接口没有必要实现
};

class DriverFactory {
public:
    static std::unique_ptr<HypervisorDriver> createDriver(const std::string& uri);
};


#endif // DRIVER_HYPERVISOR_H