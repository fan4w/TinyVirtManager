#include "virNetwork.h"
#include "virConnect.h"

std::string VirNetwork::virNetworkGetName() const {
    return name;
}

std::string VirNetwork::virNetworkGetUUID() const {
    return uuid;
}

