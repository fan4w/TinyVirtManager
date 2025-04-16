#ifndef VIRSTORAGEVOL_H
#define VIRSTORAGEVOL_H

#include <string>
#include <memory>

class StorageDriver;
class VirStoragePool;

class VirStorageVol {
private:
    std::string name;
    std::string uuid;               // 唯一标识符
    std::string path;
    // unsigned long long capacity;    // 总容量
    // unsigned long long allocation;  // 已分配空间
    // int type;  // 卷类型
    std::shared_ptr<VirStoragePool> pool;  // 所属存储池
    StorageDriver* driver;  // 对应的存储驱动指针

public:
    VirStorageVol(const std::string& name, std::string uuid, std::string path,
        std::shared_ptr<VirStoragePool> pool = nullptr, StorageDriver* driver = nullptr)
        : name(name), uuid(uuid), path(path), pool(pool), driver(driver) {
    }

    // 基本信息获取
    std::string virStorageVolGetName() const;
    std::string virStorageVolGetKey() const;
    std::string virStorageVolGetPath() const;
    std::string virStorageVolGetXMLDesc(unsigned int flags = 0) const;
    unsigned long long virStorageVolGetCapacity() const;
    unsigned long long virStorageVolGetAllocation() const;
    int virStorageVolGetType() const;
    std::shared_ptr<VirStoragePool> virStorageVolGetPool() const;

    // 存储卷操作
    int virStorageVolDelete(unsigned int flags = 0);
    int virStorageVolResize(unsigned long long capacity, unsigned int flags = 0);
    int virStorageVolWipe(unsigned int flags = 0);
};

#endif // VIRSTORAGEVOL_H