#include "gtest/gtest.h"

#include "orchdaemon.h"
#include "saihelper.h"
#include "hiredis.h"
#include "consumertablebase.h"
//#include "aclorch.h"

void syncd_apply_view()
{
}

using namespace std;

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

uint32_t set_hostif_group_attr_count;
uint32_t set_hostif_attr_count;
sai_attribute_t set_hostif_group_attr_list[20];
sai_attribute_t set_hostif_attr_list[20];

extern sai_hostif_api_t* sai_hostif_api;
extern sai_policer_api_t* sai_policer_api;
extern sai_switch_api_t *sai_switch_api;

struct CoppTest : public ::testing::Test {

    CoppTest() {
        set_hostif_group_attr_count = 0;
        set_hostif_attr_count = 0;
        memset(set_hostif_group_attr_list, 0, sizeof(set_hostif_group_attr_list));
        memset(set_hostif_attr_list, 0, sizeof(set_hostif_attr_list));
    }
    ~CoppTest() {
    }
};

class CoppOrchExtend : public CoppOrch {
public:
    CoppOrchExtend(DBConnector *db, string tableName) : CoppOrch(db, tableName) {
    }
    task_process_status processCoppRule(Consumer& consumer) {
        CoppOrch::processCoppRule(consumer);
    }
    bool createPolicer(string trap_group, vector<sai_attribute_t> &policer_attribs) {
        CoppOrch::createPolicer(trap_group, policer_attribs);
    }
    bool removePolicer(string trap_group_name) {
        CoppOrch::removePolicer(trap_group_name);
    }
};

// struct CoppTestRedis : public ::testing::Test {
//     CoppTestRedis() {
//     }

//     void start_server_and_remote_all_data() {
//         //.....
//     }

//     // override
//     void SetUp() override {
//         start_server_and_remote_all_data();
//     }

//     void InjectData(int instance, void *data) {
//         if (instance == APPL_DB) {
//             ///
//         }
//         else if (instance == CONFIG_DB) {
//             ///
//         }
//         else if (instance == STATE_DB) {
//             ///
//         }
//     }

//     int GetData(int instance) {
//         return 0;
//     }
// };

int fake_create_hostif_trap_group(sai_object_id_t *hostif_trap_group_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t *attr_list) {
            set_hostif_group_attr_count = attr_count;
            memcpy(set_hostif_group_attr_list, attr_list, sizeof(sai_attribute_t)*attr_count);
            return SAI_STATUS_FAILURE;
        }

int fake_create_hostif_trap(sai_object_id_t *hostif_trap_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t *attr_list) {
            set_hostif_attr_count = attr_count;
            memcpy(set_hostif_attr_list, attr_list, sizeof(sai_attribute_t)*attr_count);
            return SAI_STATUS_FAILURE;
        }

TEST_F(CoppTest, create_hostif_group) {
    sai_hostif_api = new sai_hostif_api_t();

    sai_hostif_api->create_hostif_trap_group = fake_create_hostif_trap_group;
    sai_hostif_api->create_hostif_trap = fake_create_hostif_trap;

    DBConnector *appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    CoppOrchExtend *copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    Consumer *consumer = new Consumer(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1) ,copp_orch_extend, APP_COPP_TABLE_NAME);
 
    // set trap group expected data
    uint32_t expected_hostif_group_attr_count = 1;
    vector<sai_attribute_t> expected_hostif_group_attr_list;

    sai_attribute_t groupAttr;    
    memset(&groupAttr, 0, sizeof(groupAttr));    
    groupAttr.id = SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE;
    groupAttr.value.u32 = (uint32_t)3;
    expected_hostif_group_attr_list.push_back(groupAttr);

    // set grap expected data
    uint32_t expected_hostif_attr_count = 1;
    vector<sai_attribute_t> expected_hostif_attr_list;    

    sai_attribute_t trapAttr;    
    memset(&trapAttr, 0, sizeof(trapAttr));    
    trapAttr.id = SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION;
    trapAttr.value.s32 = SAI_PACKET_ACTION_DROP;
    expected_hostif_attr_list.push_back(trapAttr);

    //call CoPP function    
    copp_orch_extend->processCoppRule(*consumer);

    // validate ...
    EXPECT_EQ(expected_hostif_group_attr_count, set_hostif_group_attr_count);    
    // for (int i = 0; i < set_hostif_group_attr_count; ++i) {
    //     auto b_ret = verify_acltable_attr(expected_hostif_group_attr_list, &set_hostif_group_attr_list[i]);
    //     ASSERT_EQ(b_ret, true);
    // }

    EXPECT_EQ(expected_hostif_attr_count, set_hostif_attr_count);
    // for (int i = 0; i < set_hostif_attr_count; ++i) {
    //     auto b_ret = verify_acltable_attr(expected_hostif_attr_list, &set_hostif_attr_list[i]);
    //     ASSERT_EQ(b_ret, true);
    // }

    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    delete sai_hostif_api;
}