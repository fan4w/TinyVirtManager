#include "virConnect.h"

// 构造函数
VirConnect::VirConnect(const std::string& uri) : uri(uri) {
    // 通过URI创建对应类型的Driver
    driver = DriverFactory::createDriver(uri);
}

// 创建虚拟机
std::shared_ptr<VirDomain> VirConnect::createDomain(const std::string& name, int memory, int vcpus) {
    if (domains.find(name) != domains.end()) {
        throw std::runtime_error("Domain with name " + name + " already exists.");
    }
    auto domain = std::make_shared<VirDomain>(name, memory, vcpus, driver.get());
    domains[name] = domain;
    return domain;
}

// 删除虚拟机
void VirConnect::deleteDomain(const std::string& name) {
    auto it = domains.find(name);
    if (it == domains.end()) {
        throw std::runtime_error("Domain with name " + name + " does not exist.");
    }
    domains.erase(it);
}

// 获取虚拟机
std::shared_ptr<VirDomain> VirConnect::getDomain(const std::string& name) const {
    auto it = domains.find(name);
    if (it == domains.end()) {
        throw std::runtime_error("Domain with name " + name + " does not exist.");
    }
    return it->second;
}

// 显示所有虚拟机
void VirConnect::showAllDomain() const {
    std::cout << "All domains:" << std::endl;
    for (const auto& pair : domains) {
        std::cout << pair.first << std::endl;
    }
}   

// 调用驱动的方法
void VirConnect::startVM(const std::string& name) {
    auto domain = getDomain(name);
    domain->start();
}

void VirConnect::stopVM(const std::string& name) {
    auto domain = getDomain(name);
    domain->shutdown();
}

std::string VirConnect::getInfo() const {
    return driver->getInfo();
}
