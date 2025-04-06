#include <iostream>
#include "virConnect.h"

void printUsage() {
    std::cout << "Usage: ./myVirsh <command> [options]\n\n"
        << "Commands:\n"
        << "  list              列出所有虚拟机\n"
        << "  start <domain>    启动指定虚拟机\n"
        << "  destroy <domain>  强制关闭指定虚拟机\n"
        << "  shutdown <domain> 优雅关闭指定虚拟机"
        << "  status <domain>   查询指定虚拟机状态\n"
        << "  help              显示此帮助信息\n";
}

int main(int argc, char* argv[])
{
    if ( argc < 2 ) {
        printUsage();
        return 1;
    }

    if ( std::string(argv[1]) == "help" ) {
        printUsage();
        return 0;
    }

    // 解析命令
    std::string command = argv[1];

    if ( command == "list" ) {
        // 建立连接
        VirConnect conn("qemu:///system");
        std::vector<std::shared_ptr<VirDomain>> domains = conn.virConnectListAllDomains();
        for ( const auto& domain : domains ) {
            std::cout << "Domain name: " << domain->virDomainGetName() << std::endl;
        }
    }
    else if ( command == "start" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少域名参数\n";
            printUsage();
            return 1;
        }
        // 建立连接
        VirConnect conn("qemu:///system");
        const char* domainName = argv[2];
        std::shared_ptr<VirDomain> domain = conn.virDomainLookupByName(domainName);

        if ( domain == NULL ) {
            std::cerr << "错误: 找不到域 '" << domainName << "'\n";
            return 1;
        }

        try {
            conn.virDomainCreate(domain);
        }
        catch ( const std::exception& e ) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    else if ( command == "shutdown" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少域名参数\n";
            printUsage();
            return 1;
        }
        // 建立连接
        VirConnect conn("qemu:///system");
        const char* domainName = argv[2];
        std::shared_ptr<VirDomain> domain = conn.virDomainLookupByName(domainName);

        if ( domain == NULL ) {
            std::cerr << "错误: 找不到域 '" << domainName << "'\n";
            return 1;
        }

        try {
            conn.virDomainShutdown(domain);
        }
        catch ( const std::exception& e ) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    else if ( command == "destroy" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少域名参数\n";
            printUsage();
            return 1;
        }
        // 建立连接
        VirConnect conn("qemu:///system");
        const char* domainName = argv[2];
        std::shared_ptr<VirDomain> domain = conn.virDomainLookupByName(domainName);

        if ( domain == NULL ) {
            std::cerr << "错误: 找不到域 '" << domainName << "'\n";
            return 1;
        }

        try {
            conn.virDomainDestroy(domain);
        }
        catch ( const std::exception& e ) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    else if ( command == "status" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少域名参数\n";
            printUsage();
            return 1;
        }
        // 建立连接
        VirConnect conn("qemu:///system");
        const char* domainName = argv[2];
        std::shared_ptr<VirDomain> domain = conn.virDomainLookupByName(domainName);

        if ( domain == NULL ) {
            std::cerr << "错误: 找不到域 '" << domainName << "'\n";
            return 1;
        }
        try
        {
            unsigned int reason = 0;
            int state = domain->virDomainGetState(reason);
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
    }
    else {
        std::cerr << "未知命令: " << command << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}