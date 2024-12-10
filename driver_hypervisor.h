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
    virtual void startVM(const std::string& name) = 0;
    virtual void stopVM(const std::string& name) = 0;
    virtual std::string getInfo() const = 0;
};

class DriverFactory {
public:
    static std::unique_ptr<HypervisorDriver> createDriver(const std::string& uri);
};


#endif // DRIVER_HYPERVISOR_H