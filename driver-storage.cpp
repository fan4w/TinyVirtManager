#include "driver-storage.h"
#include "storage/storage_driver.h"

std::unique_ptr<StorageDriver> StorageDriverFactory::createStorageDriver(const std::string& type) {
    if ( type == "filesystem" ) {
        return std::unique_ptr<StorageDriver>(new FileSystemStorageDriver());
    }
    else return nullptr;
}