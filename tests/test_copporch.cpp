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

#define DEFAULT_BATCH_SIZE 128
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

extern sai_hostif_api_t *sai_hostif_api;
extern sai_policer_api_t *sai_policer_api;
extern sai_switch_api_t *sai_switch_api;

struct CoppTest : public ::testing::Test
{

    CoppTest()
    {
    }
    ~CoppTest()
    {
    }

    void SetUp() override
    {
        set_hostif_group_attr_count = 0;
        set_hostif_attr_count = 0;
        memset(set_hostif_group_attr_list, 0, sizeof(set_hostif_group_attr_list));
        memset(set_hostif_attr_list, 0, sizeof(set_hostif_attr_list));
    }
};

class CoppOrchExtend : public CoppOrch
{
  public:
    CoppOrchExtend(DBConnector *db, string tableName) : CoppOrch(db, tableName)
    {
    }
    task_process_status processCoppRule(Consumer &consumer)
    {
        CoppOrch::processCoppRule(consumer);
    }
    bool createPolicer(string trap_group, vector<sai_attribute_t> &policer_attribs)
    {
        CoppOrch::createPolicer(trap_group, policer_attribs);
    }
    bool removePolicer(string trap_group_name)
    {
        CoppOrch::removePolicer(trap_group_name);
    }
};

class ConsumerExtend : public Consumer
{
  public:
    ConsumerExtend(ConsumerTableBase *select, Orch *orch, const string &name) : Consumer(select, orch, name)
    {
    }

    size_t addToSync(std::deque<KeyOpFieldsValuesTuple> &entries)
    {
        Consumer::addToSync(entries);
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

int fake_create_hostif_table_entry(sai_object_id_t *hostif_trap_id,
                                   sai_object_id_t switch_id,
                                   uint32_t attr_count,
                                   const sai_attribute_t *attr_list)
{
    return SAI_STATUS_SUCCESS;
}

int fake_create_hostif_trap_group(sai_object_id_t *hostif_trap_group_id,
                                  sai_object_id_t switch_id,
                                  uint32_t attr_count,
                                  const sai_attribute_t *attr_list)
{
    set_hostif_group_attr_count = attr_count;
    *hostif_trap_group_id = 12345l;
    memcpy(set_hostif_group_attr_list, attr_list, sizeof(sai_attribute_t) * attr_count);
    return SAI_STATUS_SUCCESS;
}

int fake_remove_hostif_trap_group(sai_object_id_t hostif_trap_group_id)
{
    return SAI_STATUS_SUCCESS;
}

int fake_create_hostif_trap(sai_object_id_t *hostif_trap_id,
                            sai_object_id_t switch_id,
                            uint32_t attr_count,
                            const sai_attribute_t *attr_list)
{
    set_hostif_attr_count = attr_count;
    *hostif_trap_id = 12345l;
    memcpy(set_hostif_attr_list, attr_list, sizeof(sai_attribute_t) * attr_count);
    return SAI_STATUS_SUCCESS;
}

int fake_get_switch_attribute(sai_object_id_t switch_id,
                              sai_uint32_t attr_count,
                              sai_attribute_t *attr_list)
{
    return SAI_STATUS_SUCCESS;
}

bool verify_group_attrs(vector<sai_attribute_t> &expect_table_attrs, sai_attribute_t *verify_attrs, uint32_t verify_count)
{
    bool result = false;
    if (expect_table_attrs.size() != verify_count && expect_table_attrs.size() != 1)
        return false;

    sai_attribute_t expected_value =  expect_table_attrs.at(0);
    sai_attribute_t verify_value =  verify_attrs[0];

    if (expected_value.id != SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE)
        return false;

    if (verify_value.id != SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE)
        return false;

    if (expected_value.value.u32 != verify_value.value.u32)
        return false;
    return true;
}

bool verify_attrs(vector<sai_attribute_t> &expect_table_attrs, sai_attribute_t *verify_attrs, uint32_t verify_count)
{
    bool result = false;
    if (expect_table_attrs.size() != verify_count)
        return false;

    for (auto it : expect_table_attrs)
    {
        bool find = false;
        sai_attribute_t target_attribute;

        if(it.id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE)
            continue;

        if(it.id == SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP)
            continue;

        for (int j = 0; j < verify_count; j++)
        {
            if (it.id == verify_attrs[j].id)
            {
                target_attribute = verify_attrs[j];
                find = true;
                break;
            }
        }
        if (!find)
        {
            return false;
        }
        else
        {
            switch (it.id)
            {
            case SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY:
                if (it.value.u32 != target_attribute.value.u32)
                    return false;
                break;
            case SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION:
                if (it.value.s32 != target_attribute.value.s32)
                    return false;
            }
        }
    }
    return true;
}

TEST_F(CoppTest, create_copp_rule_without_policer)
{
    sai_hostif_api = new sai_hostif_api_t();
    sai_switch_api = new sai_switch_api_t();

    sai_hostif_api->create_hostif_trap_group = fake_create_hostif_trap_group;
    sai_hostif_api->create_hostif_trap = fake_create_hostif_trap;
    sai_hostif_api->create_hostif_table_entry = fake_create_hostif_table_entry;
    sai_switch_api->get_switch_attribute = fake_get_switch_attribute;

    DBConnector *appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 1);
    CoppOrchExtend *copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    ConsumerExtend *consumer = new ConsumerExtend(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1), copp_orch_extend, APP_COPP_TABLE_NAME);

    std::deque<KeyOpFieldsValuesTuple> setData = {};
    KeyOpFieldsValuesTuple groupAttr("copp1", "SET", {{"trap_ids", "stp,lacp,eapol"}, {"trap_action", "drop"}, {"queue", "3"}, {"trap_priority", "1"}});
    setData.push_back(groupAttr);

    //prepare expect data
    uint32_t expected_trap_group_attr_count = 1;
    uint32_t expected_trap_attr_count = 2;
    vector<sai_attribute_t> expected_trap_group_attr_list;
    vector<sai_attribute_t> expected_trap_attr_list;

    //prepare expect group data
    sai_attribute_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE;
    attr.value.u32 = 3;
    expected_trap_group_attr_list.push_back(attr);

    //prepare expect trap data
    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE;
    attr.value.s32 = 3;
    expected_trap_attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP;
    attr.value.s32 = 3;
    expected_trap_attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION;
    attr.value.s32 = 0;
    expected_trap_attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY;
    attr.value.u32 = 1;
    expected_trap_attr_list.push_back(attr);

    //call CoPP function
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    //verify
    EXPECT_EQ(1, set_hostif_group_attr_count);
    auto b_ret = verify_group_attrs(expected_trap_group_attr_list, set_hostif_group_attr_list, set_hostif_group_attr_count);
    ASSERT_EQ(b_ret, true);

    EXPECT_EQ(4, set_hostif_attr_count);
    b_ret = verify_attrs(expected_trap_attr_list, set_hostif_attr_list, set_hostif_attr_count);
    ASSERT_EQ(b_ret, true);

    //teardown
    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    sai_hostif_api->create_hostif_table_entry = NULL;
    sai_switch_api->get_switch_attribute = NULL;
    delete sai_hostif_api;
    delete sai_switch_api;
}

TEST_F(CoppTest, delete_copp_rule_without_policer)
{
    sai_hostif_api = new sai_hostif_api_t();
    sai_switch_api = new sai_switch_api_t();

    sai_hostif_api->remove_hostif_trap_group = fake_remove_hostif_trap_group;
    sai_hostif_api->create_hostif_trap_group = fake_create_hostif_trap_group;
    sai_hostif_api->create_hostif_trap = fake_create_hostif_trap;
    sai_hostif_api->create_hostif_table_entry = fake_create_hostif_table_entry;
    sai_switch_api->get_switch_attribute = fake_get_switch_attribute;

    DBConnector *appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 1);
    CoppOrchExtend *copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    ConsumerExtend *consumer = new ConsumerExtend(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1), copp_orch_extend, APP_COPP_TABLE_NAME);

    std::deque<KeyOpFieldsValuesTuple> setData = {};
    KeyOpFieldsValuesTuple addAttr("copp1", "SET", {{"trap_ids", "stp,lacp,eapol"}, {"trap_action", "drop"}, {"queue", "3"}, {"trap_priority", "1"}});        
    setData.push_back(addAttr);

    //prepare expect data
    uint32_t expected_trap_group_attr_count = 1;
    uint32_t expected_trap_attr_count = 2;
    vector<sai_attribute_t> expected_trap_group_attr_list;
    vector<sai_attribute_t> expected_trap_attr_list;

    //prepare expect group data
    sai_attribute_t attr;
    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE;
    attr.value.u32 = 3;
    expected_trap_group_attr_list.push_back(attr);

    //prepare expect trap data
    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE;
    attr.value.s32 = 3;
    expected_trap_attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP;
    attr.value.s32 = 0;
    expected_trap_attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION;
    attr.value.s32 = SAI_PACKET_ACTION_FORWARD;
    expected_trap_attr_list.push_back(attr);

    //call CoPP function
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    KeyOpFieldsValuesTuple delAttr("copp1", "DEL", {{"trap_ids", "stp,lacp,eapol"}});
    setData.clear();
    setData.push_back(delAttr);

    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    //verify
    EXPECT_EQ(3, set_hostif_attr_count);
    auto b_ret = verify_attrs(expected_trap_attr_list, set_hostif_attr_list, set_hostif_attr_count);
    ASSERT_EQ(b_ret, true);

    //teardown
    sai_hostif_api->remove_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    sai_hostif_api->create_hostif_table_entry = NULL;
    sai_switch_api->get_switch_attribute = NULL;
    delete sai_hostif_api;
    delete sai_switch_api;
}