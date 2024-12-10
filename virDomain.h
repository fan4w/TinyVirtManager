#ifndef VIRDOMAIN_H
#define VIRDOMAIN_H

#include <string>
#include <stdexcept>
#include <iostream>
#include "driver_hypervisor.h"

class VirDomain {
private:
    std::string name;       // 虚拟机名称
    int memory;             // 虚拟机内存大小 (MB)
    int vcpus;              // 虚拟机虚拟 CPU 数量
    bool isRunning;         // 虚拟机当前状态
    HypervisorDriver* driver; // 对应的驱动指针，供操作虚拟机使用

public:
    // 构造函数
    VirDomain(const std::string& name, int memory, int vcpus, HypervisorDriver* driver);

    // 虚拟机操作
    void start();           // 启动虚拟机
    void shutdown();        // 关闭虚拟机
    void reboot();          // 重启虚拟机
    std::string getInfo() const; // 获取虚拟机信息

    // 获取属性方法
    std::string getName() const;
    int getMemory() const;
    int getVCPUs() const;
    bool getState() const;
};

#endif // VIRDOMAIN_H
