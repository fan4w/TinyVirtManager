#include "qemu_driver.h"

QemuDriver::QemuDriver() {
    std::cout << "QEMU Driver initialized." << std::endl;
}

QemuDriver::~QemuDriver() {
    std::cout << "QEMU Driver destroyed." << std::endl; 
}

// 启动虚拟机
// TODO: 实现真正的虚拟机启动
void QemuDriver::startVM(const std::string& name) {
    if (domains.find(name) != domains.end() && domains[name]) {
        throw std::runtime_error("Domain " + name + " is already running.");
    }
    domains[name] = true; // 模拟虚拟机启动
    std::cout << "QEMU: Domain " << name << " started." << std::endl;
}

// 停止虚拟机
// TODO: 实现真正的虚拟机停止
void QemuDriver::stopVM(const std::string& name) {
    if (domains.find(name) == domains.end() || !domains[name]) {
        throw std::runtime_error("Domain " + name + " is not running.");
    }
    domains[name] = false; // 模拟虚拟机停止
    std::cout << "QEMU: Domain " << name << " stopped." << std::endl;
}

// 获取驱动信息
std::string QemuDriver::getInfo() const {
    return "QEMU Driver managing virtual machines.";
}
