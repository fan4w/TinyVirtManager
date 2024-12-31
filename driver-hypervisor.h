#ifndef DRIVER_HYPERVISOR_H
#define DRIVER_HYPERVISOR_H

#include <string>
#include <memory>
#include <stdexcept>
// #include "qemu/qemu_driver.h"

class VirDomain;

class HypervisorDriver {
public:
    virtual ~HypervisorDriver() = default;

    // 以下为仿照Libvirt中driver-hypervisor的定义的接口，有些接口没有必要实现
    // 遵循和Libvirt/driver-hypervisor.h中virHypervisorDriver的命名规则，方便理解

    // create会启动一个已经创建好的虚拟机
    virtual void domainCreate(std::shared_ptr<VirDomain> domain) = 0;
    virtual void domainCreateXML(const std::string& xml) = 0;

    // destroy会停止一个虚拟机
    virtual void domainDestroy(std::shared_ptr<VirDomain> domain) = 0;

    virtual int domainGetState(std::shared_ptr<VirDomain> domain) = 0;
};

class DriverFactory {
public:
    static std::unique_ptr<HypervisorDriver> createDriver(const std::string& uri);
};


#endif // DRIVER_HYPERVISOR_H