#ifndef DRIVER_NETWORK_H
#define DRIVER_NETWORK_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "./conf/network_conf.h"

class VirNetwork;

// 网络层没必要实现驱动机制，所以不使用虚函数进行重载

class NetworkDriver {
public:
    NetworkDriver();
    // 网络相关操作
    std::vector<std::shared_ptr<VirNetwork>> connectListAllNetworks(unsigned int flags = 0);
    std::shared_ptr<VirNetwork> networkLookupByName(const std::string& name);
    std::shared_ptr<VirNetwork> networkLookupByUUID(const std::string& uuid);

    std::shared_ptr<VirNetwork> networkDefineXML(const std::string& xml, unsigned int flags = 0);
    void networkCreate(std::shared_ptr<VirNetwork> network);
    void networkDestroy(std::shared_ptr<VirNetwork> network);
    void networkUndefine(std::shared_ptr<VirNetwork> network);

    // 将网络接口绑定到虚拟机
    bool attachNetworkInterface(const std::string& domainName,
        const std::string& networkName,
        const std::string& macAddress = "",
        const std::string& deviceModel = "virtio");

    // 从虚拟机分离网络接口
    bool detachNetworkInterface(const std::string& domainName, const std::string& macAddress);

private:
    std::string configDir; // 配置目录
    // 网络池管理
    std::vector<std::shared_ptr<networkObj>> networks; // 网络池列表

    // 在NetworkDriver类的私有成员中添加
    struct DomainNetworkInterface {
        std::string domainName;
        std::string networkName;
        std::string interfaceName;
        std::string macAddress;
        std::string deviceModel;
        std::string tapDeviceName;  // 如果适用
    };

    // 存储域名和网络接口的映射关系
    std::map<std::string, std::vector<DomainNetworkInterface>> domainNetworkInterfaces;

    // 辅助函数
    void loadAllNetworkConfigs();
    std::shared_ptr<networkObj> parseAndCreateNetworkObj(const std::string& xmlDesc);
};


#endif // DRIVER_NETWORK_H