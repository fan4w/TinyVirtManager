#ifndef VIRSTORAGEPOOL_H
#define VIRSTORAGEPOOL_H

#include <string>
#include <memory>
#include <vector>

class StorageDriver;
class VirStorageVol;

class VirStoragePool {
private:
    std::string name;
    std::string uuid;
    StorageDriver* driver;      // 对应的存储驱动指针

public:
    VirStoragePool(const std::string& name, const std::string& uuid, StorageDriver* driver = nullptr)
        : name(name), uuid(uuid), driver(driver) {
    }

    // 基本信息获取
    std::string virStoragePoolGetName() const;
    std::string virStoragePoolGetUUID() const;
    int virStoragePoolGetType() const;
    std::string virStoragePoolGetPath() const;
    // std::string virStoragePoolGetXMLDesc(unsigned int flags = 0) const;

    // 存储池操作
    // int virStoragePoolBuild(unsigned int flags = 0);
    int virStoragePoolDelete(unsigned int flags = 0);
    int virStoragePoolGetState() const;

    // 存储卷列表
    std::vector<std::shared_ptr<VirStorageVol>> virStoragePoolListAllVolumes(unsigned int flags = 0) const;
};

#endif // VIRSTORAGEPOOL_H