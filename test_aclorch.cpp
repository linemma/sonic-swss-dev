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

uint32_t expected_attr_count;
sai_attribute_t expected_attr_list[11] = {0};

extern sai_acl_api_t* sai_acl_api;

struct AclTest : public ::testing::Test {

};

struct AclTestRedis : public ::testing::Test {
    DBConnector appl_db;
    DBConnector config_db;
    DBConnector state_db;

    AclTestRedis() :
        appl_db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0),
        config_db(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0),
        state_db(STATE_DB, DBConnector::DEFAULT_UNIXSOCKET, 0) {
    }

    void start_server_and_remote_all_data() {
        //.....
    }

    // override
    void SetUp() override {
        start_server_and_remote_all_data();
    }

    void InjectData(int instance, void *data) {
        if (instance == APPL_DB) {
            ///
        }
        else if (instance == CONFIG_DB) {
            ///
        }
        else if (instance == STATE_DB) {
            ///
        }
    }

    int GetData(int instance) {
        return 0;
    }
};

int fake_create_acl_table(sai_object_id_t *acl_table_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t *attr_list) {
            expected_attr_count = attr_count;
            memcpy(expected_attr_list, attr_list, sizeof(expected_attr_list));
            return SAI_STATUS_FAILURE;
        }

TEST_F(AclTest, foo) {
    sai_acl_api = new sai_acl_api_t();
    // sai_acl_api->create_acl_table = [&](sai_object_id_t *acl_table_id,
    //     sai_object_id_t switch_id,
    //     uint32_t attr_count,
    //     const sai_attribute_t *attr_list) -> int { 
    //     //return SAI_STATUS_SUCCESS;
    //     return SAI_STATUS_FAILURE;
    // };

    sai_acl_api->create_acl_table = fake_create_acl_table;

    AclTable acltable;

    acltable.create();

    // create expected data
    sai_attribute_t attr;
    vector<sai_attribute_t> table_attrs;
    vector<int32_t> bpoint_list;

    bpoint_list = { SAI_ACL_BIND_POINT_TYPE_PORT, SAI_ACL_BIND_POINT_TYPE_LAG };

    attr.id = SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST;
    attr.value.s32list.count = static_cast<uint32_t>(bpoint_list.size());
    attr.value.s32list.list = bpoint_list.data();
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_IP;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_IP;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    attr.id = SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    int32_t range_types_list[] = { SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE, SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE };
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE;
    attr.value.s32list.count = (uint32_t)(sizeof(range_types_list) / sizeof(range_types_list[0]));
    attr.value.s32list.list = range_types_list;
    table_attrs.push_back(attr);

    sai_acl_stage_t acl_stage;
    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    acl_stage = SAI_ACL_STAGE_INGRESS;
    attr.value.s32 = acl_stage;
    table_attrs.push_back(attr);

    // validate ...
    EXPECT_EQ(expected_attr_count, table_attrs.size());
    
    EXPECT_EQ(expected_attr_list[0].id, table_attrs[0].id);
    EXPECT_EQ(expected_attr_list[0].value.s32list.count, table_attrs[0].value.s32list.count);
    ASSERT_TRUE( 0 == memcmp(expected_attr_list[0].value.s32list.list, table_attrs[0].value.s32list.list, sizeof(attr.value.s32list.list)));
    //ASSERT_TRUE( 0 == memcmp((void *)&expected_attr_list[0], (void *)&table_attrs[0], sizeof(expected_attr_list[0])));

    EXPECT_EQ(expected_attr_list[1].id, table_attrs[1].id);
    EXPECT_EQ(expected_attr_list[1].value.booldata, table_attrs[1].value.booldata);

    EXPECT_EQ(expected_attr_list[2].id, table_attrs[2].id);
    EXPECT_EQ(expected_attr_list[2].value.booldata, table_attrs[2].value.booldata);

    EXPECT_EQ(expected_attr_list[3].id, table_attrs[3].id);
    EXPECT_EQ(expected_attr_list[3].value.booldata, table_attrs[3].value.booldata);

    EXPECT_EQ(expected_attr_list[4].id, table_attrs[4].id);
    EXPECT_EQ(expected_attr_list[4].value.booldata, table_attrs[4].value.booldata);

    EXPECT_EQ(expected_attr_list[5].id, table_attrs[5].id);
    EXPECT_EQ(expected_attr_list[5].value.booldata, table_attrs[5].value.booldata);    

    EXPECT_EQ(expected_attr_list[6].id, table_attrs[6].id);
    EXPECT_EQ(expected_attr_list[6].value.booldata, table_attrs[6].value.booldata); 

    EXPECT_EQ(expected_attr_list[7].id, table_attrs[7].id);
    EXPECT_EQ(expected_attr_list[7].value.booldata, table_attrs[7].value.booldata); 

    EXPECT_EQ(expected_attr_list[8].id, table_attrs[8].id);
    EXPECT_EQ(expected_attr_list[8].value.booldata, table_attrs[8].value.booldata); 

    EXPECT_EQ(expected_attr_list[9].id, table_attrs[9].id);
    EXPECT_EQ(expected_attr_list[9].value.s32list.count, table_attrs[9].value.s32list.count);
    // EXPECT_EQ(expected_attr_list[9].value.s32list.list[0], table_attrs[9].value.s32list.list[0]);
    // EXPECT_EQ(expected_attr_list[9].value.s32list.list[1], table_attrs[9].value.s32list.list[1]);
    // ASSERT_TRUE( 0 == memcmp(expected_attr_list[9].value.s32list.list, table_attrs[9].value.s32list.list, sizeof(expected_attr_list[9].value.s32list.list)));

    EXPECT_EQ(expected_attr_list[10].id, table_attrs[10].id);
    EXPECT_EQ(expected_attr_list[10].value.s32, table_attrs[10].value.s32);

    sai_acl_api->create_acl_table = NULL;
    delete sai_acl_api;
}

extern const sai_acl_api_t redis_acl_api;

TEST_F(AclTestRedis, foo) {
    sai_acl_api = (sai_acl_api_t *) &redis_acl_api;

    InjectData(CONFIG_DB, 0);

    AclTable acltable;

    acltable.create();

    // validate ...
    auto x = GetData(ASIC_DB);
    // check x == ??
}

TEST(aclOrch, initial)
{
    DBConnector appl_db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector config_db(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector state_db(STATE_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);

    auto orchDaemon = make_shared<OrchDaemon>(&appl_db, &config_db, &state_db);

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
