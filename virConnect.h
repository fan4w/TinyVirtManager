#ifndef VIRCONNECT_H
#define VIRCONNECT_H

#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
#include "virDomain.h"
#include "driver-hypervisor.h"

typedef enum {
    VIR_DOMAIN_NOSTATE = 0,     /* no state */
    VIR_DOMAIN_RUNNING = 1,     /* the domain is running */
    VIR_DOMAIN_BLOCKED = 2,     /* the domain is blocked on resource */
    VIR_DOMAIN_PAUSED = 3,     /* the domain is paused by user */
    VIR_DOMAIN_SHUTDOWN = 4,     /* the domain is being shut down */
    VIR_DOMAIN_SHUTOFF = 5,     /* the domain is shut off */
    VIR_DOMAIN_CRASHED = 6,     /* the domain is crashed */
    VIR_DOMAIN_PMSUSPENDED = 7, /* the domain is suspended by guest
                                   power management */
} virDomainState;

/**
 * virDomainDefineFlags:
 */
typedef enum {
    VIR_DOMAIN_DEFINE_VALIDATE = (1 << 0), /* Validate the XML document against schema */
} virDomainDefineFlags;

class VirConnect {
private:
    std::string uri;                                   // 连接 URI
    std::unique_ptr<HypervisorDriver> driver;          // 驱动实例
    std::vector<std::shared_ptr<VirDomain>> domains; // 虚拟机链表

    // 用于存放虚拟机配置文件的目录
    // TODO: 这里写死不太好，应该写入一个配置文件中，暂时先这样
    const std::string pathToConfigDir = "./temp/domains";
public:
    // 仿照Libvirt中的定义，增加一些方法，请根据以下方法编写程序
    explicit VirConnect(const std::string& uri, unsigned int flags = 0);

    // 派生其它对象
    std::shared_ptr<VirDomain> virDomainDefineXML(const std::string& xmlDesc);
    // TODO: 建立网络对象以及存储对象

    // 删除其它对象
    void virDomainUndefine(const std::shared_ptr<VirDomain> domain);
    // TODO: 删除网络对象以及存储对象

    // Lookup: 用于通过某种类型的标识符对对象执行查找
    std::shared_ptr<VirDomain> virDomainLookupByName(const std::string& name) const;
    std::shared_ptr<VirDomain> virDomainLookupByID(const int& id) const;
    std::shared_ptr<VirDomain> virDomainLookupByUUID(const std::string& uuid) const;

    // Enumeration: 用于枚举给定的 hypervisor 上可用的一组对象
    std::vector<std::shared_ptr<VirDomain>> virConnectListAllDomains(unsigned int flags = 0) const;
    // TODO: 枚举HyperVisor上的网络对象以及存储对象

    // Description: 通用访问器，提供一组关于对象的通用信息

    // Accessors: 特定的访问器方法，用于查询或修改给定对象的数据
    // TODO：需要定义一个结点的信息的数据结构用于返回值

    // 为了和Libvirt中的方法定义保持一致，这里的Create和Destroy方法也遵循libvirt中的定义，即：
    // Create真正的作用其实是启动一个已定义的虚拟机，或启动一个临时的虚拟机
    // Destroy则是强制关机一个虚拟机，但并不删除虚拟机定义

    // Creation: 用于创建和启动对象。...CreateXML API 将根据 XML 描述创建对象，而 ...Create API 将基于现有对象指针创建对象
    // Domain对象由VirConnect对象创建，所以这里的Create方法应该是VirConnect的方法
    std::shared_ptr<VirDomain> virDomainCreateXML(const std::string& xmlDesc, unsigned int flags = 0);
    void virDomainCreate(const std::shared_ptr<VirDomain> domain, unsigned int flags = 0);

    // Destruction: 用于关闭或停用并析构对象
    void virDomainDestroy(const std::shared_ptr<VirDomain> domain);
};

#endif // VIRCONNECT_H
