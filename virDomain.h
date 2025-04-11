#ifndef VIRDOMAIN_H
#define VIRDOMAIN_H

#include <string>
#include <stdexcept>
#include <iostream>
#include "driver-hypervisor.h"

class VirDomain {
private:
    std::string name;
    int id;
    std::string uuid;
    HypervisorDriver* driver; // 对应的驱动指针，供操作虚拟机使用

public:
    // 仿照Libvirt中的定义，增加一些方法，请根据以下方法编写程序
    VirDomain(const std::string& xmlDesc, HypervisorDriver* driver, unsigned int flags = 0);
    VirDomain(const std::string& name, int id, const std::string& uuid) : name(name), id(id), uuid(uuid) {}
    VirDomain(const std::string& name, int id, const std::string& uuid, HypervisorDriver* driver) : name(name), id(id), uuid(uuid), driver(driver) {}

    void virDomainSetID(int id) {
        this->id = id;
    }
    // Accessors
    int virDomainGetState(unsigned int& reason) const;
    std::string virDomainGetName() const;
    int virDomainGetID() const;
    std::string virDomainGetUUID() const;
};

#endif // VIRDOMAIN_H
