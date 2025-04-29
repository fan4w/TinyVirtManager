#ifndef XEN_DOMAIN_H
#define XEN_DOMAIN_H

#include "../conf/domain_conf.h"

class xenDomainDef : public virDomainDef {};
class xenDomainObj : public virDomainObj {};

#endif // XEN_DOMAIN_H