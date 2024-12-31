#include <iostream>
#include "virConnect.h"

int main() {
    try {
        // 创建与 QEMU 的连接
        std::cout << "Initializing connection to QEMU..." << std::endl;
        VirConnect conn("qemu:///system");
        std::vector<std::shared_ptr<VirDomain>> domains = conn.virConnectListAllDomains();
        for ( const auto& domain : domains ) {
            std::cout << "Domain name: " << domain->virDomainGetName() << std::endl;
        }

        std::shared_ptr<VirDomain> domain = conn.virDomainLookupByName("test");
        std::cout << "this Domain name: " << domain->virDomainGetName() << std::endl;
        conn.virDomainCreate(domain);

        // // 创建虚拟机
        // std::string xmlDesc = "<domain type='kvm'>"
        //                       "  <name>test</name>"
        //                       "  <memory unit='KiB'>1048576</memory>"
        //                       "  <vcpu placement='static'>1</vcpu>"
        //                       "  <os>"
        //                       "    <type arch='x86_64' machine='pc-i440fx-2.10'>hvm</type>"
        //                       "    <boot dev='hd'/>"
        //                       "  </os>"
        //                       "  <devices>"
        //                       "    <disk type='file' device='disk'>"
        //                       "      <driver name='qemu' type='qcow2'/>"
        //                       "      <source file='/var/lib/libvirt/images/test.qcow2'/>"
        //                       "      <target dev='vda' bus='virtio'/>"
        //                       "    </disk>"
        //                       "    <interface type='network'>"
        //                       "      <mac address='52:54:00:00:00:01'/>"
        //                       "      <source network='default'/>"
        //                       "    </interface>"
        //                       "  </devices>"
        //                       "</domain>";

        // std::shared_ptr<VirDomain> domain = conn.virDomainDefineXML(xmlDesc);
        // std::cout << "Domain name: " << domain->virDomainGetName() << std::endl;

        // conn.virDomainCreateXML(xmlDesc);

        // // 获取虚拟机的状态
        // unsigned int reason;
        // int state = domain->virDomainGetState(reason);
        // std::cout << "Domain state: " << state << std::endl;

        // // 停止虚拟机
        // conn.virDomainDestroy(domain);
    }
    catch ( const std::exception& e ) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
