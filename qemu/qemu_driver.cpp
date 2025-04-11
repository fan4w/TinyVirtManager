#include "qemu_driver.h"
#include "../virDomain.h"
#include "../virConnect.h"
#include "../tinyxml/tinyxml2.h"
#include "../util/generate_uuid.h"
#include <dirent.h>
#include <memory>
#include <map>
#include <sys/stat.h>
#include <fcntl.h>

QemuDriver::QemuDriver() {
    config = QemuDriverConfig();
    domains = std::vector<std::shared_ptr<qemuDomainObj>>();
    // 加载所有虚拟机配置文件
    loadAllDomainConfigs();
}

void QemuDriver::loadAllDomainConfigs() {
    // 获取配置目录
    std::string configDir = config.getConfigDir();

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
        std::string pidFilePath = config.getConfigDir() + "/" + domainObj->def->name + ".pid";
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

std::string QemuDriver::readFileContent(const std::string& filePath) const {
    std::ifstream file(filePath);
    if ( !file ) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return content;
}

// 解析XML字段，并创建domainOBJ对象
std::shared_ptr<qemuDomainObj> QemuDriver::parseAndCreateDomainObj(const std::string& xmlDesc) {
    using namespace tinyxml2;

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
        std::string filePath = config.getConfigDir() + "/" + domainName + ".xml";
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

    // 解析磁盘镜像
    std::string diskPath;
    XMLElement* devicesElem = domainElem ? domainElem->FirstChildElement("devices") : nullptr;
    XMLElement* diskElem = nullptr;
    if ( devicesElem ) {
        XMLElement* diskNode = devicesElem->FirstChildElement("disk");
        if ( diskNode ) {
            diskElem = diskNode->FirstChildElement("source");
        }
    }
    if ( diskElem && diskElem->Attribute("file") ) {
        diskPath = diskElem->Attribute("file");
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

    // 判断是否启用KVM
    XMLElement* featuresElem = domainElem->FirstChildElement("features");

    // 创建qemuDomainDef对象
    std::shared_ptr<qemuDomainDef> def = std::make_shared<qemuDomainDef>();
    def->name = domainName;
    def->uuid = uuid;
    def->id = -1;  // 未运行状态
    def->vcpus = vcpus;
    def->memory = memoryMB;
    def->xmlDesc = xmlDesc;
    def->diskPath = diskPath;
    def->cdromPath = cdromPath;
    def->enableKVM = (featuresElem && featuresElem->FirstChildElement("kvm"));

    std::string qmpSocketPath;
    // 使用默认路径
    if ( qmpSocketPath.empty() ) {
        qmpSocketPath = config.getQmpSocketDir() + "/" + domainName + ".sock";
    }

    def->qmpSocketPath = qmpSocketPath;

    // 创建virDomainObj对象
    std::shared_ptr<qemuDomainObj> domainObj = std::make_shared<qemuDomainObj>();
    domainObj->def = def;
    domainObj->pid = -1;  // 未运行状态
    domainObj->stateReason.state = 5;  // VIR_DOMAIN_SHUTOFF
    domainObj->stateReason.reason = 0;  // 正常关闭
    domainObj->persistent = 1;
    domainObj->autostart = 0;  // 默认不自动启动


    return domainObj;

    // 连接QemuMonitor（如果虚拟机正在运行）
    // 注意：这里不应该立即连接，而是在需要时才连接
    // 所以不创建QemuMonitor对象，只记录路径
}

int QemuDriver::generateUniqueID() {
    static int idCounter = 0;
    return idCounter++;
}

int QemuDriver::processQemuObject(std::shared_ptr<qemuDomainObj> domainObj) {
    // 检查虚拟机状态
    if ( domainObj->pid != -1 ) {
        throw std::runtime_error("Domain " + domainObj->def->name + " is already running.");
    }

    std::shared_ptr<qemuDomainDef> qemuDef = std::dynamic_pointer_cast< qemuDomainDef >(domainObj->def);
    if ( !qemuDef ) {
        throw std::runtime_error("Failed to cast domain definition to QEMU definition");
    }

    // 准备命令行参数数组
    std::vector<std::string> args;
    args.push_back(config.getQemuEmulator());
    args.push_back("-name");
    args.push_back(qemuDef->name);
    args.push_back("-m");
    args.push_back(std::to_string(qemuDef->memory));
    args.push_back("-smp");
    args.push_back(std::to_string(qemuDef->vcpus));

    if ( !qemuDef->diskPath.empty() ) {
        args.push_back("-hda");
        args.push_back(qemuDef->diskPath);
    }

    if ( !qemuDef->cdromPath.empty() ) {
        args.push_back("-cdrom");
        args.push_back(qemuDef->cdromPath);
    }

    args.push_back("-boot");
    args.push_back("d");

    if ( qemuDef->enableKVM ) {
        args.push_back("-enable-kvm");
    }

    // QMP 监控
    if ( !qemuDef->qmpSocketPath.empty() ) {
        // 确保套接字目录存在
        std::string socketDir = qemuDef->qmpSocketPath.substr(0, qemuDef->qmpSocketPath.find_last_of('/'));
        struct stat st;
        if ( stat(socketDir.c_str(), &st) == -1 ) {
            mkdir(socketDir.c_str(), 0700);
        }
        args.push_back("-qmp");
        args.push_back("unix:" + qemuDef->qmpSocketPath + ",server,nowait");
    }

    // 将参数转换为 char* 数组用于 execv
    std::vector<char*> execArgs;
    for ( const auto& arg : args ) {
        execArgs.push_back(const_cast< char* >(arg.c_str()));
    }
    execArgs.push_back(nullptr); // execv 需要 NULL 结尾

    // 记录即将执行的完整命令（调试用）
    // std::cout << "Executing QEMU command: ";
    std::string command;
    for ( const auto& arg : args ) {
        // std::cout << arg << " ";
        command += arg + " ";
    }
    // std::cout << std::endl;
    LOG_INFO("Executing QEMU command: %s", command.c_str());

    // 创建子进程启动 QEMU
    pid_t pid = fork();
    if ( pid == -1 ) {
        // std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
        LOG_ERROR("Failed to fork: %s", strerror(errno));
        return -1;
    }

    if ( pid == 0 ) {
        // 子进程
        // 设置进程组
        setpgid(0, 0);

        // 重定向标准输出和错误输出到日志文件
        std::string logPath = config.getLogDir() + "/" + qemuDef->name + ".log";
        int fd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if ( fd != -1 ) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // 执行 QEMU 二进制文件
        execv(config.getQemuEmulator().c_str(), execArgs.data());

        // 如果 execv 失败，记录错误并退出
        // std::cerr << "Failed to execute QEMU: " << strerror(errno) << std::endl;
        LOG_ERROR("Failed to execute QEMU: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    else {
        // 父进程
        // 更新域对象状态
        domainObj->pid = pid;
        domainObj->stateReason.state = VIR_DOMAIN_RUNNING;
        domainObj->stateReason.reason = 0;
        domainObj->def->id = generateUniqueID(); // 生成唯一ID
    }

    // std::cout << "Domain: " << domainObj->def->name << " started with PID: " << domainObj->pid << std::endl;
    LOG_INFO("Domain: %s started with PID: %d", domainObj->def->name.c_str(), domainObj->pid);

    // 把PID存储到[domainName].pid文件中
    std::string pidFilePath = config.getConfigDir() + "/" + qemuDef->name + ".pid";
    std::ofstream pidFile(pidFilePath);
    if ( !pidFile.is_open() ) {
        // std::cerr << "Failed to open PID file: " << pidFilePath << std::endl;
        LOG_ERROR("Failed to open PID file: %s", pidFilePath.c_str());
        return -1;
    }
    pidFile << pid;
    pidFile.close();

    return 0;
}

QemuDriver::~QemuDriver() {
    // std::cout << "QEMU Driver destroyed." << std::endl;
    LOG_INFO("QEMU Driver destroyed.");
}

std::vector<std::shared_ptr<VirDomain>> QemuDriver::connectListAllDomains(unsigned int flags) const {
    if ( flags != 0 ) {
        throw std::runtime_error("Unsupported flags");
    }
    std::vector<std::shared_ptr<VirDomain>> ret;
    for ( const auto& domain : domains ) {
        ret.push_back(std::make_shared<VirDomain>(domain->def->name, domain->def->id, domain->def->uuid));
    }
    return ret;
}

std::shared_ptr<VirDomain> QemuDriver::domainLookupByName(const std::string& name) const {
    for ( const auto& domain : domains ) {
        if ( domain->def->name == name ) {
            return std::make_shared<VirDomain>(domain->def->name, domain->def->id, domain->def->uuid);
        }
    }
    return nullptr;
}

std::shared_ptr<VirDomain> QemuDriver::domainDefineXML(const std::string& xml) {
    return domainDefineXMLFlags(xml, 0);
}

std::shared_ptr<VirDomain> QemuDriver::domainDefineXMLFlags(const std::string& xml, unsigned int flags) {
    // 创建VirDomain对象
    std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xml, this);

    // 解析XML并创建内部domainObj对象
    parseAndCreateDomainObj(xml);

    if ( flags == 0 ) {
        // 保存配置文件
        std::string filePath = config.getConfigDir() + "/" + domain->virDomainGetName() + ".xml";
        std::ofstream file(filePath);
        if ( !file.is_open() ) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }
        file << xml;
    }

    return domain;
}

void QemuDriver::domainCreate(std::shared_ptr<VirDomain> domain) {
    std::string name = domain->virDomainGetName();
    for ( const auto& domainObj : domains ) {
        if ( domainObj->def->name == name ) {
            processQemuObject(domainObj);
            return;
        }
    }
    return;
}

std::shared_ptr<VirDomain> QemuDriver::domainCreateXML(const std::string& xmlDesc) {
    std::shared_ptr<qemuDomainObj> domainObj = std::make_shared<qemuDomainObj>();
    domainObj = parseAndCreateDomainObj(xmlDesc);
    domains.push_back(domainObj);
    processQemuObject(domainObj);
    return std::make_shared<VirDomain>(domainObj->def->name, domainObj->def->id, domainObj->def->uuid);
}

void QemuDriver::domainDestroy(std::shared_ptr<VirDomain> domain) {
    if ( domain->virDomainGetID() < 0 ) {
        throw std::runtime_error("Domain " + domain->virDomainGetName() + " is not running.");
    }
    std::shared_ptr<qemuDomainObj> domainObj;
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
    // std::shared_ptr<qemuDomainDef> qemuDef = std::dynamic_pointer_cast< qemuDomainDef >(domainObj->def);
    // domainObj->monitor = std::make_shared<QemuMonitor>(qemuDef->qmpSocketPath);

    if ( !found || !domainObj ) {
        // std::cout << "found: " << (!found) << std::endl;
        LOG_INFO("found: %d", !found);
        // std::cout << "domainObj: " << (!domainObj) << std::endl;
        LOG_INFO("domainObj: %d", !domainObj);
        // std::cout << "domainMon: " << (!domainObj->monitor) << std::endl;
        return;
    }

    // std::cout<<"pid: " << domainObj->pid << std::endl;
    // Send SIGKILL to forcefully terminate the QEMU process
    if ( kill(domainObj->pid, SIGKILL) < 0 ) {
        // std::cerr << "Failed to kill domain process: " << strerror(errno) << std::endl;
        LOG_ERROR("Failed to kill domain process: %s", strerror(errno));
        return;
    }

    // Update domain state to reflect shutdown
    domainObj->stateReason.state = VIR_DOMAIN_SHUTOFF;
    domainObj->stateReason.reason = 1; // Destroyed
    domainObj->pid = -1; // Mark as not running

    // std::cout << "Domain " << domainObj->def->name << " destroyed." << std::endl;
    LOG_INFO("Domain %s destroyed.", domainObj->def->name.c_str());

    // 删除pid文件
    std::string pidFilePath = config.getConfigDir() + "/" + domainObj->def->name + ".pid";
    if ( remove(pidFilePath.c_str()) != 0 ) {
        // std::cerr << "Failed to delete PID file: " << pidFilePath << std::endl;
        LOG_ERROR("Failed to delete PID file: %s", pidFilePath.c_str());
    }

    return;
}

void QemuDriver::domainShutdown(std::shared_ptr<VirDomain> domain) {
    // TODO: add ID and finish it
    if ( domain->virDomainGetID() < 0 ) {
        throw std::runtime_error("Domain " + domain->virDomainGetName() + " is not running.");
    }
    std::shared_ptr<qemuDomainObj> domainObj;
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
    std::shared_ptr<qemuDomainDef> qemuDef = std::dynamic_pointer_cast< qemuDomainDef >(domainObj->def);
    domainObj->monitor = std::make_shared<QemuMonitor>(qemuDef->qmpSocketPath);

    if ( !found || !domainObj || !domainObj->monitor ) {
        // std::cout << "found: " << (!found) << std::endl;
        LOG_INFO("found: %d", !found);
        // std::cout << "domainObj: " << (!domainObj) << std::endl;
        LOG_INFO("domainObj: %d", !domainObj);
        // std::cout << "domainMon: " << (!domainObj->monitor) << std::endl;
        LOG_INFO("domainMon: %d", !domainObj->monitor);
        return;
    }

    std::string cmd = "{ \"execute\":\"quit\"}";
    std::string result;

    // std::cout << "first send..." << cmd << std::endl;
    LOG_INFO("first send...%s", cmd.c_str());
    if ( domainObj->monitor->qemuMonitorSendMessage(cmd, result) < 0 ) {
        return;
    }

    domainObj->stateReason.state = VIR_DOMAIN_SHUTOFF;
    domainObj->stateReason.reason = 1; // Destroyed
    domainObj->pid = -1; // Mark as not running

    // std::cout << "Domain " << domainObj->def->name << " shutdown." << std::endl;
    LOG_INFO("Domain %s shutdown.", domainObj->def->name.c_str());

    // 删除pid文件
    std::string pidFilePath = config.getConfigDir() + "/" + domainObj->def->name + ".pid";
    if ( remove(pidFilePath.c_str()) != 0 ) {
        // std::cerr << "Failed to delete PID file: " << pidFilePath << std::endl;
        LOG_ERROR("Failed to delete PID file: %s", pidFilePath.c_str());
    }

    return;
}

int QemuDriver::domainUndefine(std::shared_ptr<VirDomain> domain) {
    return domainUndefineFlags(domain, 0);
}

int QemuDriver::domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) {
    if ( flags != 0 ) {
        throw std::runtime_error("Unsupported flags");
    }
    std::string filePath = config.getConfigDir() + "/" + domain->virDomainGetName() + ".xml";
    if ( remove(filePath.c_str()) != 0 ) {
        throw std::runtime_error("Failed to delete file: " + filePath);
    }
    return 0;
}

int QemuDriver::domainGetState(std::shared_ptr<VirDomain> domain) {
    if ( domain->virDomainGetID() < 0 ) {
        return VIR_DOMAIN_SHUTOFF;
    }
    std::shared_ptr<qemuDomainObj> domainObj;
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
    std::shared_ptr<qemuDomainDef> qemuDef = std::dynamic_pointer_cast< qemuDomainDef >(domainObj->def);
    domainObj->monitor = std::make_shared<QemuMonitor>(qemuDef->qmpSocketPath);

    if ( !found || !domainObj || !domainObj->monitor ) {
        // std::cout << "found: " << (!found) << std::endl;
        LOG_INFO("found: %d", !found);
        // std::cout << "domainObj: " << (!domainObj) << std::endl;
        LOG_INFO("domainObj: %d", !domainObj);
        // std::cout << "domainMon: " << (!domainObj->monitor) << std::endl;
        LOG_INFO("domainMon: %d", !domainObj->monitor);
        return VIR_DOMAIN_NOSTATE;  // 返回无状态而不是崩溃
    }

    // if ( domainObj->monitor->qemuMonitorOpenUnixSocket() < 0 ) {
    //     throw std::runtime_error("Failed to open monitor socket.");
    // }

    std::string cmd = "{ \"execute\":\"query-status\"}";
    std::string cmdQuit = "{ \"execute\":\"quit\"}";
    std::string result;

    // std::cout << "first send..." << cmd << std::endl;
    LOG_INFO("first send...%s", cmd.c_str());
    if ( domainObj->monitor->qemuMonitorSendMessage(cmd, result) < 0 ) {
        return -1;
    }

    // 解析JSON响应获取状态
    if ( result.find("\"status\": \"running\"") != std::string::npos ) {
        domainObj->stateReason.state = VIR_DOMAIN_RUNNING;
    }
    else if ( result.find("\"status\": \"paused\"") != std::string::npos ) {
        domainObj->stateReason.state = VIR_DOMAIN_PAUSED;
    }
    // 其他状态...

    return domainObj->stateReason.state;
}