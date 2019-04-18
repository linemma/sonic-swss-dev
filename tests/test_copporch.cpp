#include "gtest/gtest.h"

#include "hiredis.h"
#include "orchdaemon.h"
#include "saihelper.h"

#include "consumertablebase.h"
#include "saiattributelist.h"

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

extern sai_hostif_api_t* sai_hostif_api;
extern sai_policer_api_t* sai_policer_api;
extern sai_switch_api_t* sai_switch_api;

class CoppOrchExtend : public CoppOrch {
public:
    CoppOrchExtend(DBConnector* db, string tableName)
        : CoppOrch(db, tableName)
    {
    }
    task_process_status processCoppRule(Consumer& consumer)
    {
        CoppOrch::processCoppRule(consumer);
    }
    bool createPolicer(string trap_group, vector<sai_attribute_t>& policer_attribs)
    {
        CoppOrch::createPolicer(trap_group, policer_attribs);
    }
    bool removePolicer(string trap_group_name)
    {
        CoppOrch::removePolicer(trap_group_name);
    }
};

class ConsumerExtend : public Consumer {
public:
    ConsumerExtend(ConsumerTableBase* select, Orch* orch, const string& name)
        : Consumer(select, orch, name)
    {
    }

    size_t addToSync(std::deque<KeyOpFieldsValuesTuple>& entries)
    {
        Consumer::addToSync(entries);
    }

    void clear()
    {
        Consumer::m_toSync.clear();
    }
};

struct CreateCoppResult {
    bool ret_val;

    std::vector<sai_attribute_t> group_attr_list;
    std::vector<sai_attribute_t> trap_attr_list;

    void reset() {
        group_attr_list.clear();
        trap_attr_list.clear();
    }
};

struct TestBase : public ::testing::Test {
    static sai_status_t sai_create_hostif_table_entry_(sai_object_id_t* hostif_trap_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_hostif_table_entry_fn(hostif_trap_id, switch_id, attr_count,
            attr_list);
    }

    static sai_status_t sai_create_hostif_trap_group_(sai_object_id_t* hostif_trap_group_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_hostif_trap_group_fn(hostif_trap_group_id, switch_id, attr_count,
            attr_list);
    }

    static sai_status_t sai_set_hostif_trap_group_attribute_(sai_object_id_t hostif_trap_group_id, const sai_attribute_t* attr_list)
    {
        return that->sai_set_hostif_trap_group_attribute_fn(hostif_trap_group_id, attr_list);
    }

    static sai_status_t sai_remove_hostif_trap_group_(sai_object_id_t hostif_trap_group_id)
    {
        return that->sai_remove_hostif_trap_group_fn(hostif_trap_group_id);
    }

    static sai_status_t sai_create_hostif_trap_(sai_object_id_t* hostif_trap_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_hostif_trap_fn(hostif_trap_id, switch_id, attr_count, attr_list);
    }

    static sai_status_t sai_get_switch_attribute_(sai_object_id_t switch_id,
        sai_uint32_t attr_count,
        sai_attribute_t* attr_list)
    {
        return that->sai_get_switch_attribute_fn(switch_id, attr_count, attr_list);
    }

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        sai_create_hostif_table_entry_fn;

    std::function<sai_status_t(sai_object_id_t*,
        sai_object_id_t,
        uint32_t,
        const sai_attribute_t*)>
        sai_create_hostif_trap_group_fn;

    std::function<sai_status_t(sai_object_id_t, const sai_attribute_t*)>
        sai_set_hostif_trap_group_attribute_fn;

    std::function<sai_status_t(sai_object_id_t)>
        sai_remove_hostif_trap_group_fn;

    std::function<sai_status_t(sai_object_id_t*,
        sai_object_id_t,
        uint32_t,
        const sai_attribute_t*)>
        sai_create_hostif_trap_fn;

    std::function<sai_status_t(sai_object_id_t,
        sai_uint32_t,
        sai_attribute_t*)>
        sai_get_switch_attribute_fn;

    bool AttrListEq(const std::vector<sai_attribute_t>& act_attr_list, /*const*/ SaiAttributeList& exp_attr_list)
    {
        if (act_attr_list.size() != exp_attr_list.get_attr_count()) {
            return false;
        }

        auto l = exp_attr_list.get_attr_list();
        for (int i = 0; i < exp_attr_list.get_attr_count(); ++i) {
            // sai_attribute_t* ptr = &l[i];
            // sai_attribute_t& ref = l[i];
            auto found = std::find_if(act_attr_list.begin(), act_attr_list.end(), [&](const sai_attribute_t& attr) {
                if (attr.id != l[i].id) {
                    return false;
                }

                // FIXME: find a way to conver attribute id to type
                // type = idToType(attr.id) // metadata ..
                // switch (type) {
                //     case SAI_ATTR_VALUE_TYPE_BOOL:
                //     ...
                // }

                return true;
            });

            if (found == act_attr_list.end()) {
                std::cout << "Can not found " << l[i].id;
                // TODO: Show act_attr_list
                // TODO: Show exp_attr_list
                return false;
            }
        }

        return true;
    }
};

TestBase* TestBase::that = nullptr;

struct CoppTest : public TestBase {

    CoppTest()
    {
        sai_hostif_api = new sai_hostif_api_t();
        sai_switch_api = new sai_switch_api_t();
    }
    ~CoppTest()
    {
    }

    void SetUp() override
    {
    }
};

TEST_F(CoppTest, create_copp_stp_rule_without_policer)
{
    sai_hostif_api->create_hostif_trap_group = sai_create_hostif_trap_group_;
    sai_hostif_api->create_hostif_trap = sai_create_hostif_trap_;
    sai_hostif_api->create_hostif_table_entry = sai_create_hostif_table_entry_;
    sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;

    that = this;

    auto ret = std::make_shared<CreateCoppResult>();

    sai_create_hostif_trap_group_fn =
        [&](sai_object_id_t* hostif_trap_group_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        for (auto i = 0; i < attr_count; ++i) {
            ret->group_attr_list.emplace_back(attr_list[i]);
        }
        return SAI_STATUS_SUCCESS;
    };

    sai_create_hostif_trap_fn =
        [&](sai_object_id_t* hostif_trap_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        bool defaultTrap = false;
        for (auto i = 0; i < attr_count; ++i) {
            if (attr_list[i].id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE) {
                if (attr_list[i].value.s32 == SAI_HOSTIF_TRAP_TYPE_TTL_ERROR) {
                    defaultTrap = true;
                    break;
                }
            }
        }

        if (!defaultTrap) {
            // FIXME: should not hard code !!
            *hostif_trap_id = 12345l;
            for (auto i = 0; i < attr_count; ++i) {
                ret->trap_attr_list.emplace_back(attr_list[i]);
            }
        }
        return SAI_STATUS_SUCCESS;
    };

    sai_create_hostif_table_entry_fn =
        [&](sai_object_id_t* hostif_trap_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        return SAI_STATUS_SUCCESS;
    };

    sai_get_switch_attribute_fn =
        [&](sai_object_id_t switch_id,
            sai_uint32_t attr_count,
            sai_attribute_t* attr_list) -> sai_status_t {
        return SAI_STATUS_SUCCESS;
    };

    DBConnector* appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 1);
    CoppOrchExtend* copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    ConsumerExtend* consumer = new ConsumerExtend(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1), copp_orch_extend, APP_COPP_TABLE_NAME);

    std::deque<KeyOpFieldsValuesTuple> setData = {};

    setData.clear();
    consumer->clear();
    KeyOpFieldsValuesTuple rule1Attr("coppRule1", "SET", { { "trap_ids", "stp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } });
    setData.push_back(rule1Attr);

    auto groupValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", "1" } });
    SaiAttributeList group_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, groupValue, false);

    auto trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "1" }, { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", "1" } });
    SaiAttributeList trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

    //call CoPP function
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    //verify
    ASSERT_TRUE(AttrListEq(ret->group_attr_list, group_attr_list));
    ASSERT_TRUE(AttrListEq(ret->trap_attr_list, trap_attr_list));

    //teardown
    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    sai_hostif_api->create_hostif_table_entry = NULL;
    sai_switch_api->get_switch_attribute = NULL;
    delete sai_hostif_api;
    delete sai_switch_api;
    delete appl_db;
    delete copp_orch_extend;
    delete consumer;
}

TEST_F(CoppTest, delete_copp_stp_rule_without_policer)
{
    sai_hostif_api->create_hostif_trap_group = sai_create_hostif_trap_group_;
    sai_hostif_api->remove_hostif_trap_group = sai_remove_hostif_trap_group_;
    sai_hostif_api->create_hostif_trap = sai_create_hostif_trap_;
    sai_hostif_api->create_hostif_table_entry = sai_create_hostif_table_entry_;
    sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;

    that = this;

    auto ret = std::make_shared<CreateCoppResult>();

    sai_create_hostif_trap_group_fn =
        [&](sai_object_id_t* hostif_trap_group_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        for (auto i = 0; i < attr_count; ++i) {
            ret->group_attr_list.emplace_back(attr_list[i]);
        }
        *hostif_trap_group_id = 12345l;
        return SAI_STATUS_SUCCESS;
    };

    sai_remove_hostif_trap_group_fn =
        [&](sai_object_id_t hostif_trap_group_id) -> sai_status_t {
        return SAI_STATUS_SUCCESS;
    };

    sai_create_hostif_trap_fn =
        [&](sai_object_id_t* hostif_trap_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        bool defaultTrap = false;
        for (auto i = 0; i < attr_count; ++i) {
            if (attr_list[i].id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE) {
                if (attr_list[i].value.s32 == SAI_HOSTIF_TRAP_TYPE_TTL_ERROR)
                    defaultTrap = true;
                break;
            }
        }

        if (!defaultTrap) {
            // FIXME: should not hard code !!
            *hostif_trap_id = 12345l;
            for (auto i = 0; i < attr_count; ++i) {
                ret->trap_attr_list.emplace_back(attr_list[i]);
            }
        }
        return SAI_STATUS_SUCCESS;
    };

    sai_create_hostif_table_entry_fn =
        [&](sai_object_id_t* hostif_trap_id,
            sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        return SAI_STATUS_SUCCESS;
    };

    sai_get_switch_attribute_fn =
        [&](sai_object_id_t switch_id,
            sai_uint32_t attr_count,
            sai_attribute_t* attr_list) -> sai_status_t {
        return SAI_STATUS_SUCCESS;
    };

    DBConnector* appl_db = new DBConnector(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 1);
    CoppOrchExtend* copp_orch_extend = new CoppOrchExtend(appl_db, APP_COPP_TABLE_NAME);
    ConsumerExtend* consumer = new ConsumerExtend(new ConsumerStateTable(appl_db, APP_COPP_TABLE_NAME, 1, 1), copp_orch_extend, APP_COPP_TABLE_NAME);

    std::deque<KeyOpFieldsValuesTuple> setData = {};

    setData.clear();
    consumer->clear();
    KeyOpFieldsValuesTuple addRuleAttr("coppRule1", "SET", { { "trap_ids", "stp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } });
    setData.push_back(addRuleAttr);

    auto groupValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", "1" } });
    SaiAttributeList group_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, groupValue, false);

    auto trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "1" }, { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", "1" } });
    SaiAttributeList trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

    //call CoPP function
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    ASSERT_TRUE(AttrListEq(ret->group_attr_list, group_attr_list));
    ASSERT_TRUE(AttrListEq(ret->trap_attr_list, trap_attr_list));

    KeyOpFieldsValuesTuple delAttr("coppRule1", "DEL", { { "trap_ids", "stp" } });
    setData.clear();
    setData.push_back(delAttr);

    trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "1" }, { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x1" } });
    SaiAttributeList delete_trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

    //reset list data on create rule
    ret->reset();

    consumer->clear();
    consumer->addToSync(setData);
    copp_orch_extend->processCoppRule(*consumer);

    //verify
    ASSERT_TRUE(AttrListEq(ret->trap_attr_list, delete_trap_attr_list));

    //teardown
    sai_hostif_api->create_hostif_trap_group = NULL;
    sai_hostif_api->remove_hostif_trap_group = NULL;
    sai_hostif_api->create_hostif_trap = NULL;
    sai_hostif_api->create_hostif_table_entry = NULL;
    sai_switch_api->get_switch_attribute = NULL;
    delete sai_hostif_api;
    delete sai_switch_api;
    delete appl_db;
    delete copp_orch_extend;
    delete consumer;
}