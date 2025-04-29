#include <dirent.h>
#include <sstream>
#include "xen_driver.h"
#include "../virConnect.h"
#include "../util/createDir.h"
#include "../util/generate_uuid.h"
#include "../tinyxml/tinyxml2.h"
#include "log/log.h"

XenDriver::XenDriver() {
    configManager = ConfigManager::Instance();
    std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");
    createDirectoryIfNotExists(configDir);

    loadAllDomainConfigs();
}

void XenDriver::loadAllDomainConfigs() {
    // 获取配置目录
    std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");

    // 遍历配置目录下的所有XML文件
    DIR* dir = opendir(configDir.c_str());
    // std::cout << "Loading domain configurations from " << configDir << std::endl;
    LOG_INFO("Loading domain configurations from %s", configDir.c_str());
    if ( !dir ) {
        // std::cerr << "Failed to open domain config directory: " << configDir << std::endl;
        LOG_ERROR("Failed to open domain config directory: %s", configDir.c_str());
        return;
    }

    struct dirent* entry;
    while ( (entry = readdir(dir)) != nullptr ) {
        std::string filename = entry->d_name;
        // 仅处理.xml文件
        if ( filename.size() > 4 && filename.substr(filename.size() - 4) == ".xml" ) {
            std::string filePath = configDir + "/" + filename;
            try {
                // 读取XML文件内容
                std::string xmlDesc = readFileContent(filePath);

                // 解析XML创建domain对象
                this->domains.push_back(parseAndCreateDomainObj(xmlDesc));
            }
            catch ( const std::exception& e ) {
                // std::cerr << "Failed to load domain config " << filename << ": " << e.what() << std::endl;
                LOG_ERROR("Failed to load domain config %s: %s", filename.c_str(), e.what());
            }
        }
    }
    closedir(dir);

    // std::cout << "Loaded " << domains.size() << " domain configurations." << std::endl;
    LOG_INFO("Loaded %zu domain configurations.", domains.size());

    // 遍历虚拟机对象，查看是否存在对应的pid文件
    for ( const auto& domainObj : domains ) {
        std::string pidFilePath = configDir + "/" + domainObj->def->name + ".pid";
        std::ifstream pidFile(pidFilePath);
        if ( pidFile.is_open() ) {
            int pid;
            pidFile >> pid;
            pidFile.close();

            // 检查PID是否有效（进程是否存在）
            if ( kill(pid, 0) == 0 ) {
                // 进程存在，设置运行状态
                domainObj->pid = pid;
                domainObj->def->id = generateUniqueID(); // 生成唯一ID
                domainObj->stateReason.state = VIR_DOMAIN_RUNNING;
                LOG_INFO("Domain %s is running with PID: %d", domainObj->def->name.c_str(), pid);
            }
            else {
                // 进程不存在，删除过期的PID文件
                LOG_WARN("Domain %s has stale PID file (PID %d not running), cleaning up",
                    domainObj->def->name.c_str(), pid);
                remove(pidFilePath.c_str());
                domainObj->pid = -1;
                domainObj->stateReason.state = VIR_DOMAIN_SHUTOFF;
            }
        }
    }
}

std::string XenDriver::readFileContent(const std::string& filePath) const {
    std::ifstream file(filePath);
    if ( !file.is_open() ) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 解析XML字段，并创建domainOBJ对象
std::shared_ptr<xenDomainObj> XenDriver::parseAndCreateDomainObj(const std::string& xmlDesc) {
    using namespace tinyxml2;

    // 创建xenDomainDef对象
    std::shared_ptr<xenDomainDef> def = std::make_shared<xenDomainDef>();

    XMLDocument doc;
    XMLError err = doc.Parse(xmlDesc.c_str());
    if ( err != XML_SUCCESS ) {
        throw std::runtime_error("Failed to parse XML");
    }

    // 获取domain节点
    XMLElement* domainElem = doc.FirstChildElement("domain");
    if ( !domainElem ) {
        throw std::runtime_error("Failed to find element: domain");
    }

    // 解析domain的name属性
    XMLElement* nameElem = domainElem->FirstChildElement("name");
    if ( !nameElem || !nameElem->GetText() ) {
        throw std::runtime_error("Failed to find element: name");
    }
    std::string domainName = nameElem->GetText();

    // 解析UUID
    XMLElement* uuidElem = domainElem->FirstChildElement("uuid");
    std::string uuid;
    // std::string uuid = uuidElem && uuidElem->GetText() ? uuidElem->GetText() : "";
    if ( uuidElem && uuidElem->GetText() ) {
        uuid = uuidElem->GetText();
    }
    else {
        uuid = generateUUID();
        LOG_INFO("Not found UUID, generate a new one: %s", uuid.c_str());
        // 写入UUID到XML
        // 创建一个新的UUID节点
        XMLElement* newUuidElem = doc.NewElement("uuid");
        XMLText* uuidText = doc.NewText(uuid.c_str());
        newUuidElem->InsertEndChild(uuidText);

        // 将UUID节点添加到domain节点中，放在name节点之后
        if ( nameElem->NextSiblingElement() ) {
            domainElem->InsertAfterChild(nameElem, newUuidElem);
        }
        else {
            domainElem->InsertEndChild(newUuidElem);
        }

        // 将修改后的XML保存到原文件
        std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");
        std::string filePath = configDir + "/" + domainName + ".xml";
        doc.SaveFile(filePath.c_str());
        LOG_INFO("UUID written to XML file: %s", filePath.c_str());
    }

    // 解析内存
    int memoryMB = 0;   // 默认单位为MiB
    XMLElement* memElem = domainElem ? domainElem->FirstChildElement("memory") : nullptr;
    if ( memElem ) {
        const std::string unit = memElem->Attribute("unit") ? memElem->Attribute("unit") : "KiB";
        memElem->QueryIntText(&memoryMB);

        if ( unit == "KiB" ) {
            memoryMB /= 1024;
        }
        else if ( unit == "MiB" ) {
            // do nothing
        }
        else if ( unit == "GiB" ) {
            memoryMB *= 1024;
        }
        else if ( unit == "TiB" ) {
            memoryMB *= 1024 * 1024;
        }
        else {
            throw std::runtime_error("Unknown memory unit: " + unit);
        }
    }

    // 解析vcpus
    int vcpus = 1;
    XMLElement* vcpusElem = domainElem->FirstChildElement("vcpu");
    if ( vcpusElem ) {
        vcpusElem->QueryIntText(&vcpus);
    }

    // 更新解析磁盘部分的代码
    // 解析磁盘镜像
    std::string diskPath;
    XMLElement* devicesElem = domainElem ? domainElem->FirstChildElement("devices") : nullptr;
    if ( devicesElem ) {
        // 处理disk元素
        for ( XMLElement* diskNode = devicesElem->FirstChildElement("disk");
            diskNode;
            diskNode = diskNode->NextSiblingElement("disk") ) {

            // 检查设备类型
            const char* device = diskNode->Attribute("device");
            if ( device && std::string(device) == "disk" ) {
                XMLElement* sourceElem = diskNode->FirstChildElement("source");
                if ( sourceElem && sourceElem->Attribute("file") ) {
                    diskPath = sourceElem->Attribute("file");
                    break;  // 只取第一个磁盘
                }
            }
        }

        // 处理cdrom
        // XMLElement* cdromElem = devicesElem->FirstChildElement("cdrom");
        // 现有cdrom处理代码...
    }

    // 解析CDROM镜像
    std::string cdromPath;
    if ( devicesElem ) {
        XMLElement* cdromElem1 = devicesElem->FirstChildElement("cdrom");
        if ( cdromElem1 ) {
            XMLElement* sourceElem = cdromElem1->FirstChildElement("source");
            if ( sourceElem && sourceElem->Attribute("file") ) {
                cdromPath = sourceElem->Attribute("file");
            }
        }
    }

    // 解析网络接口
    if ( devicesElem ) {
        for ( XMLElement* ifaceElem = devicesElem->FirstChildElement("interface");
            ifaceElem;
            ifaceElem = ifaceElem->NextSiblingElement("interface") ) {

            NetworkInterfaceInfo netIface;

            // 获取接口类型
            const char* typeAttr = ifaceElem->Attribute("type");
            if ( typeAttr ) {
                netIface.type = typeAttr;
            }
            else {
                LOG_WARN("Interface without type attribute found, skipping");
                continue;
            }

            // 获取MAC地址
            XMLElement* macElem = ifaceElem->FirstChildElement("mac");
            if ( macElem && macElem->Attribute("address") ) {
                netIface.macAddress = macElem->Attribute("address");
            }

            // 获取模型类型
            XMLElement* modelElem = ifaceElem->FirstChildElement("model");
            if ( modelElem && modelElem->Attribute("type") ) {
                netIface.modelType = modelElem->Attribute("type");
            }
            else {
                // 默认使用e1000
                netIface.modelType = "e1000";
            }

            // 根据接口类型获取源
            XMLElement* sourceElem = ifaceElem->FirstChildElement("source");
            if ( sourceElem ) {
                if ( netIface.type == "bridge" && sourceElem->Attribute("bridge") ) {
                    netIface.source = sourceElem->Attribute("bridge");
                }
                else if ( netIface.type == "network" && sourceElem->Attribute("network") ) {
                    netIface.source = sourceElem->Attribute("network");
                }
            }

            // 获取目标设备名称
            XMLElement* targetElem = ifaceElem->FirstChildElement("target");
            if ( targetElem && targetElem->Attribute("dev") ) {
                netIface.target = targetElem->Attribute("dev");
            }

            // 将网络接口添加到列表
            def->networkInterfaces.push_back(netIface);
            LOG_INFO("Found network interface: type=%s, mac=%s, model=%s, source=%s",
                netIface.type.c_str(),
                netIface.macAddress.c_str(),
                netIface.modelType.c_str(),
                netIface.source.c_str());
        }

        // 添加处理<network>元素的代码
        for ( XMLElement* netElem = devicesElem->FirstChildElement("network");
            netElem;
            netElem = netElem->NextSiblingElement("network") ) {

            NetworkInterfaceInfo netIface;

            // 默认使用bridge类型
            netIface.type = "bridge";

            // 获取网络名称
            XMLElement* nameElem = netElem->FirstChildElement("name");
            if ( nameElem && nameElem->GetText() ) {
                // 记录网络名称，可用于日志
                std::string netName = nameElem->GetText();
                LOG_INFO("Found network: %s", netName.c_str());
            }

            // 查找bridge元素获取桥接名
            XMLElement* bridgeElem = netElem->FirstChildElement("bridge");
            if ( bridgeElem && bridgeElem->Attribute("name") ) {
                netIface.source = bridgeElem->Attribute("name");
            }

            // 设置默认网卡类型
            netIface.modelType = "e1000";

            // 如果有MAC地址则解析
            XMLElement* macElem = netElem->FirstChildElement("mac");
            if ( macElem && macElem->Attribute("address") ) {
                netIface.macAddress = macElem->Attribute("address");
            }

            // 将网络接口添加到列表
            if ( !netIface.source.empty() ) {
                def->networkInterfaces.push_back(netIface);
                LOG_INFO("Found network element with bridge: %s, mac: %s",
                    netIface.source.c_str(),
                    netIface.macAddress.empty() ? "auto" : netIface.macAddress.c_str());
            }
        }
    }

    def->name = domainName;
    def->uuid = uuid;
    def->id = -1;  // 未运行状态
    def->vcpus = vcpus;
    def->memory = memoryMB;
    def->xmlDesc = xmlDesc;
    def->diskPath = diskPath;
    def->cdromPath = cdromPath;

    // 创建virDomainObj对象
    std::shared_ptr<xenDomainObj> domainObj = std::make_shared<xenDomainObj>();
    domainObj->def = def;
    domainObj->pid = -1;  // 未运行状态
    domainObj->stateReason.state = 5;  // VIR_DOMAIN_SHUTOFF
    domainObj->stateReason.reason = 0;  // 正常关闭
    domainObj->persistent = 1;
    domainObj->autostart = 0;  // 默认不自动启动

    return domainObj;
}

int XenDriver::generateUniqueID() {
    static int uniqueID = 0;
    return ++uniqueID;
}

int XenDriver::processXenObject(std::shared_ptr<xenDomainObj> domainObj) {
    // 处理Xen对象的逻辑
    // 这里可以添加具体的处理代码
    return 0;
}

XenDriver::~XenDriver() {
    LOG_INFO("Xen Driver destroyed.");
}

std::vector<std::shared_ptr<VirDomain>> XenDriver::connectListAllDomains(unsigned int flags) const {
    if ( flags != 0 ) {
        throw std::runtime_error("Unsupported flags");
    }
    std::vector<std::shared_ptr<VirDomain>> ret;
    for ( const auto& domain : domains ) {
        ret.push_back(std::make_shared<VirDomain>(domain->def->name, domain->def->id, domain->def->uuid));
    }
    return ret;
}

std::shared_ptr<VirDomain> XenDriver::domainLookupByName(const std::string& name) const {
    for ( const auto& domain : domains ) {
        if ( domain->def->name == name ) {
            return std::make_shared<VirDomain>(domain->def->name, domain->def->id, domain->def->uuid);
        }
    }
    return nullptr;
}

std::shared_ptr<VirDomain> XenDriver::domainDefineXML(const std::string& xml) {
    return domainDefineXMLFlags(xml, 0);
}

std::shared_ptr<VirDomain> XenDriver::domainDefineXMLFlags(const std::string& xml, unsigned int flags) {
    // 创建VirDomain对象
    std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xml, this);

    // 解析XML并创建内部domainObj对象
    parseAndCreateDomainObj(xml);

    if ( flags == 0 ) {
        // 保存配置文件
        std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");
        std::string filePath = configDir + "/" + domain->virDomainGetName() + ".xml";
        std::ofstream file(filePath);
        if ( !file.is_open() ) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }
        file << xml;
    }

    return domain;
}

void XenDriver::domainCreate(std::shared_ptr<VirDomain> domain) {
    std::string name = domain->virDomainGetName();
    for ( const auto& domainObj : domains ) {
        if ( domainObj->def->name == name ) {
            processXenObject(domainObj);
            domain->virDomainSetID(domainObj->def->id);
            return;
        }
    }
}

std::shared_ptr<VirDomain> XenDriver::domainCreateXML(const std::string& xmlDesc) {
    std::shared_ptr<xenDomainObj> domainObj = std::make_shared<xenDomainObj>();
    domainObj = parseAndCreateDomainObj(xmlDesc);
    domains.push_back(domainObj);
    processXenObject(domainObj);
    return std::make_shared<VirDomain>(domainObj->def->name, domainObj->def->id, domainObj->def->uuid);
}

int XenDriver::domainAttachDevice(std::shared_ptr<VirDomain> domain, const std::string& xmlDesc, unsigned int flags) {
    // 实现设备附加逻辑
    LOG_INFO("Attaching device to domain %s", domain->virDomainGetName().c_str());
}

void XenDriver::domainDestroy(std::shared_ptr<VirDomain> domain) {
    if ( domain->virDomainGetID() < 0 ) {
        throw std::runtime_error("Domain " + domain->virDomainGetName() + " is not running.");
    }
    std::shared_ptr<xenDomainObj> domainObj;
    bool found = false;
    for ( const auto& domainObj_ : domains ) {
        if ( domainObj_->def->name == domain->virDomainGetName() ) {
            domainObj = domainObj_;
            found = true;
            break;
        }
    }
    if ( !found ) {
        throw std::runtime_error("Domain not found.");
    }

    if ( !found || !domainObj ) {
        LOG_INFO("found: %d", !found);
        LOG_INFO("domainObj: %d", !domainObj);
        return;
    }

    if ( kill(domainObj->pid, SIGKILL) < 0 ) {
        LOG_ERROR("Failed to kill domain process: %s", strerror(errno));
        return;
    }

    // Update domain state to reflect shutdown
    domainObj->stateReason.state = VIR_DOMAIN_SHUTOFF;
    domainObj->stateReason.reason = 1; // Destroyed
    domainObj->pid = -1; // Mark as not running

    LOG_INFO("Domain %s destroyed.", domainObj->def->name.c_str());

    // 删除pid文件
    std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");
    std::string pidFilePath = configDir + "/" + domainObj->def->name + ".pid";
    if ( remove(pidFilePath.c_str()) != 0 ) {
        LOG_ERROR("Failed to delete PID file: %s", pidFilePath.c_str());
    }

    return;
}

void XenDriver::domainShutdown(std::shared_ptr<VirDomain> domain) {
    if ( domain->virDomainGetID() < 0 ) {
        throw std::runtime_error("Domain " + domain->virDomainGetName() + " is not running.");
    }
    // TODO: 实现优雅关闭xen的具体逻辑

    return;
}

int XenDriver::domainUndefine(std::shared_ptr<VirDomain> domain) {
    return domainUndefineFlags(domain, 0);
}

int XenDriver::domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) {
    if ( flags != 0 ) {
        throw std::runtime_error("Unsupported flags");
    }
    std::string configDir = configManager->getValue("xen.config_dir", "./temp/xen");
    std::string filePath = configDir + "/" + domain->virDomainGetName() + ".xml";
    if ( remove(filePath.c_str()) != 0 ) {
        throw std::runtime_error("Failed to delete file: " + filePath);
    }
    return 0;
}

int XenDriver::domainGetState(std::shared_ptr<VirDomain> domain) {
    if ( domain->virDomainGetID() < 0 ) {
        return VIR_DOMAIN_SHUTOFF;
    }
    std::shared_ptr<xenDomainObj> domainObj;
    bool found = false;
    for ( const auto& domainObj_ : domains ) {
        if ( domainObj_->def->name == domain->virDomainGetName() ) {
            domainObj = domainObj_;
            found = true;
            break;
        }
    }
    if ( !found ) {
        throw std::runtime_error("Domain not found.");
    }

    // TODO: 实现获取虚拟机状态的逻辑

    return domainObj->stateReason.state;
}