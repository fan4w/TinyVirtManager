#include "virDomain.h"

VirDomain::VirDomain(const std::string& xmlDesc, HypervisorDriver* driver, unsigned int flags) {
    // TODO: 解析XML描述，获取虚拟机的基本信息

}

int VirDomain::virDomainGetState(unsigned int& reason) const {
    // TODO: 调用驱动的接口获取虚拟机的状态
    return driver->domainGetState(std::make_shared<VirDomain>(*this));
}

std::string VirDomain::virDomainGetName() const {
    return name;
}

int VirDomain::virDomainGetID() const {
    return id;
}

std::string VirDomain::virDomainGetUUID() const {
    return uuid;
}