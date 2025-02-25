#include "qemu_driver.h"
#include "../virDomain.h"
#include "../tinyxml/tinyxml2.h"

QemuDriver::QemuDriver() {
    config = QemuDriverConfig();
    std::cout << "QEMU Driver initialized." << std::endl;
}

QemuDriver::~QemuDriver() {
    std::cout << "QEMU Driver destroyed." << std::endl;
}

std::shared_ptr<VirDomain> QemuDriver::domainDefineXML(const std::string& xml) {
    return domainDefineXMLFlags(xml, 0);
}

std::shared_ptr<VirDomain> QemuDriver::domainDefineXMLFlags(const std::string& xml, unsigned int flags) {
    // TODO: 先不写处理flags的逻辑
    std::shared_ptr<VirDomain> domain = std::make_shared<VirDomain>(xml, this);

    std::string filePath = config.getConfigDir() + "/" + domain->virDomainGetName() + ".xml";
    std::ofstream file(filePath);
    if ( !file.is_open() ) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    file << xml;

    return domain;
}

void QemuDriver::domainCreate(std::shared_ptr<VirDomain> domain) {
    std::string name = domain->virDomainGetName();
    std::string xmlFileName = "./temp/domains/" + name + ".xml";
    std::string xmlDesc;
    try {
        std::ifstream file(xmlFileName);
        if ( !file ) {
            throw std::runtime_error("Failed to open file: " + xmlFileName);
        }
        std::string line;
        while ( std::getline(file, line) ) {
            xmlDesc += line;
        }
    }
    catch ( const std::exception& e ) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    domainCreateXML(xmlDesc);
    return;
}

void QemuDriver::domainCreateXML(const std::string& xmlDesc) {
    using namespace tinyxml2;
    XMLDocument doc;
    XMLError err = doc.Parse(xmlDesc.c_str());
    if ( err != XML_SUCCESS ) {
        throw std::runtime_error("Failed to parse XML");
    }

    // 获取domain节点
    XMLElement* domain = doc.FirstChildElement("domain");
    if ( !domain ) {
        // std::cerr << "未找到domain元素" << std::endl;
        throw std::runtime_error("Failed to find element: domain");
    }

    // 解析内存
    int memoryMB = 0;
    XMLElement* memElem = domain ? domain->FirstChildElement("memory") : nullptr;
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

    // 解析vCPU数量
    int vcpus = 1;
    XMLElement* vcpuElem = domain ? domain->FirstChildElement("vcpu") : nullptr;
    if ( vcpuElem ) {
        vcpuElem->QueryIntText(&vcpus);
    }

    // 解析磁盘镜像
    std::string diskPath;
    XMLElement* devicesElem = domain ? domain->FirstChildElement("devices") : nullptr;
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
    XMLElement* featuresElem = domain ? domain->FirstChildElement("features") : nullptr;
    bool enableKVM = (featuresElem && featuresElem->FirstChildElement("kvm"));

    // 解析qemu:commandline中的QMP和monitor
    std::string qmpParam, monitorParam;
    XMLElement* cmdline = domain->FirstChildElement("qemu:commandline");
    for ( XMLElement* arg = cmdline ? cmdline->FirstChildElement("qemu:arg") : nullptr;
        arg; arg = arg->NextSiblingElement("qemu:arg") ) {
        const char* val = arg->Attribute("value");
        if ( val ) {
            if ( std::string(val) == "-qmp" ) {
                arg = arg->NextSiblingElement("qemu:arg");
                if ( arg && arg->Attribute("value") ) qmpParam = std::string("-qmp ") + arg->Attribute("value");
            }
            else if ( std::string(val) == "-monitor" ) {
                arg = arg->NextSiblingElement("qemu:arg");
                if ( arg && arg->Attribute("value") ) monitorParam = std::string("-monitor ") + arg->Attribute("value");
            }
        }
    }

    // 根据解析结果拼接命令
    std::string cmd = "qemu-system-x86_64\\\n";
    cmd += " -m " + std::to_string(memoryMB) + "\\\n";
    cmd += " -smp " + std::to_string(vcpus) + "\\\n";
    if ( !diskPath.empty() )  cmd += " -hda " + diskPath + "\\\n";
    if ( !cdromPath.empty() ) cmd += " -cdrom " + cdromPath + "\\\n";
    // 假设以cdrom优先启动
    cmd += " -boot d\\\n";
    if ( enableKVM ) cmd += " -enable-kvm\\\n";
    if ( !monitorParam.empty() ) cmd += " " + monitorParam + "\\\n";
    if ( !qmpParam.empty() )     cmd += " " + qmpParam + "\\\n";

    cmd += "\n";

    // 执行QEMU指令，启动QEMU虚拟机
    std::cout << "Execute command: \n" << cmd << std::endl;

    // 创建一个子进程并执行
    pid_t pid = fork();
    if ( pid == -1 ) {
        std::cerr << "Failed to fork." << std::endl;
        return;
    }
    if ( pid == 0 ) {
        // 子进程
        system(cmd.c_str());
        exit(0);
    }
    else {
        // 父进程
        int status;
        std::cout << "Waiting for child process to finish..." << std::endl;
        waitpid(pid, &status, 0);
    }

    return;
}

void QemuDriver::domainDestroy(std::shared_ptr<VirDomain> domain) {
    return;
}

int QemuDriver::domainUndefine(std::shared_ptr<VirDomain> domain) {
    return domainUndefineFlags(domain, 0);
}

int QemuDriver::domainUndefineFlags(std::shared_ptr<VirDomain> domain, unsigned int flags) {
    std::string filePath = config.getConfigDir() + "/" + domain->virDomainGetName() + ".xml";
    if ( remove(filePath.c_str()) != 0 ) {
        throw std::runtime_error("Failed to delete file: " + filePath);
    }
    return 0;
}

int QemuDriver::domainGetState(std::shared_ptr<VirDomain> domain) {
    return 0;
}