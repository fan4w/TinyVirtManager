#pragma once
#include <iostream>

// 每个虚拟机对象有一个Monitor对象，这个对象必须是线程安全的
class Monitor {
private:
    std::string unixSocketPath;  // UNIX SOCKET路径
    std::string addr;  // socket IP
    int port;  // socket port

    bool open;  // 是否在连接状态（似乎没什么用）
    int unixSocketFd;  // 内部连接unixSocket的fd

public:
    int qemuMonitorOpenUnixSocket();
    int qemuMonitorCloseUnixSocket();
    int qemuMonitorNegotiation();  // QMP协议握手，理论上应该设为private，只在Open内部调用，防止有问题先不改
    int qemuMonitorSendMessage(const std::string, std::string& reply);  // 直接发送给定指令并获取返回结果
    

    // 构造函数与析构函数
    Monitor() {};
    Monitor(std::string socketPath):unixSocketPath(socketPath){};
    ~Monitor();

    bool isOpen() { return this->open; }
    void setUnixSocketPath(std::string path) {this->unixSocketPath = path;};


};