#ifndef XEN_DRIVER_H
#define XEN_DRIVER_H

#include "../driver-hypervisor.h"
#include "xen_domain.h"
#include "../conf/domain_conf.h"
#include "../conf/config_manager.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>


class XenDriver : public HypervisorDriver {
private:
    std::vector<std::shared_ptr<xenDomainObj>> domains;

    ConfigManager* configManager;   // 配置管理器
    static int idCounter;
    // 辅助函数
    void loadAllDomainConfigs();
    std::string readFileContent(const std::string& filePath) const;
    std::shared_ptr<xenDomainObj> parseAndCreateDomainObj(const std::string& xmlDesc);
    int processXenObject(std::shared_ptr<xenDomainObj> domainObj);
    // int processXenObject(std::shared_ptr<xenDomainObj> domainObj);
    int generateUniqueID();
public:
    // 构造函数与析构函数
    XenDriver();
    ~XenDriver();

    // 获取虚拟机列表
    std::vector<std::shared_ptr<VirDomain>> connectListAllDomains(unsigned int flags = 0) const override;

    // 查找虚拟机
    std::shared_ptr<VirDomain> domainLookupByName(const std::string& name) const override;

    // 实现虚拟机操作
    std::shared_ptr<VirDomain> domainDefineXML(const std::string& xml) override;
    std::shared_ptr<VirDomain> domainDefineXMLFlags(const std::string& xml, unsigned int flags) override;

    void domainCreate(std::shared_ptr<VirDomain> domain) override;
    std::shared_ptr<VirDomain> domainCreateXML(const std::string& xmlDesc) override;

    int domainAttachDevice(std::shared_ptr<VirDomain> domain, const std::string& xmlDesc, unsigned int flags) override;

    void domainDestroy(std::shared_ptr<VirDomain> domain) override;
    void domainShutdown(std::shared_ptr<VirDomain> domain) override;

    int domainUndefine(std::shared_ptr<VirDomain> domain) override;
    int domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) override;

    int domainGetState(std::shared_ptr<VirDomain> domain) override;
};

#endif // XEN_DRIVER_H
