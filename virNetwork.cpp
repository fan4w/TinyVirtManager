#include "virNetwork.h"
#include "virConnect.h"

std::string VirNetwork::virNetworkGetName() const {
    return name;
}

std::string VirNetwork::virNetworkGetUUID() const {
    return uuid;
}

std::string VirNetwork::virNetworkGetXMLDesc(unsigned int flags) const {
    if ( driver ) {
        return driver->netWorkGetXMLDesc(std::make_shared<VirNetwork>(name, uuid), flags);
    }
    return "";
}

void VirNetwork::virNetworkCreate() {
    if ( driver ) {
        driver->networkCreate(std::make_shared<VirNetwork>(name, uuid));
    }
}
