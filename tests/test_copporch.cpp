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

class ConsumerExtend : public Consumer {
public:
    ConsumerExtend(ConsumerTableBase *select, Orch *orch, const string &name) :
     Consumer(select, orch, name) {
    }

    size_t addToSync(std::deque<KeyOpFieldsValuesTuple> &entries){
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
        const sai_attribute_t *attr_list) {
            return SAI_STATUS_SUCCESS;
        }

int fake_create_hostif_trap_group(sai_object_id_t *hostif_trap_group_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t *attr_list) {
            set_hostif_group_attr_count = attr_count;
            memcpy(set_hostif_group_attr_list, attr_list, sizeof(sai_attribute_t)*attr_count);
            return SAI_STATUS_SUCCESS;
        }

int fake_create_hostif_trap(sai_object_id_t *hostif_trap_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t *attr_list) {
            set_hostif_attr_count = attr_count;
            memcpy(set_hostif_attr_list, attr_list, sizeof(sai_attribute_t)*attr_count);
            return SAI_STATUS_SUCCESS;
        }

int fake_get_switch_attribute(sai_object_id_t switch_id,
        sai_uint32_t attr_count,
        sai_attribute_t *attr_list){
            return SAI_STATUS_SUCCESS;
        }

TEST_F(CoppTest, create_hostif_group) {
    sai_hostif_api = new sai_hostif_api_t();
    sai_switch_api = new sai_switch_api_t();

    sai_hostif_api->create_hostif_trap_group = fake_create_hostif_trap_group;
    sai_hostif_api->create_hostif_trap = fake_create_hostif_trap;
    sai_hostif_api->create_hostif_table_entry = fake_create_hostif_table_entry;
    sai_switch_api->get_switch_attribute = fake_get_switch_attribute;

    DBConnector *appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 1);
    CoppOrchExtend *copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    ConsumerExtend *consumer = new ConsumerExtend(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1) ,copp_orch_extend, APP_COPP_TABLE_NAME);
 
    // set trap group expected data
    uint32_t expected_hostif_group_attr_count = 1;

    // set grap expected data
    uint32_t expected_hostif_attr_count = 1;    

    std::deque<KeyOpFieldsValuesTuple> setData = {};
    KeyOpFieldsValuesTuple groupAttr("copp1", "SET", {{"trap_ids", "stp,lacp,eapol"}, {"trap_action", "drop"}, {"queue", "3"}});
    setData.push_back(groupAttr);
   
    //call CoPP function
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);
 
    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    sai_hostif_api->create_hostif_table_entry = NULL;
    sai_switch_api->get_switch_attribute = NULL;
    delete sai_hostif_api;
    delete sai_switch_api;
}