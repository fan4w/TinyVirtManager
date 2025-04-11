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

        std::cout << "domain ID: " << domain->virDomainGetID() << std::endl;
        
        unsigned int reason = 0;
        int state = domain->virDomainGetState(reason);
        if ( state == VIR_DOMAIN_RUNNING ) {
            std::cout << "Domain " << domain->virDomainGetName() << " is running." << std::endl;
        }
        else {
            std::cout << "Domain " << domain->virDomainGetName() << " is not running." << std::endl;
        }

        state = domain->virDomainGetState(reason);
        if ( state == VIR_DOMAIN_RUNNING ) {
            std::cout << "Domain " << domain->virDomainGetName() << " is running." << std::endl;
        }
        else {
            std::cout << "Domain " << domain->virDomainGetName() << " is not running." << std::endl;
        }
    }
    catch ( const std::exception& e ) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
