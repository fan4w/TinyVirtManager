#include "virConnect.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


VirConnect::VirConnect(const std::string& uri, unsigned int flags) : uri(uri) {
    // 根据URI创建对应的Driver
    driver = DriverFactory::createDriver(uri);
    std::vector<std::shared_ptr<VirDomain>> domains_ = driver->connectListAllDomains(flags);
    for ( const auto& domain : domains_ ) {
        domains.push_back(std::make_shared<VirDomain>(domain->virDomainGetName(), domain->virDomainGetID(), domain->virDomainGetUUID(), driver.get()));
    }
}


std::shared_ptr<VirDomain> VirConnect::virDomainDefineXML(const std::string& xmlDesc) {
    std::shared_ptr<VirDomain> domain = driver->domainDefineXML(xmlDesc);
    domains.push_back(domain);
    return domain;
}

void VirConnect::virDomainUndefine(const std::shared_ptr<VirDomain> domain) {
    // 删除虚拟机对象
    for ( auto it = domains.begin(); it != domains.end(); ++it ) {
        if ( *it == domain ) {
            domains.erase(it);
            break;
        }
    }

    // driver停止并删除虚拟机
    driver->domainUndefine(domain);
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByName(const std::string& name) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetName() == name ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with name " + name + " does not exist.");
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByID(const int& id) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetID() == id ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with ID " + std::to_string(id) + " does not exist.");
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByUUID(const std::string& uuid) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetUUID() == uuid ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with UUID " + uuid + " does not exist.");
}

std::vector<std::shared_ptr<VirDomain>> VirConnect::virConnectListAllDomains(unsigned int flags) const {
    if ( flags == 0 ) {
        return domains;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virConnectListAllDomains.");
    }
}

std::shared_ptr<VirDomain> VirConnect::virDomainCreateXML(const std::string& xmlDesc, unsigned int flags) {
    if ( flags == 0 ) {
        std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xmlDesc, driver.get());
        // 调用驱动的方法启动虚拟机
        driver->domainCreateXML(xmlDesc);
        domains.push_back(domain);
        return domain;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virDomainCreateXML.");
    }
}

void VirConnect::virDomainCreate(const std::shared_ptr<VirDomain> domain, unsigned int flags) {
    if ( flags == 0 ) {
        // 调用驱动的方法启动虚拟机
        driver->domainCreate(domain);
        return;
    }
}

void VirConnect::virDomainDestroy(const std::shared_ptr<VirDomain> domain) {
    // TODO: 调用驱动的方法关闭虚拟机
    driver->domainDestroy(domain);
    return;
}