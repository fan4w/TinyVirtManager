#ifndef DRIVER_HYPERVISOR_H
#define DRIVER_HYPERVISOR_H

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
// #include "qemu/qemu_driver.h"

class VirDomain;

class HypervisorDriver {
public:
    virtual ~HypervisorDriver() = default;

    // 以下为仿照Libvirt中driver-hypervisor的定义的接口，有些接口没有必要实现
    // 遵循和Libvirt/driver-hypervisor.h中virHypervisorDriver的命名规则，方便理解
    virtual std::vector<std::shared_ptr<VirDomain>> connectListAllDomains(unsigned int flags = 0) const = 0;

    // 查找虚拟机对象
    virtual std::shared_ptr<VirDomain> domainLookupByName(const std::string& name) const = 0;
    // virtual std::shared_ptr<VirDomain> domainLookupByID(const int& id) const = 0;
    // virtual std::shared_ptr<VirDomain> domainLookupByUUID(const std::string& uuid) const = 0;

    // define会创建一个虚拟机对象，但不会启动虚拟机
    virtual std::shared_ptr<VirDomain> domainDefineXML(const std::string& xml) = 0;
    virtual std::shared_ptr<VirDomain> domainDefineXMLFlags(const std::string& xml, unsigned int flags) = 0;

    // create会启动一个已经创建好的虚拟机
    virtual void domainCreate(std::shared_ptr<VirDomain> domain) = 0;
    virtual std::shared_ptr<VirDomain> domainCreateXML(const std::string& xml) = 0;

    // destroy会停止一个虚拟机
    virtual void domainDestroy(std::shared_ptr<VirDomain> domain) = 0;

    // undefine会删除一个虚拟机对象
    virtual int domainUndefine(std::shared_ptr<VirDomain> domain) = 0;
    virtual int domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) = 0;

    virtual int domainGetState(std::shared_ptr<VirDomain> domain) = 0;
};

class DriverFactory {
public:
    static std::unique_ptr<HypervisorDriver> createDriver(const std::string& uri);
};


#endif // DRIVER_HYPERVISOR_H