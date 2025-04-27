#include "virConnect.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


VirConnect::VirConnect(const std::string& uri, unsigned int flags) : uri(uri) {
    // 从配置文件初始化日志系统
    Log::Instance()->initFromConfig("./myLibvirt.conf");

    LOG_INFO("Initializing VirConnect with URI: %s", uri.c_str());

    // 根据URI创建对应的Driver
    driver = DriverFactory::createDriver(uri);
    storageDriver = StorageDriverFactory::createStorageDriver("filesystem");
    networkDriver = std::unique_ptr<NetworkDriver>(new NetworkDriver());
    std::vector<std::shared_ptr<VirDomain>> domains_ = driver->connectListAllDomains(flags);
    for ( const auto& domain : domains_ ) {
        domains.push_back(std::make_shared<VirDomain>(domain->virDomainGetName(), domain->virDomainGetID(), domain->virDomainGetUUID(), driver.get()));
    }
    std::vector<std::shared_ptr<VirStoragePool>> storagePools_ = storageDriver->connectListStoragePools(flags);
    for ( const auto& pool : storagePools_ ) {
        storagePools.push_back(std::make_shared<VirStoragePool>(pool->virStoragePoolGetName(), pool->virStoragePoolGetUUID(), storageDriver.get()));
    }
    std::vector<std::shared_ptr<VirNetwork>> networks_ = networkDriver->connectListAllNetworks(flags);
    for ( const auto& network : networks_ ) {
        networks.push_back(std::make_shared<VirNetwork>(network->virNetworkGetName(), network->virNetworkGetUUID(), networkDriver.get()));
    }

    LOG_INFO("VirConnect initialized successfully, found %zu domains", domains.size());
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
    // 调用驱动的方法关闭虚拟机
    driver->domainDestroy(domain);
    return;
}

void VirConnect::virDomainShutdown(const std::shared_ptr<VirDomain> domain) {
    // 调用驱动的方法关闭虚拟机
    driver->domainShutdown(domain);
    return;
}

std::shared_ptr<VirStoragePool> VirConnect::virStoragePoolDefineXML(const std::string& xmlDesc, unsigned int flags) {
    std::shared_ptr<VirStoragePool> pool = storageDriver->storagePoolDefine(xmlDesc, flags);
    storagePools.push_back(pool);
    return pool;
}

void VirConnect::virStoragePoolUndefine(const std::shared_ptr<VirStoragePool> pool) {
    // 删除存储池对象
    for ( auto it = storagePools.begin(); it != storagePools.end(); ++it ) {
        if ( *it == pool ) {
            storagePools.erase(it);
            break;
        }
    }

    // driver停止并删除存储池
    storageDriver->storagePoolUndefine(pool);
}

std::shared_ptr<VirStoragePool> VirConnect::virStoragePoolCreateXML(const std::string& xmlDesc, unsigned int flags) {
    std::shared_ptr<VirStoragePool> pool = storageDriver->storagePoolCreateXML(xmlDesc, flags);
    storagePools.push_back(pool);
    return pool;
}

void VirConnect::virStoragePoolCreate(const std::shared_ptr<VirStoragePool> pool, unsigned int flags) {
    storageDriver->storagePoolCreate(pool, flags);
    return;
}

std::shared_ptr<VirStoragePool> VirConnect::virStoragePoolLookupByName(const std::string& name) const {
    for ( const auto& pool : storagePools ) {
        if ( pool->virStoragePoolGetName() == name ) {
            return pool;
        }
    }
    throw std::runtime_error("Storage pool with name " + name + " does not exist.");
}

std::shared_ptr<VirStoragePool> VirConnect::virStoragePoolLookupByUUID(const std::string& UUID) const {
    for ( const auto& pool : storagePools ) {
        if ( pool->virStoragePoolGetUUID() == UUID ) {
            return pool;
        }
    }
    throw std::runtime_error("Storage pool with UUID " + UUID + " does not exist.");
}

std::vector<std::shared_ptr<VirStoragePool>> VirConnect::virConnectListAllStoragePools(unsigned int flags) const {
    if ( flags == 0 ) {
        return storagePools;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virConnectListAllStoragePools.");
    }
}

void VirConnect::virStoragePoolDestroy(const std::shared_ptr<VirStoragePool> pool) {
    // 调用驱动的方法关闭存储池
    storageDriver->storagePoolDestroy(pool);
    return;
}

std::shared_ptr<VirStorageVol> VirConnect::virStorageVolCreateXML(const std::shared_ptr<VirStoragePool> pool, const std::string& xmlDesc, unsigned int flags) {
    std::shared_ptr<VirStorageVol> vol = storageDriver->storageVolCreateXML(pool, xmlDesc, flags);
    return vol;
}
std::shared_ptr<VirStorageVol> VirConnect::virStorageVolCreateXMLFrom(const std::shared_ptr<VirStoragePool> pool, const std::string& xmlDesc, const std::shared_ptr<VirStorageVol> srcVol, unsigned int flags) {
    std::shared_ptr<VirStorageVol> vol = storageDriver->storageVolCreateXMLFrom(pool, xmlDesc, srcVol, flags);
    return vol;
}

std::shared_ptr<VirStorageVol> VirConnect::virStorageVolLookupByName(const std::shared_ptr<VirStoragePool> pool, const std::string& name) const {
    return storageDriver->storageVolLookupByName(pool, name);
}
std::shared_ptr<VirStorageVol> VirConnect::virStorageVolLookupByPath(const std::string& path) const {
    return storageDriver->storageVolLookupByPath(path);
}

int VirConnect::virStorageVolDelete(const std::shared_ptr<VirStorageVol> vol, unsigned int flags) {
    return storageDriver->storageVolDelete(vol, flags);
}

std::vector<std::shared_ptr<VirNetwork>> VirConnect::virConnectListAllNetworks(unsigned int flags) const {
    if ( flags == 0 ) {
        return networks;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virConnectListAllNetworks.");
    }
}

std::shared_ptr<VirNetwork> VirConnect::virNetworkDefineXML(const std::string& xmlDesc, unsigned int flags) {
    std::shared_ptr<VirNetwork> network = networkDriver->networkDefineXML(xmlDesc, flags);
    networks.push_back(network);
    return network;
}

std::shared_ptr<VirNetwork> VirConnect::virNetworkLookupByName(const std::string& name) const {
    for ( const auto& network : networks ) {
        if ( network->virNetworkGetName() == name ) {
            return network;
        }
    }
    throw std::runtime_error("Network with name " + name + " does not exist.");
}

std::shared_ptr<VirNetwork> VirConnect::virNetworkLookupByUUID(const std::string& uuid) const {
    for ( const auto& network : networks ) {
        if ( network->virNetworkGetUUID() == uuid ) {
            return network;
        }
    }
    throw std::runtime_error("Network with UUID " + uuid + " does not exist.");
}
