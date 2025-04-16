#ifndef STORAGE_CONF_H
#define STORAGE_CONF_H

#include <string>
#include <vector>
#include <mutex>
#include <memory>

class StorageVolumeObj {
public:
    std::string name;           // 卷名称
    std::string uuid;           // 卷UUID
    std::string path;           // 卷路径
    std::string format;         // 卷格式, 例如qcow2, raw等
    int type;                   // 卷类型

    size_t capacity = 0;        // 卷容量
    size_t allocation = 0;      // 卷已分配空间
};

class StoragePoolObj {
public:
    std::string name;           // 存储池名称
    std::string uuid;           // 存储池UUID
    int type;                   // 存储池类型
    std::string path;           // 存储池目标路径
    std::string xmlDesc;        // 存储池XML描述，便于开发和调试

    bool active;                // 存储池是否处于活动状态
    bool persistent;            // 存储池是否持久化
    size_t capacity;            // 存储池容量
    size_t available;           // 存储池可用空间
    size_t allocation;          // 存储池已分配空间

    std::mutex poolMutex;       // 互斥锁，用于线程安全

    std::vector<std::shared_ptr<StorageVolumeObj>> volumes; // 存储池中的卷列表
};

#endif