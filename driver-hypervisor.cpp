#include "driver-hypervisor.h"
#include "qemu/qemu_driver.h"
#include "xen/xen_driver.h"
#include "log/log.h"

// 初始化函数，用于注册所有驱动
void initializeDrivers() {
    static bool initialized = false;
    if ( !initialized ) {
        REGISTER_DRIVER("qemu", QemuDriver);
        REGISTER_DRIVER("xen", XenDriver);
        initialized = true;
    }
}

void DriverFactory::registerDriver(const std::string& pattern, Creator creator) {
    getRegistry()[pattern] = std::move(creator);
}

std::map<std::string, DriverFactory::Creator>& DriverFactory::getRegistry() {
    static std::map<std::string, DriverFactory::Creator> driverRegistry;
    return driverRegistry;
}

std::unique_ptr<HypervisorDriver> DriverFactory::createDriver(const std::string& uri) {
    // 确保驱动已注册
    initializeDrivers();

    // 解析URI获取协议类型
    size_t pos = uri.find("://");
    std::string protocol;

    if ( pos != std::string::npos ) {
        protocol = uri.substr(0, pos);
        LOG_INFO("Creating driver for protocol: %s", protocol.c_str());
    }
    else {
        LOG_ERROR("Invalid URI format: %s", uri.c_str());
        return nullptr;
    }

    // 从注册表中查找对应的驱动创建函数
    auto& registry = getRegistry();
    auto it = registry.find(protocol);

    if ( it != registry.end() ) {
        // 找到对应的驱动创建函数，调用它创建驱动实例
        return it->second();
    }
    else {
        LOG_ERROR("Driver not found for URI: %s", uri.c_str());
        return nullptr;
    }
}