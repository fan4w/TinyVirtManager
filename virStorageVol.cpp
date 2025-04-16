#include "virStorageVol.h"
#include "virStoragePool.h"
#include "driver-storage.h"

std::string VirStorageVol::virStorageVolGetName() const {
    return name;
}

std::string VirStorageVol::virStorageVolGetKey() const {
    return uuid;
}

std::string VirStorageVol::virStorageVolGetPath() const {
    return path;
}

std::string VirStorageVol::virStorageVolGetXMLDesc(unsigned int flags) const {
    return driver ? driver->storageVolGetXMLDesc(std::make_shared<VirStorageVol>(*this), flags) : "";
}

unsigned long long VirStorageVol::virStorageVolGetCapacity() const {
    return driver ? driver->storageVolGetCapacity(std::make_shared<VirStorageVol>(*this)) : 0;
}

unsigned long long VirStorageVol::virStorageVolGetAllocation() const {
    return driver ? driver->storageVolGetAllocation(std::make_shared<VirStorageVol>(*this)) : 0;
}

int VirStorageVol::virStorageVolGetType() const {
    return driver ? driver->storageVolGetType(std::make_shared<VirStorageVol>(*this)) : -1;
}

std::shared_ptr<VirStoragePool> VirStorageVol::virStorageVolGetPool() const {
    return pool;
}

int VirStorageVol::virStorageVolDelete(unsigned int flags) {
    return driver ? driver->storageVolDelete(std::make_shared<VirStorageVol>(*this), flags) : -1;
}

int VirStorageVol::virStorageVolResize(unsigned long long capacity, unsigned int flags) {
    return driver ? driver->storageVolResize(std::make_shared<VirStorageVol>(*this), capacity, flags) : -1;
}
int VirStorageVol::virStorageVolWipe(unsigned int flags) {
    return driver ? driver->storageVolWipe(std::make_shared<VirStorageVol>(*this), flags) : -1;
}
