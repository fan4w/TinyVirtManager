#ifndef DOMAIN_CONF_H
#define DOMAIN_CONF_H

#include <string>
#include <vector>
#include <memory>

// 声明基础域定义类
class virDomainDef {
    // TODO: 用Public不是个好主意 :(
public:
    // 基本信息
    std::string name;           // 虚拟机名称
    std::string uuid;           // 虚拟机UUID
    int id;                     // 运行时ID

    // 硬件配置
    unsigned long memory;       // 内存大小
    std::string memoryUnit;     // 内存单位
    int vcpus;                  // 虚拟CPU数量

    std::string diskPath;
    std::string cdromPath;

    // 虚拟化类型
    std::string type;           // 虚拟化类型(kvm, qemu等)
    std::string arch;           // 架构(x86_64等)

    // 用于存储XML描述
    std::string xmlDesc;  

    // 设备和网络配置...
    // (省略其他配置项)
    virtual ~virDomainDef() = default;
};

// 定义状态原因类
class virDomainStateReason {
public:
    int state;
    int reason;
};

// 域对象类
class virDomainObj {
public:
    // 用于多线程的锁之类的数据，暂时不实现
    // virDomainObj和virDomainDef需要区分实现，其中virDomainDef中存储的是虚拟机的静态数据
    // virDomainObj中存放虚拟机的动态数据，并且有利于实现线程安全和热迁移
    // 对于第一版实现，暂时不考虑多线程的实现

    // 用于存储虚拟机的运行时状态
    pid_t pid;
    virDomainStateReason stateReason;

    // 各种标志位
    unsigned int autostart : 1;
    unsigned int persistent : 1;
    unsigned int updated : 1;
    unsigned int removing : 1;

    // 使用智能指针管理内存
    std::shared_ptr<virDomainDef> def;
    std::shared_ptr<virDomainDef> newDef;

    virtual ~virDomainObj() = default;
};

#endif // DOMAIN_CONF_H