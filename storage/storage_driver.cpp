#include "storage_driver.h"
#include "../virConnect.h"
#include "../virStoragePool.h"
#include "../virStorageVol.h"
#include "../log/log.h"
#include "../conf/config_manager.h"
#include "../tinyxml/tinyxml2.h"
#include "../util/generate_uuid.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>
#include <ctime>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

FileSystemStorageDriver::FileSystemStorageDriver() {
    // 获取配置目录
    auto configManager = ConfigManager::Instance();
    configDir = configManager->getValue("storage.config_dir", "./temp/storage");
    poolsDir = configDir + "/pools";

    // 确保配置目录存在
    createDirectoryIfNotExists(configDir);
    createDirectoryIfNotExists(poolsDir);

    // 加载现有存储池配置
    loadPoolConfigs();

    LOG_INFO("FileSystemStorageDriver initialized, config directory: %s", configDir.c_str());
}

FileSystemStorageDriver::~FileSystemStorageDriver() {
    LOG_INFO("FileSystemStorageDriver destroyed");
}

std::vector<std::shared_ptr<VirStoragePool>> FileSystemStorageDriver::connectListStoragePools(unsigned int flags) const {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    std::vector<std::shared_ptr<VirStoragePool>> poolList;
    std::lock_guard<std::mutex> lock(poolsMutex);

    // 遍历存储池映射，添加到返回列表中
    for ( const auto& pool : pools ) {
        poolList.push_back(std::make_shared<VirStoragePool>(pool->name, pool->uuid));
    }

    return poolList;
}

std::shared_ptr<VirStoragePool> FileSystemStorageDriver::storagePoolLookupByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(poolsMutex);

    for ( const auto& pool : pools ) {
        if ( pool->name == name ) {
            return std::make_shared<VirStoragePool>(pool->name, pool->uuid);
        }
    }

    LOG_WARN("Storage pool not found: %s", name.c_str());
    return nullptr;
}

std::shared_ptr<VirStoragePool> FileSystemStorageDriver::storagePoolLookupByUUID(const std::string& uuid) const {
    std::lock_guard<std::mutex> lock(poolsMutex);

    for ( const auto& pool : pools ) {
        if ( pool->uuid == uuid ) {
            return std::make_shared<VirStoragePool>(pool->name, pool->uuid);
        }
    }

    LOG_WARN("Storage pool not found: %s", uuid.c_str());
    return nullptr;
}

std::shared_ptr<VirStoragePool> FileSystemStorageDriver::storagePoolDefine(const std::string& xml, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    try {
        auto poolObj = parseAndCreateStoragePoolObj(xml);
        {
            std::lock_guard<std::mutex> lock(poolsMutex);
            // 检查存储池是否已存在
            std::string name = poolObj->name;
            for ( const auto& existingPool : pools ) {
                if ( existingPool->name == name ) {
                    LOG_ERROR("Storage pool already exists: %s", name.c_str());
                    return nullptr;
                }
            }
            // 添加到存储池列表
            pools.push_back(poolObj);
        }
        auto pool = std::make_shared<VirStoragePool>(poolObj->name, poolObj->uuid);
        // 保存配置文件
        std::string configFile = poolsDir + "/" + poolObj->name + ".xml";
        std::ofstream file(configFile);
        if ( !file.is_open() ) {
            LOG_ERROR("Failed to open storage pool config file: %s", configFile.c_str());
            throw std::runtime_error("Failed to open storage pool config file");
        }
        file << xml;
        file.close();

        LOG_INFO("Storage pool defined: %s", poolObj->name.c_str());
        return pool;
    }
    catch ( const std::exception& e ) {
        LOG_ERROR("Failed to define storage pool: %s", e.what());
        throw;
    }
}

std::shared_ptr<VirStoragePool> FileSystemStorageDriver::storagePoolCreateXML(const std::string& xml, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    auto poolObj = parseAndCreateStoragePoolObj(xml);
    auto pool = std::make_shared<VirStoragePool>(poolObj->name, poolObj->uuid);
    storagePoolCreate(pool, flags);
    return pool;
}

int FileSystemStorageDriver::storagePoolUndefine(std::shared_ptr<VirStoragePool> pool) {
    if ( !pool ) {
        LOG_ERROR("Attempt to undefine null storage pool");
        return -1;
    }

    const std::string& name = pool->virStoragePoolGetName();
    const std::string& uuid = pool->virStoragePoolGetUUID();

    // 检查存储池状态，只能取消定义非活动的存储池
    if ( pool->virStoragePoolGetState() != VIR_STORAGE_POOL_INACTIVE ) {
        LOG_ERROR("Cannot undefine active storage pool: %s", name.c_str());
        return -1;
    }

    // 删除配置文件
    std::string configFile = poolsDir + "/" + name + ".xml";
    if ( unlink(configFile.c_str()) != 0 && errno != ENOENT ) {
        LOG_ERROR("Failed to delete storage pool config file: %s (%s)", configFile.c_str(), strerror(errno));
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(poolsMutex);
        // 从列表中删除
        std::remove_if(pools.begin(), pools.end(),
            [&uuid](const std::shared_ptr<StoragePoolObj>& pool) { return pool->uuid == uuid; });
    }

    LOG_INFO("Storage pool undefined: %s", name.c_str());
    return 0;
}

int FileSystemStorageDriver::storagePoolCreate(std::shared_ptr<VirStoragePool> pool, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool creation: %u", flags);
        return -1;
    }
    std::string path = pool->virStoragePoolGetPath();
    if ( !createDirectoryIfNotExists(path) ) {
        LOG_ERROR("Failed to create storage pool directory: %s", path.c_str());
        return -1;
    }
    LOG_INFO("Storage pool created: %s", path.c_str());
    return 0;
}

int FileSystemStorageDriver::storagePoolDestroy(std::shared_ptr<VirStoragePool> pool) {
    if ( !pool ) {
        LOG_ERROR("Attempt to stop null storage pool");
        return -1;
    }

    try {
        // TODO: 实现存储池停止逻辑
        LOG_INFO("Storage pool stopped: %s", pool->virStoragePoolGetName().c_str());
        return 0;
    }
    catch ( const std::exception& e ) {
        LOG_ERROR("Failed to stop storage pool: %s", e.what());
        return -1;
    }
}

int FileSystemStorageDriver::storagePoolDelete(std::shared_ptr<VirStoragePool> pool, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    if ( !pool ) {
        LOG_ERROR("Attempt to delete null storage pool");
        return -1;
    }

    try {
        // TODO: 实现存储池删除逻辑
        LOG_INFO("Storage pool resources deleted: %s", pool->virStoragePoolGetName().c_str());
        return 0;
    }
    catch ( const std::exception& e ) {
        LOG_ERROR("Failed to delete storage pool resources: %s", e.what());
        return -1;
    }
}

int FileSystemStorageDriver::storagePoolGetState(std::shared_ptr<VirStoragePool> pool) const {
    if ( !pool ) {
        LOG_ERROR("Attempt to get state of null storage pool");
        return -1;
    }
    for ( const auto& poolObj : pools ) {
        if ( poolObj->name == pool->virStoragePoolGetName() ) {
            return poolObj->active ? VIR_STORAGE_POOL_RUNNING : VIR_STORAGE_POOL_INACTIVE;
        }
    }
    LOG_ERROR("Storage pool not found: %s", pool->virStoragePoolGetName().c_str());
    return -1;
}

int FileSystemStorageDriver::storagePoolGetType(std::shared_ptr<VirStoragePool> pool) const {
    if ( !pool ) {
        LOG_ERROR("Attempt to get type of null storage pool");
        return -1;
    }
    for ( const auto& poolObj : pools ) {
        if ( poolObj->uuid == pool->virStoragePoolGetUUID() ) {
            return poolObj->type;
        }
    }
    LOG_ERROR("Storage pool not found: %s", pool->virStoragePoolGetName().c_str());
    return -1;
}

std::string FileSystemStorageDriver::storagePoolGetPath(std::shared_ptr<VirStoragePool> pool) const {
    if ( !pool ) {
        LOG_ERROR("Attempt to get path of null storage pool");
        return "";
    }
    for ( const auto& poolObj : pools ) {
        if ( poolObj->uuid == pool->virStoragePoolGetUUID() ) {
            return poolObj->path;
        }
    }
    return "";
}

// 以下是存储池相关的操作

std::vector<std::shared_ptr<VirStorageVol>> FileSystemStorageDriver::storagePoolListAllVolumes(
    std::shared_ptr<VirStoragePool> pool, unsigned int flags) const {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    if ( !pool ) {
        LOG_ERROR("Attempt to list volumes in null storage pool");
        return {};
    }
    std::vector<std::shared_ptr<VirStorageVol>> volumeList;
    std::lock_guard<std::mutex> lock(volumesMutex);
    std::shared_ptr<StoragePoolObj> poolObj;
    for ( const auto& it : pools ) {
        if ( it->uuid == pool->virStoragePoolGetUUID() ) {
            poolObj = it;
            break;
        }
    }
    auto it = volumesMap.find(poolObj);
    if ( it != volumesMap.end() ) {
        for ( const auto& volPair : it->second ) {
            volumeList.push_back(std::make_shared<VirStorageVol>(volPair->name, volPair->uuid, volPair->path));
        }
    }
    else {
        LOG_WARN("No volumes found in storage pool: %s", pool->virStoragePoolGetName().c_str());
    }
    return volumeList;
}

std::shared_ptr<VirStorageVol> FileSystemStorageDriver::storageVolLookupByName(
    std::shared_ptr<VirStoragePool> pool, const std::string& name) const {
    if ( !pool ) {
        LOG_ERROR("Attempt to lookup volume in null storage pool");
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(volumesMutex);
    std::shared_ptr<StoragePoolObj> poolObj;
    for ( const auto& it : pools ) {
        if ( it->uuid == pool->virStoragePoolGetUUID() ) {
            poolObj = it;
            break;
        }
    }
    auto it = volumesMap.find(poolObj);
    if ( it != volumesMap.end() ) {
        for ( const auto& volPair : it->second ) {
            if ( volPair->name == name ) {
                return std::make_shared<VirStorageVol>(volPair->name, volPair->uuid, volPair->path);
            }
        }
    }
    LOG_WARN("Storage volume not found: %s in pool: %s", name.c_str(), pool->virStoragePoolGetName().c_str());
    return nullptr;
}


std::shared_ptr<VirStorageVol> FileSystemStorageDriver::storageVolLookupByPath(const std::string& path) const {
    std::lock_guard<std::mutex> lock(volumesMutex);
    for ( auto it : volumesMap ) {
        for ( auto vol : it.second ) {
            if ( vol->path == path ) {
                return std::make_shared<VirStorageVol>(vol->name, vol->uuid, vol->path);
            }
        }
    }
    LOG_WARN("Storage volume not found by path: %s", path.c_str());
    return nullptr;
}

std::shared_ptr<VirStorageVol> FileSystemStorageDriver::storageVolCreateXML(
    std::shared_ptr<VirStoragePool> pool, const std::string& xml, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    if ( !pool ) {
        LOG_ERROR("Attempt to create volume in null storage pool");
        throw std::runtime_error("Invalid storage pool");
    }

    if ( pool->virStoragePoolGetState() != VIR_STORAGE_POOL_RUNNING ) {
        LOG_ERROR("Creating volume in inactive storage pool: %s", pool->virStoragePoolGetName().c_str());
        throw std::runtime_error("Storage pool not active");
    }

    try {
        // 解析XML获取卷信息
        tinyxml2::XMLDocument doc;
        if ( doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS ) {
            throw std::runtime_error("Failed to parse volume XML description");
        }

        tinyxml2::XMLElement* volElem = doc.FirstChildElement("volume");
        if ( !volElem ) {
            throw std::runtime_error("Volume element not found in XML");
        }

        // 获取卷名
        tinyxml2::XMLElement* nameElem = volElem->FirstChildElement("name");
        if ( !nameElem || !nameElem->GetText() ) {
            throw std::runtime_error("Volume name not specified");
        }
        std::string name = nameElem->GetText();

        // 获取卷容量
        unsigned long long capacity = 0;
        tinyxml2::XMLElement* capacityElem = volElem->FirstChildElement("capacity");
        if ( capacityElem && capacityElem->GetText() ) {
            capacity = std::stoull(capacityElem->GetText());
        }
        else {
            // 默认容量
            capacity = 1024 * 1024; // 1MB
        }

        // 生成卷路径
        std::string poolPath = pool->virStoragePoolGetPath();
        std::string volPath = poolPath + "/" + name;

        // 检查卷是否已存在
        if ( fileExists(volPath) ) {
            throw std::runtime_error("Volume already exists: " + name);
        }

        // 创建卷文件
        int fd = open(volPath.c_str(), O_CREAT | O_WRONLY, 0644);
        if ( fd < 0 ) {
            throw std::runtime_error("Failed to create volume file: " + std::string(strerror(errno)));
        }

        // 调整文件大小
        if ( ftruncate(fd, capacity) != 0 ) {
            close(fd);
            unlink(volPath.c_str());
            throw std::runtime_error("Failed to set volume size: " + std::string(strerror(errno)));
        }

        close(fd);

        // 创建卷对象
        auto vol = std::make_shared<VirStorageVol>(name, generateUUID(), volPath);
        auto volObj = std::make_shared<StorageVolumeObj>();
        volObj->name = name;
        volObj->uuid = vol->virStorageVolGetKey();
        volObj->path = volPath;
        volObj->capacity = capacity;
        volObj->allocation = capacity; // 初始分配等于容量
        volObj->format = "raw"; // 默认格式为raw

        LOG_INFO("Storage volume created: %s (path: %s)", name.c_str(), volPath.c_str());
        return vol;
    }
    catch ( const std::exception& e ) {
        LOG_ERROR("Failed to create storage volume: %s", e.what());
        throw;
    }
    return nullptr;
}

std::shared_ptr<VirStorageVol> FileSystemStorageDriver::storageVolCreateXMLFrom(std::shared_ptr<VirStoragePool> pool, const std::string& xml, std::shared_ptr<VirStorageVol> srcVol, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    // fix warning: unused parameter 'srcVol' 'pool'
    (void)pool;
    (void)srcVol;

    // TODO
    LOG_DEBUG("Creating storage volume from XML: %s", xml.c_str());
    return nullptr;
}

int FileSystemStorageDriver::storageVolDelete(std::shared_ptr<VirStorageVol> vol, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    // TODO
    LOG_DEBUG("Deleting storage volume: %s", vol->virStorageVolGetName().c_str());
    return 0;
}

int FileSystemStorageDriver::storageVolResize(std::shared_ptr<VirStorageVol> vol, unsigned long long capacity, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    // TODO
    LOG_DEBUG("Resizing storage volume: %s to %llu", vol->virStorageVolGetName().c_str(), capacity);
    return 0;
}

int FileSystemStorageDriver::storageVolWipe(std::shared_ptr<VirStorageVol> vol, unsigned int flags) {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    // TODO
    LOG_DEBUG("Wiping storage volume: %s", vol->virStorageVolGetName().c_str());
    return 0;
}

unsigned long long FileSystemStorageDriver::storageVolGetAllocation(std::shared_ptr<VirStorageVol> vol) const {
    if ( !vol ) {
        LOG_ERROR("Attempt to get allocation of null storage volume");
        return 0;
    }
    std::lock_guard<std::mutex> lock(volumesMutex);
    for ( auto it : volumesMap ) {
        for ( auto volObj : it.second ) {
            if ( volObj->uuid == vol->virStorageVolGetKey() ) {
                return volObj->allocation;
            }
        }
    }
    LOG_WARN("Storage volume not found: %s", vol->virStorageVolGetName().c_str());
    return 0;
}

unsigned long long FileSystemStorageDriver::storageVolGetCapacity(std::shared_ptr<VirStorageVol> vol) const {
    if ( !vol ) {
        LOG_ERROR("Attempt to get capacity of null storage volume");
        return 0;
    }
    std::lock_guard<std::mutex> lock(volumesMutex);
    for ( auto it : volumesMap ) {
        for ( auto volObj : it.second ) {
            if ( volObj->uuid == vol->virStorageVolGetKey() ) {
                return volObj->capacity;
            }
        }
    }
    LOG_WARN("Storage volume not found: %s", vol->virStorageVolGetName().c_str());
    return 0;
}

int FileSystemStorageDriver::storageVolGetType(std::shared_ptr<VirStorageVol> vol) const {
    if ( !vol ) {
        LOG_ERROR("Attempt to get type of null storage volume");
        return -1;
    }
    std::lock_guard<std::mutex> lock(volumesMutex);
    for ( auto it : volumesMap ) {
        for ( auto volObj : it.second ) {
            if ( volObj->uuid == vol->virStorageVolGetKey() ) {
                return volObj->type;
            }
        }
    }
    LOG_WARN("Storage volume not found: %s", vol->virStorageVolGetName().c_str());
    return -1;
}

std::string FileSystemStorageDriver::storageVolGetXMLDesc(std::shared_ptr<VirStorageVol> vol, unsigned int flags) const {
    if ( flags != 0 ) {
        LOG_ERROR("Invalid flags for storage pool listing: %u", flags);
        return {};
    }
    if ( !vol ) {
        LOG_ERROR("Attempt to get XML description of null storage volume");
        return "";
    }
    std::lock_guard<std::mutex> lock(volumesMutex);
    for ( auto it : volumesMap ) {
        for ( auto volObj : it.second ) {
            if ( volObj->uuid == vol->virStorageVolGetKey() ) {
                return "volObj->name is returned";  // TODO: 返回卷的 XML 描述
            }
        }
    }
    LOG_WARN("Storage volume not found: %s", vol->virStorageVolGetName().c_str());
    return "";
}

// TODO: 实现存储池的 XML 描述解析和创建
std::shared_ptr<StoragePoolObj> FileSystemStorageDriver::parseAndCreateStoragePoolObj(const std::string& xmlDesc) {
    auto pool = std::make_shared<StoragePoolObj>();
    // 解析XML并创建存储池对象
    tinyxml2::XMLDocument doc;
    if ( doc.Parse(xmlDesc.c_str()) != tinyxml2::XML_SUCCESS ) {
        throw std::runtime_error("Failed to parse storage pool XML description");
    }

    tinyxml2::XMLElement* poolElem = doc.FirstChildElement("pool");
    if ( !poolElem ) {
        throw std::runtime_error("Pool element not found in XML");
    }

    // 获取存储池名称和UUID
    tinyxml2::XMLElement* nameElem = poolElem->FirstChildElement("name");
    if ( !nameElem || !nameElem->GetText() ) {
        throw std::runtime_error("Pool name not specified");
    }
    std::string name = nameElem->GetText();

    tinyxml2::XMLElement* uuidElem = poolElem->FirstChildElement("uuid");
    std::string uuid;
    if ( uuidElem && uuidElem->GetText() ) {
        uuid = uuidElem->GetText();
    }
    else {
        uuid = generateUUID();
    }

    pool->name = name;
    pool->uuid = uuid;
    pool->xmlDesc = xmlDesc;

    return pool;
}

bool FileSystemStorageDriver::fileExists(const std::string& path) const {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool FileSystemStorageDriver::createDirectoryIfNotExists(const std::string& path) const {
    // 检查目录是否存在
    struct stat st;
    if ( stat(path.c_str(), &st) == 0 ) {
        if ( S_ISDIR(st.st_mode) ) {
            // 目录已存在，无需创建
            return true;
        }
        else {
            // 路径存在但不是目录
            throw std::runtime_error("Path exists but is not a directory: " + path);
        }
    }

    // 递归创建目录，权限设置为 0755 (rwxr-xr-x)
    // 创建嵌套目录需要逐级创建
    size_t pos = 0;
    std::string current_path;

    // 处理相对路径和绝对路径
    if ( path[0] == '/' ) {
        current_path = "/";
        pos = 1;
    }

    while ( (pos = path.find('/', pos)) != std::string::npos ) {
        current_path = path.substr(0, pos);
        if ( !current_path.empty() && stat(current_path.c_str(), &st) != 0 ) {
            if ( mkdir(current_path.c_str(), 0755) != 0 && errno != EEXIST ) {
                throw std::runtime_error("Failed to create directory: " + current_path + ", error: " + strerror(errno));
            }
        }
        pos++;
    }

    // 创建最终目录
    if ( mkdir(path.c_str(), 0755) != 0 && errno != EEXIST ) {
        throw std::runtime_error("Failed to create directory: " + path + ", error: " + strerror(errno));
    }

    // std::cout << "Created directory: " << path << std::endl;
    LOG_INFO("Created directory: %s", path.c_str());
    return true;
}

void FileSystemStorageDriver::loadPoolConfigs() {
    DIR* dir = opendir(poolsDir.c_str());
    if ( !dir ) {
        LOG_ERROR("Failed to open directory: %s", poolsDir.c_str());
        return;
    }
    struct dirent* entry;
    while ( (entry = readdir(dir)) != nullptr ) {
        if ( entry->d_type == DT_REG ) { // 仅处理常规文件
            std::string fileName = entry->d_name;
            if ( fileName.substr(fileName.find_last_of('.') + 1) == "xml" ) {
                std::string filePath = poolsDir + "/" + fileName;
                std::ifstream file(filePath);
                if ( !file.is_open() ) {
                    LOG_ERROR("Failed to open file: %s", filePath.c_str());
                    continue;
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string xmlDesc = buffer.str();
                auto poolObj = parseAndCreateStoragePoolObj(xmlDesc);
                pools.push_back(poolObj);
            }
        }
    }
    closedir(dir);
    LOG_INFO("Loaded %zu storage pool configurations", pools.size());
    // 遍历存储池对象，查看是否存在对应的卷
    for ( const auto& poolObj : pools ) {
        std::string poolPath = poolObj->path;
        DIR* volDir = opendir(poolPath.c_str());
        if ( volDir ) {
            struct dirent* volEntry;
            while ( (volEntry = readdir(volDir)) != nullptr ) {
                if ( volEntry->d_type == DT_REG ) { // 仅处理常规文件
                    std::string volName = volEntry->d_name;
                    std::string volPath = poolPath + "/" + volName;
                    auto volObj = std::make_shared<StorageVolumeObj>();
                    volObj->name = volName;
                    volObj->uuid = generateUUID();
                    volObj->path = volPath;
                    volumesMap[poolObj].push_back(volObj);
                }
            }
            closedir(volDir);
        }
    }
    LOG_INFO("Loaded %zu storage volumes", volumesMap.size());
}