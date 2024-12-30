#include "driver-hypervisor.h"
#include "qemu/qemu_driver.h"

std::unique_ptr<HypervisorDriver> DriverFactory::createDriver(const std::string& uri) {
    // TODO: 这个逻辑判断地方还需要优化
    if (uri.find("qemu") == 0) {
        return std::unique_ptr<HypervisorDriver>(new QemuDriver());
    }
    // TODO: 实现其他驱动
    // else if (uri.find("xen") == 0) {
    //     return std::make_unique<XenDriver>();
    // }
    else {
        throw std::invalid_argument("Unsupported hypervisor type in URI: " + uri);
    }
}