#ifndef DRIVER_STORAGE_H
#define DRIVER_STORAGE_H

#include <string>
#include <vector>
#include <memory>

class VirStoragePool;
class VirStorageVol;

class StorageDriver {
public:
    virtual ~StorageDriver() = default;

    // 存储池相关操作
    virtual std::vector<std::shared_ptr<VirStoragePool>> connectListStoragePools(unsigned int flags = 0) const = 0;
    virtual std::shared_ptr<VirStoragePool> storagePoolLookupByName(const std::string& name) const = 0;
    virtual std::shared_ptr<VirStoragePool> storagePoolLookupByUUID(const std::string& uuid) const = 0;
    // virtual std::shared_ptr<VirStoragePool> storagePoolLookupByVolume(std::shared_ptr<VirStorageVol> vol) const = 0;

    virtual std::shared_ptr<VirStoragePool> storagePoolDefine(const std::string& xml, unsigned int flags = 0) = 0;
    virtual int storagePoolCreate(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) = 0;
    virtual std::shared_ptr<VirStoragePool> storagePoolCreateXML(const std::string& xml, unsigned int flags = 0) = 0;
    virtual int storagePoolDestroy(std::shared_ptr<VirStoragePool> pool) = 0;
    virtual int storagePoolUndefine(std::shared_ptr<VirStoragePool> pool) = 0;

    virtual int storagePoolDelete(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) = 0;
    virtual int storagePoolGetState(std::shared_ptr<VirStoragePool> pool) const = 0;
    virtual int storagePoolGetType(std::shared_ptr<VirStoragePool> pool) const = 0;
    virtual std::string storagePoolGetPath(std::shared_ptr<VirStoragePool> pool) const = 0;

    // 存储卷相关操作
    virtual std::vector<std::shared_ptr<VirStorageVol>> storagePoolListAllVolumes(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) const = 0;
    virtual std::shared_ptr<VirStorageVol> storageVolLookupByName(std::shared_ptr<VirStoragePool> pool, const std::string& name) const = 0;
    virtual std::shared_ptr<VirStorageVol> storageVolLookupByPath(const std::string& path) const = 0;

    virtual std::shared_ptr<VirStorageVol> storageVolCreateXML(std::shared_ptr<VirStoragePool> pool, const std::string& xml, unsigned int flags = 0) = 0;
    virtual std::shared_ptr<VirStorageVol> storageVolCreateXMLFrom(std::shared_ptr<VirStoragePool> pool, const std::string& xml, std::shared_ptr<VirStorageVol> srcVol, unsigned int flags = 0) = 0;
    virtual int storageVolDelete(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) = 0;
    virtual int storageVolResize(std::shared_ptr<VirStorageVol> vol, unsigned long long capacity, unsigned int flags = 0) = 0;
    virtual int storageVolWipe(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) = 0;

    virtual unsigned long long storageVolGetAllocation(std::shared_ptr<VirStorageVol> vol) const = 0;
    virtual unsigned long long storageVolGetCapacity(std::shared_ptr<VirStorageVol> vol) const = 0;
    virtual int storageVolGetType(std::shared_ptr<VirStorageVol> vol) const = 0;
    virtual std::string storageVolGetXMLDesc(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) const = 0;
};

class StorageDriverFactory {
public:
    static std::unique_ptr<StorageDriver> createStorageDriver(const std::string& type);
};

#endif // DRIVER_STORAGE_H