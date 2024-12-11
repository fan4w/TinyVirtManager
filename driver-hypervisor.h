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

    // 定义通用接口
    // 这里用memory和vcpus模拟实际会传递的参数
    virtual void createVM(const std::string& name, int memory, int vcpus) = 0;
    virtual void startVM(const std::string& name) = 0;
    virtual void stopVM(const std::string& name) = 0;
    virtual std::string getInfo() const = 0;

    // 以下为仿照Libvirt中driver-hypervisor的定义的接口，有些接口没有必要实现
    // 遵循和Libvirt/driver-hypervisor.h中virHypervisorDriver的命名规则，方便理解
    // virtual void domainLookupByID(int id) = 0;
    // virtual void domainLookupByUUID(const std::string& uuid) = 0;
    virtual std::shared_ptr<VirDomain> domainLookupByName(const std::string& name) = 0;
    // define会创建一个虚拟机但并不启动它
    virtual void domainDefineXML(const std::string& xml) = 0;
    // virtual void domainDefineXMLFlags(const std::string& xml, unsigned int flags) = 0;
    
    // create会启动一个已经创建好的虚拟机
    virtual void domainCreate(std::shared_ptr<VirDomain> domain) = 0;
};

class DriverFactory {
public:
    static std::unique_ptr<HypervisorDriver> createDriver(const std::string& uri);
};


#endif // DRIVER_HYPERVISOR_H