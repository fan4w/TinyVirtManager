#ifndef VIRCONNECT_H
#define VIRCONNECT_H

#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include "virDomain.h"
#include "driver-hypervisor.h"

class VirConnect {
private:
    std::string uri;                                   // 连接 URI
    std::unique_ptr<HypervisorDriver> driver;          // 驱动实例
    std::map<std::string, std::shared_ptr<VirDomain>> domains; // 虚拟机容器

public:
    // 构造函数
    explicit VirConnect(const std::string& uri);

    // 虚拟机管理方法
    std::shared_ptr<VirDomain> createDomain(const std::string& name, int memory, int vcpus);
    void deleteDomain(const std::string& name);
    std::shared_ptr<VirDomain> getDomain(const std::string& name) const;
    void showAllDomain() const;

    // 调用驱动的方法
    void startVM(const std::string& name);
    void stopVM(const std::string& name);
    std::string getInfo() const;
};

#endif // VIRCONNECT_H
