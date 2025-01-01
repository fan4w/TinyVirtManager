#include "../monitor/monitor.h"

int main() {
    // 记得路径改成创建虚拟机命令-qmp指定的路径
    // 第一次创建的情况下可以直接用路径创建
    Monitor* mon = new Monitor("/root/unix_sockets/cirros.sock");
    // 如果不是第一次创建的话需要重新设置路径
    // dom->mon->setUnixSocketPath("/root/unix_sockets/cirros.sock");

    // 2. 连接qemu的socket
    // 每次虚拟机启动都应该调用这个函数来连接
    if (mon->qemuMonitorOpenUnixSocket() < 0) {
        return -1;
    }
    
    // 3.模拟命令发送
    std::string cmd = "{ \"execute\":\"query-status\"}";
    std::string cmdQuit = "{ \"execute\":\"quit\"}";
    std::string result;
    
    std::cout << "first send..." << std::endl;
    if (mon->qemuMonitorSendMessage(cmd, result) < 0) {
        return -1;
    }
    // std::cout << "received msg: " << result << std::endl;

    std::cout << "second send..." << std::endl;
    if (mon->qemuMonitorSendMessage(cmdQuit, result) < 0) {
        return -1;
    }
    // std::cout << "received msg: " << result << std::endl;

    // 连续发送的测试
    // while (true) {
    //     // 输入要发送的消息
    //     std::cout << "Enter message to send (or 'q' to quit): ";
    //     std::string message;
    //     // std::string message = "client auto send a message";
    //     std::getline(std::cin, message);
    //     if (message == "q") {
    //         break;
    //     }
    //     mon->qemuMonitorSendMessage(message, result);
    //     std::cout << "result:" << result << std::endl;

    // }

    // 4. 关闭socket
    // 理论上各种原因QEMU的socket关闭时都应该调用这个，目前没搞清楚
    mon->qemuMonitorCloseUnixSocket();


    return 0;

}