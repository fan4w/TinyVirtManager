#include "virStoragePool.h"
#include "driver-storage.h"

std::string VirStoragePool::virStoragePoolGetName() const {
    return name;
}

std::string VirStoragePool::virStoragePoolGetUUID() const {
    return uuid;
}

int VirStoragePool::virStoragePoolGetType() const {
    return driver ? driver->storagePoolGetType(std::make_shared<VirStoragePool>(*this)) : -1;
}

std::string VirStoragePool::virStoragePoolGetPath() const {
    return driver ? driver->storagePoolGetPath(std::make_shared<VirStoragePool>(*this)) : "";
}

int VirStoragePool::virStoragePoolGetState() const {
    return driver ? driver->storagePoolGetState(std::make_shared<VirStoragePool>(*this)) : -1;
}

int VirStoragePool::virStoragePoolDelete(unsigned int flags) {
    return driver ? driver->storagePoolDelete(std::make_shared<VirStoragePool>(*this), flags) : -1;
}