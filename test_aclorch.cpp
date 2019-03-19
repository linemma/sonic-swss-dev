#include "gtest/gtest.h"

#include "orchdaemon.h"
#include "saihelper.h"
//#include "aclorch.h"

void syncd_apply_view()
{
}

/* Global variables */
sai_object_id_t gVirtualRouterId;
sai_object_id_t gUnderlayIfId;
sai_object_id_t gSwitchId = SAI_NULL_OBJECT_ID;
MacAddress gMacAddress;
MacAddress gVxlanMacAddress;

#define DEFAULT_BATCH_SIZE  128
int gBatchSize = DEFAULT_BATCH_SIZE;

bool gSairedisRecord = true;
bool gSwssRecord = true;
bool gLogRotate = false;
ofstream gRecordOfs;
string gRecordFile;

TEST(aclOrch, initial)
{
    DBConnector appl_db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector config_db(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector state_db(STATE_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);

//    auto orchDaemon = make_shared<OrchDaemon>(&appl_db, &config_db, &state_db);

#if 0
    PortsOrch *gPortsOrch;
    AclOrch *gAclOrch;
    NeighOrch *gNeighOrch;
    RouteOrch *gRouteOrch;

    FdbOrch *gFdbOrch;
    IntfsOrch *gIntfsOrch;
    CrmOrch *gCrmOrch;
    BufferOrch *gBufferOrch;
    SwitchOrch *gSwitchOrch;

    DBConnector appl_db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector config_db(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector state_db(STATE_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);

    TableConnector confDbAclTable(&config_db, CFG_ACL_TABLE_NAME);
    TableConnector confDbAclRuleTable(&config_db, CFG_ACL_RULE_TABLE_NAME);
    vector<TableConnector> acl_table_connectors = {
        confDbAclTable,
        confDbAclRuleTable
    };

    const int portsorch_base_pri = 40;
    vector<table_name_with_pri_t> ports_tables = {
        { APP_PORT_TABLE_NAME,        portsorch_base_pri + 5 },
        { APP_VLAN_TABLE_NAME,        portsorch_base_pri + 2 },
        { APP_VLAN_MEMBER_TABLE_NAME, portsorch_base_pri     },
        { APP_LAG_TABLE_NAME,         portsorch_base_pri + 4 },
        { APP_LAG_MEMBER_TABLE_NAME,  portsorch_base_pri     }
    };
    gPortsOrch = new PortsOrch(&appl_db, ports_tables);

    TableConnector stateDbMirrorSession(&state_db, APP_MIRROR_SESSION_TABLE_NAME);
    TableConnector confDbMirrorSession(&config_db, CFG_MIRROR_SESSION_TABLE_NAME);
    MirrorOrch *mirror_orch = new MirrorOrch(stateDbMirrorSession, confDbMirrorSession, gPortsOrch, NULL, NULL, NULL/*gRouteOrch, gNeighOrch, gFdbOrch*/);

    gAclOrch = new AclOrch(acl_table_connectors, gPortsOrch, mirror_orch, /*gNeighOrch*/NULL, /*gRouteOrch*/NULL);
#endif /* 0 */
}
