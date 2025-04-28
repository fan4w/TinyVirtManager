#ifndef VIRNETWORK_H
#define VIRNETWORK_H

#include <string>
#include "driver-network.h"

class NetworkDriver;

class VirNetwork {
private:
    std::string name;
    std::string uuid;
    NetworkDriver* driver;
public:
    VirNetwork(const std::string& name, const std::string& uuid)
        : name(name), uuid(uuid) {
    }
    VirNetwork(const std::string& name, const std::string& uuid, NetworkDriver* driver)
        : name(name), uuid(uuid), driver(driver) {
    }
    std::string virNetworkGetName() const;
    std::string virNetworkGetUUID() const;
    std::string virNetworkGetXMLDesc(unsigned int flags = 0) const;
    void virNetworkCreate();
    void virNetworkDestroy();
};

#endif // VIRNETWORK_H