#include "qemu_driver.h"

QemuDriver::QemuDriver() {
    std::cout << "QEMU Driver initialized." << std::endl;
}

QemuDriver::~QemuDriver() {
    std::cout << "QEMU Driver destroyed." << std::endl;
}

void QemuDriver::domainCreate(std::shared_ptr<VirDomain> domain) {
    return;
}

void QemuDriver::domainCreateXML(const std::string& xml) {
    return;
}

void QemuDriver::domainDestroy(std::shared_ptr<VirDomain> domain) {
    return;
}

int QemuDriver::domainGetState(std::shared_ptr<VirDomain> domain) {
    return 0;
}