#ifndef NETWORK_CONF_H
#define NETWORK_CONF_H

#include <string>

typedef enum {
    NAT,        // NAT 转发模式
    BRIDGE,     // 桥接模式
    ROUTE,      // 路由模式
    PRIVATE     // 私有模式
}networkForwardType;

class networkObj {
public:
    std::string name;           // 网络名称
    std::string uuid;           // 网络UUID
    std::string xmlDesc;        // 网络XML描述，便于开发和调试
    networkForwardType forward; // 转发模式
    std::string bridgeName;     // 桥接名称
    std::string ipAddress;      // IP地址
    std::string netmask;        // 子网掩码
    std::string macAddress;     // MAC地址
    std::pair<std::string, std::string> DHCPRange;      // DHCP范围
    std::string tapDeviceName;  // 用于存储创建的TAP设备名称，用于可能的清理操作
    bool active;                // 网络是否处于活动状态
    bool persistent;            // 网络是否持久化

    // 修改初始化顺序以匹配类定义中的成员顺序
    networkObj() :forward(BRIDGE), active(false), persistent(false) {}

    networkObj(const std::string& name, const std::string& uuid, const std::string& xmlDesc)
        : name(name), uuid(uuid), xmlDesc(xmlDesc), forward(BRIDGE), active(false), persistent(false) {
    }
};

#endif // NETWORK_CONF_H
