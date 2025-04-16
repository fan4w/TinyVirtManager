#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include "../conf/storage_conf.h"
#include "../driver-storage.h"
#include <unordered_map>
#include <mutex>
#include <sys/stat.h>

class FileSystemStorageDriver : public StorageDriver {
public:
    FileSystemStorageDriver();
    virtual ~FileSystemStorageDriver();

    // 存储池相关操作
    std::vector<std::shared_ptr<VirStoragePool>> connectListStoragePools(unsigned int flags = 0) const override;
    std::shared_ptr<VirStoragePool> storagePoolLookupByName(const std::string& name) const override;
    std::shared_ptr<VirStoragePool> storagePoolLookupByUUID(const std::string& uuid) const override;

    std::shared_ptr<VirStoragePool> storagePoolDefine(const std::string& xml, unsigned int flags = 0) override;
    int storagePoolCreate(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) override;
    std::shared_ptr<VirStoragePool> storagePoolCreateXML(const std::string& xml, unsigned int flags = 0) override;
    int storagePoolDestroy(std::shared_ptr<VirStoragePool> pool) override;
    int storagePoolUndefine(std::shared_ptr<VirStoragePool> pool) override;

    int storagePoolDelete(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) override;
    int storagePoolGetState(std::shared_ptr<VirStoragePool> pool) const override;
    int storagePoolGetType(std::shared_ptr<VirStoragePool> pool) const override;
    std::string storagePoolGetPath(std::shared_ptr<VirStoragePool> pool) const override;

    // 存储卷相关操作
    std::vector<std::shared_ptr<VirStorageVol>> storagePoolListAllVolumes(std::shared_ptr<VirStoragePool> pool, unsigned int flags = 0) const override;
    std::shared_ptr<VirStorageVol> storageVolLookupByName(std::shared_ptr<VirStoragePool> pool, const std::string& name) const override;
    std::shared_ptr<VirStorageVol> storageVolLookupByPath(const std::string& path) const override;

    std::shared_ptr<VirStorageVol> storageVolCreateXML(std::shared_ptr<VirStoragePool> pool, const std::string& xml, unsigned int flags = 0) override;
    std::shared_ptr<VirStorageVol> storageVolCreateXMLFrom(std::shared_ptr<VirStoragePool> pool, const std::string& xml, std::shared_ptr<VirStorageVol> srcVol, unsigned int flags = 0) override;
    int storageVolDelete(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) override;
    int storageVolWipe(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) override;
    int storageVolResize(std::shared_ptr<VirStorageVol> vol, unsigned long long capacity, unsigned int flags = 0) override;
    
    unsigned long long storageVolGetAllocation(std::shared_ptr<VirStorageVol> vol) const override;
    unsigned long long storageVolGetCapacity(std::shared_ptr<VirStorageVol> vol) const override;
    int storageVolGetType(std::shared_ptr<VirStorageVol> vol) const override;
    std::string storageVolGetXMLDesc(std::shared_ptr<VirStorageVol> vol, unsigned int flags = 0) const override;
private:
    // 存储池管理
    mutable std::mutex poolsMutex;
    std::vector<std::shared_ptr<StoragePoolObj>> pools; // 存储池列表

    // 存储卷管理
    mutable std::mutex volumesMutex;
    std::unordered_map<std::shared_ptr<StoragePoolObj>, std::vector<std::shared_ptr<StorageVolumeObj>>> volumesMap; // 存储卷映射

    // 配置目录
    std::string configDir;
    std::string poolsDir;

    // 辅助函数
    std::shared_ptr<StoragePoolObj> parseAndCreateStoragePoolObj(const std::string& xmlDesc);
    std::shared_ptr<VirStorageVol> parseAndCreateStorageVolume(const std::string& xmlDesc, std::shared_ptr<VirStoragePool> pool);


    bool fileExists(const std::string& path) const;
    bool createDirectoryIfNotExists(const std::string& path) const;
    void loadPoolConfigs();
};


#endif // STORAGE_DRIVER_H