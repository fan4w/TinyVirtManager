#include <iostream>
#include "virConnect.h"

int main() {
    try {
        // 创建与 QEMU 的连接
        std::cout << "Initializing connection to QEMU..." << std::endl;
        VirConnect conn("qemu:///system");

        // 创建虚拟机
        std::cout << "Creating virtual machines..." << std::endl;
        auto vm1 = conn.createDomain("VM1", 2048, 2); // 名称为 VM1，内存 2048MB，2 个 vCPU
        auto vm2 = conn.createDomain("VM2", 4096, 4); // 名称为 VM2，内存 4096MB，4 个 vCPU

        // 启动虚拟机
        std::cout << "Starting VM1..." << std::endl;
        conn.startVM("VM1");
        std::cout << vm1->getInfo() << std::endl;

        std::cout << "Starting VM2..." << std::endl;
        conn.startVM("VM2");
        std::cout << vm2->getInfo() << std::endl;

        // 重启虚拟机
        std::cout << "Rebooting VM1..." << std::endl;
        vm1->reboot();
        std::cout << vm1->getInfo() << std::endl;

        // 停止虚拟机
        std::cout << "Stopping VM2..." << std::endl;
        conn.stopVM("VM2");
        std::cout << vm2->getInfo() << std::endl;

        // 删除虚拟机
        std::cout << "Deleting VM2..." << std::endl;
        conn.deleteDomain("VM2");

        // 测试对不存在虚拟机的操作
        try {
            std::cout << "Attempting to start non-existent VM3..." << std::endl;
            conn.startVM("VM3");
        }
        catch ( const std::exception& e ) {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }

    }
    catch ( const std::exception& e ) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
