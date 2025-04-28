#include "virDomain.h"
#include "./tinyxml/tinyxml2.h"

// aborted
VirDomain::VirDomain(const std::string& xmlDesc, HypervisorDriver* driver, unsigned int flags) {
    if ( flags != 0 ) {
        throw std::runtime_error("Unsupported flags");
    }
    this->driver = driver;
    // 解析XML描述，获取虚拟机的基本信息
    using namespace tinyxml2;
    XMLDocument doc;
    XMLError err = doc.Parse(xmlDesc.c_str());
    if ( err != XML_SUCCESS ) {
        throw std::runtime_error("Failed to parse XML");
    }

    XMLElement* domain = doc.FirstChildElement("domain");
    if ( !domain ) {
        throw std::runtime_error("Failed to find element: domain");
    }

    // 获取虚拟机名称
    XMLElement* nameElem = domain->FirstChildElement("name");
    if ( nameElem ) name = nameElem->GetText();

    // 获取虚拟机ID
    // XMLElement* idElem = domain->FirstChildElement("id");
    // if ( idElem ) idElem->QueryIntText(&id);

    // 获取虚拟机UUID
    XMLElement* uuidElem = domain->FirstChildElement("uuid");
    if ( uuidElem ) uuid = uuidElem->GetText();
}

int VirDomain::virDomainGetState(unsigned int& reason) const {
    // 调用驱动的接口获取虚拟机的状态
    // TODO: 返回正确的reason
    reason = 0;
    return driver->domainGetState(std::make_shared<VirDomain>(*this));
}

std::string VirDomain::virDomainGetName() const {
    return name;
}

int VirDomain::virDomainGetID() const {
    return id;
}

std::string VirDomain::virDomainGetUUID() const {
    return uuid;
}

int VirDomain::virDomainAttachDevice(const std::string& xmlDesc, unsigned int flags) {
    return driver->domainAttachDevice(std::make_shared<VirDomain>(*this), xmlDesc, flags);
}
