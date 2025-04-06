#ifndef QEMU_DRIVER_H
#define QEMU_DRIVER_H

#include "../driver-hypervisor.h"
#include "qemu_monitor.h"
#include "qemu_conf.h"
#include "qemu_domain.h"
#include "../conf/domain_conf.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>


class QemuDriver : public HypervisorDriver {
private:
    // std::unordered_map<std::string, std::string> domainSockets; // 存储虚拟机的socket
    QemuDriverConfig config;
    std::vector<std::shared_ptr<qemuDomainObj>> domains;

    static int idCounter;
    // 辅助函数
    void loadAllDomainConfigs();
    std::string readFileContent(const std::string& filePath) const;
    std::shared_ptr<qemuDomainObj> parseAndCreateDomainObj(const std::string& xmlDesc);
    int processQemuObject(std::shared_ptr<qemuDomainObj> domainObj);
    // int processQemuObject(std::shared_ptr<qemuDomainObj> domainObj);
    int generateUniqueID();
public:
    // 构造函数与析构函数
    QemuDriver();
    ~QemuDriver();

    // 获取配置方法
    QemuDriverConfig getConfig() const {
        return config;
    }

    // 获取虚拟机列表
    std::vector<std::shared_ptr<VirDomain>> connectListAllDomains(unsigned int flags = 0) const override;

    // 查找虚拟机
    std::shared_ptr<VirDomain> domainLookupByName(const std::string& name) const override;

    // 实现虚拟机操作
    std::shared_ptr<VirDomain> domainDefineXML(const std::string& xml) override;
    std::shared_ptr<VirDomain> domainDefineXMLFlags(const std::string& xml, unsigned int flags) override;

    void domainCreate(std::shared_ptr<VirDomain> domain) override;
    std::shared_ptr<VirDomain> domainCreateXML(const std::string& xmlDesc) override;

    void domainDestroy(std::shared_ptr<VirDomain> domain) override;
    void domainShutdown(std::shared_ptr<VirDomain> domain) override;

    int domainUndefine(std::shared_ptr<VirDomain> domain) override;
    int domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) override;

    int domainGetState(std::shared_ptr<VirDomain> domain) override;
};

#endif // QEMU_DRIVER_H
