#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "virConnect.h"

void printUsage() {
    std::cout << "Usage: ./myVirsh <command> [options]\n\n"
        << "虚拟机命令:\n"
        << "  list                     列出所有虚拟机\n"
        << "  start <domain>           启动指定虚拟机\n"
        << "  destroy <domain>         强制关闭指定虚拟机\n"
        << "  shutdown <domain>        优雅关闭指定虚拟机\n"
        << "  status <domain>          查询指定虚拟机状态\n\n"
        << "存储池命令:\n"
        << "  pool-list                列出所有存储池\n"
        << "  pool-define-xml <file>   从XML文件定义存储池\n"
        << "  pool-create <name>       激活存储池\n"
        << "  pool-destroy <name>      停用存储池\n"
        << "  pool-undefine <name>     取消定义存储池\n\n"
        << "存储卷命令:\n"
        << "  vol-list <pool>          列出指定存储池中的所有存储卷\n"
        << "  vol-create-xml <pool> <file>  从XML文件创建存储卷\n"
        << "  vol-delete <pool> <vol>  删除存储卷\n\n"
        << "网络命令:\n"
        << "  net-list                 列出所有网络\n"
        << "  net-define-xml <file>    从XML文件定义网络\n"
        << "  net-start <name>         启动指定网络\n"
        << "  net-destroy <name>       停用指定网络\n"
        << "  net-undefine <name>      取消定义网络\n\n"
        << std::endl
        << "  help                     显示此帮助信息\n";
}

// 辅助函数：将虚拟机状态转换为可读字符串
std::string getDomainStateString(int state) {
    switch ( state ) {
    case VIR_DOMAIN_NOSTATE: return "无状态";
    case VIR_DOMAIN_RUNNING: return "运行中";
    case VIR_DOMAIN_BLOCKED: return "阻塞";
    case VIR_DOMAIN_PAUSED: return "已暂停";
    case VIR_DOMAIN_SHUTDOWN: return "正在关闭";
    case VIR_DOMAIN_SHUTOFF: return "已关闭";
    case VIR_DOMAIN_CRASHED: return "已崩溃";
    case VIR_DOMAIN_PMSUSPENDED: return "电源已挂起";
    default: return "未知";
    }
}

// 辅助函数：将存储池状态转换为可读字符串
std::string getPoolStateString(int state) {
    switch ( state ) {
    case VIR_STORAGE_POOL_INACTIVE: return "未激活";
    case VIR_STORAGE_POOL_BUILDING: return "正在初始化";
    case VIR_STORAGE_POOL_RUNNING: return "运行中";
    case VIR_STORAGE_POOL_DEGRADED: return "降级";
    case VIR_STORAGE_POOL_INACCESSIBLE: return "无法访问";
    default: return "未知";
    }
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

        // 打印表头
        std::cout << std::setw(5) << std::left << "ID"
            << std::setw(30) << std::left << "名称"
            << std::setw(15) << std::left << "状态" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        // 打印每个域的信息
        for ( const auto& domain : domains ) {
            unsigned int id = 0;
            try {
                id = domain->virDomainGetID();
            }
            catch ( ... ) {
                // 如果域没有运行，ID 通常为 -1 或抛出异常
                id = -1;
            }

            unsigned int reason = 0;
            int state;
            try {
                state = domain->virDomainGetState(reason);
            }
            catch ( ... ) {
                state = -1;
            }

            std::cout << std::setw(5) << std::left
                << (id == static_cast< unsigned int >(-1) ? "-" : std::to_string(id))
                << std::setw(30) << std::left << domain->virDomainGetName()
                << std::setw(15) << std::left << getDomainStateString(state)
                << std::endl;
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
    else if ( command == "pool-list" ) {
        // 建立连接
        VirConnect conn("qemu:///system");
        std::vector<std::shared_ptr<VirStoragePool>> pools = conn.virConnectListAllStoragePools();

        // 打印表头
        std::cout << std::setw(30) << std::left << "名称"
            // << std::setw(36) << std::left << "UUID"
            << std::setw(15) << std::left << "状态" << std::endl;
        std::cout << std::string(60, '-') << std::endl;

        // 打印每个存储池的信息
        for ( const auto& pool : pools ) {
            int state = pool->virStoragePoolGetState();

            std::cout << std::setw(30) << std::left << pool->virStoragePoolGetName()
                // << std::setw(36) << std::left << pool->virStoragePoolGetUUID()
                << std::setw(15) << std::left << getPoolStateString(state)
                << std::endl;
        }
    }
    else if ( command == "pool-define-xml" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少XML文件路径\n";
            printUsage();
            return 1;
        }
        // 读取XML文件
        const char* xmlFile = argv[2];
        std::ifstream file(xmlFile);
        if ( !file.is_open() ) {
            std::cerr << "错误: 无法打开文件 '" << xmlFile << "'\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string xmlDesc = buffer.str();

        // 建立连接并定义存储池
        try {
            VirConnect conn("qemu:///system");
            std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolDefineXML(xmlDesc);
            std::cout << "存储池 '" << pool->virStoragePoolGetName() << "' 已定义\n";
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    else if ( command == "pool-create" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少存储池名参数\n";
            printUsage();
            return 1;
        }
        // 建立连接
        try {
            VirConnect conn("qemu:///system");
            const char* poolName = argv[2];
            std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);
            conn.virStoragePoolCreate(pool);
            std::cout << "存储池 '" << poolName << "' 已激活\n";
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    else if ( command == "pool-destroy" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少存储池名参数\n";
            printUsage();
            return 1;
        }
        try {
            VirConnect conn("qemu:///system");
            const char* poolName = argv[2];
            std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);
            conn.virStoragePoolDestroy(pool);
            std::cout << "存储池 '" << poolName << "' 已停用\n";
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    else if ( command == "pool-undefine" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少存储池名参数\n";
            printUsage();
            return 1;
        }
        try {
            VirConnect conn("qemu:///system");
            const char* poolName = argv[2];
            std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);
            conn.virStoragePoolUndefine(pool);
            std::cout << "存储池 '" << poolName << "' 已取消定义\n";
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    else if ( command == "vol-list" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少存储池名参数\n";
            printUsage();
            return 1;
        }
        try {
            VirConnect conn("qemu:///system");
            const char* poolName = argv[2];
            std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);

            // 列出所有存储卷
            std::vector<std::shared_ptr<VirStorageVol>> volumes = pool->virStoragePoolListAllVolumes();

            // 打印表头
            std::cout << std::setw(30) << std::left << "名称"
                << std::setw(15) << std::left << "大小(字节)"
                << std::setw(50) << std::left << "路径" << std::endl;
            std::cout << std::string(95, '-') << std::endl;

            // 打印每个存储卷的信息
            for ( const auto& vol : volumes ) {
                std::cout << std::setw(30) << std::left << vol->virStorageVolGetName()
                    << std::setw(15) << std::left << vol->virStorageVolGetCapacity()
                    << std::setw(50) << std::left << vol->virStorageVolGetPath()
                    << std::endl;
            }
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    // else if ( command == "vol-create-xml" ) {
    //     if ( argc < 4 ) {
    //         std::cerr << "错误: 缺少存储池名或XML文件路径\n";
    //         printUsage();
    //         return 1;
    //     }
    //     // 读取XML文件
    //     const char* xmlFile = argv[3];
    //     std::ifstream file(xmlFile);
    //     if ( !file.is_open() ) {
    //         std::cerr << "错误: 无法打开文件 '" << xmlFile << "'\n";
    //         return 1;
    //     }
    //     std::stringstream buffer;
    //     buffer << file.rdbuf();
    //     std::string xmlDesc = buffer.str();

    //     // 建立连接并创建存储卷
    //     try {
    //         VirConnect conn("qemu:///system");
    //         const char* poolName = argv[2];
    //         std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);
    //         std::shared_ptr<VirStorageVol> vol = pool->virStorageVolCreateXML(xmlDesc);
    //         std::cout << "存储卷 '" << vol->virStorageVolGetName() << "' 已创建\n";
    //     }
    //     catch ( const std::exception& e ) {
    //         std::cerr << "错误: " << e.what() << std::endl;
    //         return 1;
    //     }
    // }
    // else if ( command == "vol-delete" ) {
    //     if ( argc < 4 ) {
    //         std::cerr << "错误: 缺少存储池名或存储卷名参数\n";
    //         printUsage();
    //         return 1;
    //     }
    //     try {
    //         VirConnect conn("qemu:///system");
    //         const char* poolName = argv[2];
    //         const char* volName = argv[3];
    //         std::shared_ptr<VirStoragePool> pool = conn.virStoragePoolLookupByName(poolName);
    //         std::shared_ptr<VirStorageVol> vol = pool->virStorageVolLookupByName(volName);
    //         conn.virStorageVolDelete(vol);
    //         std::cout << "存储卷 '" << volName << "' 已删除\n";
    //     }
    //     catch ( const std::exception& e ) {
    //         std::cerr << "错误: " << e.what() << std::endl;
    //         return 1;
    //     }
    // }
    else if ( command == "net-list" ) {
        // 建立连接
        VirConnect conn("qemu:///system");
        std::vector<std::shared_ptr<VirNetwork>> networks = conn.virConnectListAllNetworks();

        // 打印表头
        std::cout << std::setw(30) << std::left << "名称"
            << std::setw(36) << std::left << "UUID" << std::endl;
        std::cout << std::string(75, '-') << std::endl;

        // 打印每个网络的信息
        for ( const auto& network : networks ) {
            // int state = network->virNetworkGetState();

            std::cout << std::setw(30) << std::left << network->virNetworkGetName()
                << std::setw(36) << std::left << network->virNetworkGetUUID()
                // << std::setw(15) << std::left << getDomainStateString(state)
                << std::endl;
        }
    }
    else if ( command == "net-define-xml" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少XML文件路径\n";
            printUsage();
            return 1;
        }
        // 读取XML文件
        const char* xmlFile = argv[2];
        std::ifstream file(xmlFile);
        if ( !file.is_open() ) {
            std::cerr << "错误: 无法打开文件 '" << xmlFile << "'\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string xmlDesc = buffer.str();

        // 建立连接并定义网络
        try {
            VirConnect conn("qemu:///system");
            std::shared_ptr<VirNetwork> network = conn.virNetworkDefineXML(xmlDesc);
            std::cout << "网络 '" << network->virNetworkGetName() << "' 已定义\n";
        }
        catch ( const std::exception& e ) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    else if ( command == "net-start" ) {
        if ( argc < 3 ) {
            std::cerr << "错误: 缺少网络名参数\n";
            printUsage();
            return 1;
        }
    }
    else {
        std::cerr << "未知命令: " << command << std::endl;
        printUsage();
        return 1;
    }

    return 0;
}