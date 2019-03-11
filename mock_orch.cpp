#include <limits.h>
#include <unordered_map>
#include <algorithm>
#include "logger.h"
#include "schema.h"
#include "ipprefix.h"
#include "converter.h"
#include "tokenize.h"
#include "timer.h"
#include "table.h"

#include "crmorch.h"
#include "aclorch.h"
#include "neighorch.h"
#include "routeorch.h"
#include "mirrororch.h"


PortsOrch* gPortsOrch;
CrmOrch *gCrmOrch;

Orch::Orch(DBConnector *db, const string tableName, int pri)
{

}

Orch::Orch(DBConnector *db, const vector<string> &tableNames)
{

}

Orch::Orch(DBConnector *db, const vector<table_name_with_pri_t> &tableNames_with_pri)
{

}

Orch::Orch(const vector<TableConnector>& tables)
{
    
}

Orch::~Orch()
{

}

bool Orch::bake()
{
    return true;
}

void Orch::doTask()
{

}

void Orch::addExecutor(Executor* executor)
{

}
#if 0
PortsOrch::PortsOrch(DBConnector *db, vector<table_name_with_pri_t> &tableNames) :
        Orch(db, tableNames)
{

}
#endif

bool PortsOrch::getPort(string alias, Port &p)
{
    return true;
}

bool PortsOrch::bindAclTable(sai_object_id_t id, sai_object_id_t table_oid, sai_object_id_t &group_member_oid, acl_stage_type_t acl_stage)
{
    return true;
}

bool PortsOrch::getAclBindPortId(string alias, sai_object_id_t &port_id)
{
    return true;
}

bool PortsOrch::isInitDone()
{
    return true;
}


bool PortsOrch::isPortReady()
{
    return true;
}

void CrmOrch::incCrmAclTableUsedCounter(CrmResourceType resource, sai_object_id_t tableId)
{
    return;
}

void CrmOrch::decCrmAclTableUsedCounter(CrmResourceType resource, sai_object_id_t tableId)
{

}

void CrmOrch::incCrmAclUsedCounter(CrmResourceType resource, sai_acl_stage_t stage, sai_acl_bind_point_type_t point)
{

}

void CrmOrch::decCrmAclUsedCounter(CrmResourceType resource, sai_acl_stage_t stage, sai_acl_bind_point_type_t point, sai_object_id_t oid)
{

}

#if 0
NeighOrch::NeighOrch(DBConnector *db, string tableName, IntfsOrch *intfsOrch) :
        Orch(db, tableName)
{

}
#endif

void NeighOrch::decreaseNextHopRefCount(const IpAddress &ipAddress)
{

}


bool NeighOrch::hasNextHop(IpAddress ipAddress)
{
    return true;
}

void NeighOrch::increaseNextHopRefCount(const IpAddress &ipAddress)
{

}

sai_object_id_t NeighOrch::getNextHopId(const IpAddress &ipAddress)
{
    sai_object_id_t id = 0;
    return id;
}

#if 0
RouteOrch::RouteOrch(DBConnector *db, string tableName, NeighOrch *neighOrch) :
        Orch(db, tableName)
{

}
#endif

void RouteOrch::decreaseNextHopRefCount(IpAddresses ipAddresses)
{

}

bool RouteOrch::isRefCounterZero(const IpAddresses& ipAddresses) const
{
    return true;
}

bool RouteOrch::removeNextHopGroup(IpAddresses ipAddresses)
{
    return true;
}

bool RouteOrch::hasNextHopGroup(const IpAddresses& ipAddresses) const
{
    return true;
}

bool RouteOrch::addNextHopGroup(IpAddresses ipAddresses)
{
    return true;
}

void RouteOrch::increaseNextHopRefCount(IpAddresses ipAddresses)
{
}

sai_object_id_t RouteOrch::getNextHopGroupId(const IpAddresses& ipAddresses)
{
    sai_object_id_t id;
    return id;
}

#if 0
MirrorOrch::MirrorOrch(TableConnector stateDbConnector, TableConnector confDbConnector,
        PortsOrch *portOrch, RouteOrch *routeOrch, NeighOrch *neighOrch, FdbOrch *fdbOrch) :
        Orch(vector<TableConnector> connectors(stateDbConnector, confDbConnector))
{

}
#endif

bool MirrorOrch::sessionExists(const string& name)
{
    return true;
}

bool MirrorOrch::getSessionStatus(const string& name, bool& state)
{
    return true;
}

bool MirrorOrch::increaseRefCount(const string& name)
{
    return true;
}

bool MirrorOrch::getSessionOid(const string& name, sai_object_id_t& oid)
{
    return true;
}

bool MirrorOrch::decreaseRefCount(const string& name)
{
    return true;
}

bool DTelOrch::getINTSessionOid(const string& name, sai_object_id_t& oid)
{
    return true;
}

bool DTelOrch::increaseINTSessionRefCount(const string& name)
{
    return true;
}

bool DTelOrch::decreaseINTSessionRefCount(const string& name)
{
    return true;
}