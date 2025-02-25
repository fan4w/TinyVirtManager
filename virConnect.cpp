#include "virConnect.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>


VirConnect::VirConnect(const std::string& uri, unsigned int flags) : uri(uri) {
    // 根据URI创建对应的Driver
    driver = DriverFactory::createDriver(uri);

    // 检查配置文件目录是否存在，不存在则创建
    struct stat info;

    if ( stat(pathToConfigDir.c_str(), &info) != 0 ) {
        // Directory does not exist, create it
        if ( mkdir(pathToConfigDir.c_str(), 0755) != 0 ) {
            throw std::runtime_error("Failed to create directory: " + pathToConfigDir);
        }
    }
    else if ( !(info.st_mode & S_IFDIR) ) {
        // Path exists but is not a directory
        throw std::runtime_error(pathToConfigDir + " exists but is not a directory.");
    }

    // 遍历目录，加载所有虚拟机配置文件
    std::vector<std::string> vm_files;
    DIR* dir = opendir(pathToConfigDir.c_str());
    if ( dir == nullptr ) {
        throw std::runtime_error("Failed to open directory: " + pathToConfigDir);
    }

    struct dirent* entry;
    while ( (entry = readdir(dir)) != nullptr ) {
        std::string filename = entry->d_name;
        if ( filename.size() > 4 && filename.substr(filename.size() - 4) == ".xml" ) {
            vm_files.push_back(filename);
        }
    }
    closedir(dir);

    // 读取每个XML文件并解析，存入虚拟机映射表中
    for ( const auto& file : vm_files ) {
        std::string filePath = pathToConfigDir + "/" + file;
        std::ifstream in(filePath);
        if ( !in.is_open() ) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }
        // 读取文件内容
        std::string xmlDesc((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        // 创建虚拟机对象
        std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xmlDesc, driver.get(), flags);
        domains.push_back(domain);
    }
}


std::shared_ptr<VirDomain> VirConnect::virDomainDefineXML(const std::string& xmlDesc) {
    std::shared_ptr<VirDomain> domain = driver->domainDefineXML(xmlDesc);

    return domain;
}

void VirConnect::virDomainUndefine(const std::shared_ptr<VirDomain> domain) {
    // 删除虚拟机对象
    for ( auto it = domains.begin(); it != domains.end(); ++it ) {
        if ( *it == domain ) {
            domains.erase(it);
            break;
        }
    }

    // driver停止并删除虚拟机
    driver->domainUndefine(domain);
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByName(const std::string& name) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetName() == name ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with name " + name + " does not exist.");
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByID(const int& id) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetID() == id ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with ID " + std::to_string(id) + " does not exist.");
}

std::shared_ptr<VirDomain> VirConnect::virDomainLookupByUUID(const std::string& uuid) const {
    for ( const auto& domain : domains ) {
        if ( domain->virDomainGetUUID() == uuid ) {
            return domain;
        }
    }
    throw std::runtime_error("Domain with UUID " + uuid + " does not exist.");
}

std::vector<std::shared_ptr<VirDomain>> VirConnect::virConnectListAllDomains(unsigned int flags) const {
    if ( flags == 0 ) {
        return domains;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virConnectListAllDomains.");
    }
}

std::shared_ptr<VirDomain> VirConnect::virDomainCreateXML(const std::string& xmlDesc, unsigned int flags) {
    if ( flags == 0 ) {
        std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xmlDesc, driver.get());
        // 调用驱动的方法启动虚拟机
        driver->domainCreateXML(xmlDesc);
        domains.push_back(domain);
        return domain;
    }
    else {
        throw std::invalid_argument("Unsupported flags for virDomainCreateXML.");
    }
}

void VirConnect::virDomainCreate(const std::shared_ptr<VirDomain> domain, unsigned int flags) {
    if ( flags == 0 ) {
        // 调用驱动的方法启动虚拟机
        driver->domainCreate(domain);
        return;
    }
}

void VirConnect::virDomainDestroy(const std::shared_ptr<VirDomain> domain) {
    // TODO: 调用驱动的方法关闭虚拟机
    driver->domainDestroy(domain);
    return;
}