#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include "driver-network.h"
#include "virNetwork.h"
#include "./log/log.h"
#include "./conf/config_manager.h"
#include "./util/createDir.h"
#include "./util/generate_uuid.h"
#include "./tinyxml/tinyxml2.h"

NetworkDriver::NetworkDriver() {
    auto configManager = ConfigManager::Instance();
    configDir = configManager->getValue("network.config_dir", "./temp/networks");
    createDirectoryIfNotExists(configDir);

    loadAllNetworkConfigs();
}

std::vector<std::shared_ptr<VirNetwork>> NetworkDriver::connectListAllNetworks(unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Network driver does not support flags");
        return {};
    }
    std::vector<std::shared_ptr<VirNetwork>> networkList;

    for ( const auto& network : networks ) {
        networkList.push_back(std::make_shared<VirNetwork>(network->name, network->uuid));
    }

    return networkList;
}

std::shared_ptr<VirNetwork> NetworkDriver::networkLookupByName(const std::string& name) {
    for ( const auto& network : networks ) {
        if ( network->name == name ) {
            return std::make_shared<VirNetwork>(network->name, network->uuid);
        }
    }
    LOG_WARN("Network %s not found: %s", name.c_str());
    return nullptr;
}

std::shared_ptr<VirNetwork> NetworkDriver::networkLookupByUUID(const std::string& uuid) {
    for ( const auto& network : networks ) {
        if ( network->uuid == uuid ) {
            return std::make_shared<VirNetwork>(network->name, network->uuid);
        }
    }
    LOG_WARN("Network %s not found: %s", uuid.c_str());
    return nullptr;
}

std::shared_ptr<VirNetwork> NetworkDriver::networkDefineXML(const std::string& xml, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for network definition: %u", flags);
        return {};
    }
    try {
        auto networkObj = parseAndCreateNetworkObj(xml);
        networks.push_back(networkObj);
        return std::make_shared<VirNetwork>(networkObj->name, networkObj->uuid);
    }
    catch ( const std::exception& e ) {
        LOG_ERROR("Failed to define network: %s", e.what());
        return nullptr;
    }
}

std::string NetworkDriver::netWorkGetXMLDesc(std::shared_ptr<VirNetwork> network, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for network XML description: %u", flags);
        return {};
    }
    for ( const auto& network_ : networks ) {
        if ( network_->uuid == network->virNetworkGetUUID() ) {
            return network_->xmlDesc;
        }
    }
    LOG_WARN("Network %s not found: %s", network->virNetworkGetName().c_str(), network->virNetworkGetUUID().c_str());
    return {};
}

void NetworkDriver::networkCreate(std::shared_ptr<VirNetwork> network) {
    if ( !network ) {
        LOG_ERROR("Attempt to create null network");
        return;
    }

    std::string networkName = network->virNetworkGetName();
    std::string networkUUID = network->virNetworkGetUUID();

    // 查找对应的网络配置对象
    std::shared_ptr<networkObj> netObj = nullptr;
    for ( const auto& obj : networks ) {
        if ( obj->name == networkName && obj->uuid == networkUUID ) {
            netObj = obj;
            break;
        }
    }

    if ( !netObj ) {
        LOG_ERROR("Network %s not found in configuration", networkName.c_str());
        return;
    }

    LOG_INFO("Creating network: %s (UUID: %s)", networkName.c_str(), networkUUID.c_str());

    // 根据网络转发模式执行不同的操作
    if ( netObj->forward == BRIDGE ) {
        // 桥接模式：创建TAP设备并连接到网桥
        std::string tapName = "tap_" + networkName;

        // 创建TAP设备
        std::string cmd = "ip tuntap add dev " + tapName + " mode tap";
        int ret = system(cmd.c_str());
        LOG_INFO("Executing command: %s", cmd.c_str());
        if ( ret != 0 ) {
            LOG_ERROR("Failed to create TAP device %s: %d", tapName.c_str(), ret);
            return;
        }

        // 将TAP设备连接到网桥
        cmd = "ip link set " + tapName + " master " + netObj->bridgeName;
        ret = system(cmd.c_str());
        LOG_INFO("Executing command: %s", cmd.c_str());
        if ( ret != 0 ) {
            LOG_ERROR("Failed to connect TAP device %s to bridge %s: %d",
                tapName.c_str(), netObj->bridgeName.c_str(), ret);
            // 清理已创建的TAP设备
            system(("ip link delete " + tapName).c_str());
            return;
        }

        // 设置TAP设备为启用状态
        cmd = "ip link set " + tapName + " up";
        ret = system(cmd.c_str());
        LOG_INFO("Executing command: %s", cmd.c_str());
        if ( ret != 0 ) {
            LOG_ERROR("Failed to set TAP device %s up: %d", tapName.c_str(), ret);
            // 清理已创建的TAP设备
            system(("ip link delete " + tapName).c_str());
            return;
        }

        LOG_INFO("Network %s created successfully with TAP device %s connected to bridge %s",
            networkName.c_str(), tapName.c_str(), netObj->bridgeName.c_str());

        // 保存TAP设备名称，以便后续可能需要的清理操作
        netObj->tapDeviceName = tapName;
    }
    else if ( netObj->forward == NAT ) {
        // NAT模式的实现将在后续扩展
        LOG_ERROR("NAT mode not implemented yet for network %s", networkName.c_str());
        return;
    }
    else {
        LOG_ERROR("Unknown forward mode for network %s", networkName.c_str());
        return;
    }
}

void NetworkDriver::loadAllNetworkConfigs() {
    DIR* dir = opendir(configDir.c_str());
    if ( !dir ) {
        LOG_ERROR("Failed to open network config directory: %s", configDir.c_str());
        return;
    }

    struct dirent* entry;
    while ( (entry = readdir(dir)) != nullptr ) {
        std::string filename = entry->d_name;
        if ( entry->d_type == DT_REG ) { // 仅处理常规文件
            std::string fileName = entry->d_name;
            if ( fileName.substr(fileName.find_last_of('.') + 1) == "xml" ) {
                std::string filePath = configDir + "/" + fileName;
                std::ifstream file(filePath);
                if ( !file.is_open() ) {
                    LOG_ERROR("Failed to open file: %s", filePath.c_str());
                    continue;
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string xmlDesc = buffer.str();
                auto networkObj = parseAndCreateNetworkObj(xmlDesc);
                networks.push_back(networkObj);
            }
        }
    }
    closedir(dir);

    LOG_INFO("Loaded %zu network configurations.", networks.size());
}

std::shared_ptr<networkObj> NetworkDriver::parseAndCreateNetworkObj(const std::string& xmlDesc) {
    auto network = std::make_shared<networkObj>();
    using namespace tinyxml2;
    XMLDocument doc;
    if ( doc.Parse(xmlDesc.c_str()) != XML_SUCCESS ) {
        throw std::runtime_error("Failed to parse network XML description");
    }

    // 获取根元素
    XMLElement* rootElem = doc.RootElement();
    if ( !rootElem ) {
        throw std::runtime_error("Invalid XML structure: no root elem");
    }

    // 解析网络名称
    XMLElement* nameElem = rootElem->FirstChildElement("name");
    if ( !nameElem || !nameElem->GetText() ) {
        throw std::runtime_error("Pool name not specified");
    }
    std::string name = nameElem->GetText();

    XMLElement* uuidElem = rootElem->FirstChildElement("uuid");
    std::string uuid;
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
        // 将UUID节点添加到pool节点中，放在name节点之后
        if ( nameElem->NextSiblingElement() ) {
            rootElem->InsertAfterChild(nameElem, newUuidElem);
        }
        else {
            rootElem->InsertEndChild(newUuidElem);
        }

        // 保存修改后的XML
        std::string filePath = configDir + "/" + name + ".xml";
        doc.SaveFile(filePath.c_str());
        LOG_INFO("UUID written to XML file: %s", filePath.c_str());
    }

    // 解析转发模式
    XMLElement* forwardElem = rootElem->FirstChildElement("forward");
    if ( !forwardElem ) {
        throw std::runtime_error("Forward element not specified");
    }
    const char* modeAttr = forwardElem->Attribute("mode");
    if ( !modeAttr ) {
        throw std::runtime_error("Forward mode attribute not specified");
    }
    std::string forward = modeAttr;
    if ( forward != "nat" && forward != "bridge" ) {
        throw std::runtime_error("Invalid forward mode: " + forward);
    }

    // 解析网桥名称
    XMLElement* bridgeElem = rootElem->FirstChildElement("bridge");
    if ( !bridgeElem ) {
        throw std::runtime_error("Bridge element not specified");
    }
    const char* bridgeName = bridgeElem->Attribute("name");
    if ( !bridgeName ) {
        throw std::runtime_error("Bridge name attribute not specified");
    }
    std::string bridge = bridgeName;

    network->name = name;
    network->uuid = uuid;
    if ( forward == "nat" ) {
        network->forward = NAT;
    }
    else {
        network->forward = BRIDGE;
    }
    network->bridgeName = bridge;
    network->xmlDesc = xmlDesc;

    if ( forward == "nat" ) {
        XMLElement* macElem = rootElem->FirstChildElement("mac");
        std::string macAddr;
        if ( macElem ) {
            const char* macAddrAttr = macElem->Attribute("address");
            if ( macAddrAttr ) {
                macAddr = macAddrAttr;
            }
            else {
                LOG_WARN("MAC address attribute not specified, using empty MAC");
            }
        }

        std::string ipAddr, netmask, start, end;
        // 解析IP配置
        XMLElement* ipElem = rootElem->FirstChildElement("ip");
        if ( ipElem ) {
            const char* ipAddrAttr = ipElem->Attribute("address");
            const char* netmaskAttr = ipElem->Attribute("netmask");

            if ( ipAddrAttr ) {
                ipAddr = ipAddrAttr;
            }
            if ( netmaskAttr ) {
                netmask = netmaskAttr;
            }

            // 解析DHCP配置
            XMLElement* dhcpElem = ipElem->FirstChildElement("dhcp");
            if ( dhcpElem ) {
                XMLElement* rangeElem = dhcpElem->FirstChildElement("range");
                if ( rangeElem ) {
                    const char* startAttr = rangeElem->Attribute("start");
                    const char* endAttr = rangeElem->Attribute("end");

                    if ( startAttr ) {
                        start = startAttr;
                    }
                    if ( endAttr ) {
                        end = endAttr;
                    }
                }
            }
        }
        network->macAddress = macAddr;
        network->ipAddress = ipAddr;
        network->netmask = netmask;
        network->DHCPRange.first = start;
        network->DHCPRange.second = end;
    }

    return network;
}
