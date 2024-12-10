#include "virDomain.h"

// 构造函数
VirDomain::VirDomain(const std::string& name, int memory, int vcpus, HypervisorDriver* driver)
    : name(name), memory(memory), vcpus(vcpus), isRunning(false), driver(driver) {
    if ( driver == nullptr ) {
        throw std::invalid_argument("Driver cannot be null.");
    }
}

// 启动虚拟机
void VirDomain::start() {
    if ( isRunning ) {
        throw std::runtime_error("Domain " + name + " is already running.");
    }
    driver->startVM(name);  // 调用驱动的启动方法
    isRunning = true;
    std::cout << "Domain " << name << " started." << std::endl;
}

// 关闭虚拟机
void VirDomain::shutdown() {
    if ( !isRunning ) {
        throw std::runtime_error("Domain " + name + " is not running.");
    }
    driver->stopVM(name);   // 调用驱动的停止方法
    isRunning = false;
    std::cout << "Domain " << name << " shut down." << std::endl;
}

// 重启虚拟机
void VirDomain::reboot() {
    if ( !isRunning ) {
        throw std::runtime_error("Domain " + name + " is not running.");
    }
    shutdown();  // 调用关机方法
    start();     // 调用启动方法
    std::cout << "Domain " << name << " rebooted." << std::endl;
}

// 获取虚拟机信息
std::string VirDomain::getInfo() const {
    return "Domain Info:\n" +
        ("Name: " + name + "\n") +
        ("Memory: " + std::to_string(memory) + " MB\n") +
        ("vCPUs: " + std::to_string(vcpus) + "\n") +
        "State: " + (isRunning ? "Running" : "Stopped") + "\n";
}

// 获取名称
std::string VirDomain::getName() const {
    return name;
}

// 获取内存
int VirDomain::getMemory() const {
    return memory;
}

// 获取虚拟 CPU 数量
int VirDomain::getVCPUs() const {
    return vcpus;
}

// 获取状态
bool VirDomain::getState() const {
    return isRunning;
}
