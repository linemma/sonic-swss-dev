#include "gtest/gtest.h"

#include "converter.h"
#include "hiredis.h"
#include "orchdaemon.h"
#include "sai_vs.h"
#include "saihelper.h"

//#include "aclorch.h"
#include "saiattributelist.h"
#include "spec_auto_config.h"

void syncd_apply_view() {}

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

// uint32_t set_attr_count;
// sai_attribute_t set_attr_list[20];
// vector<int32_t> bpoint_list;
// vector<int32_t> range_types_list;

extern CrmOrch* gCrmOrch;
extern PortsOrch* gPortsOrch;
extern RouteOrch* gRouteOrch;
extern IntfsOrch* gIntfsOrch;
extern NeighOrch* gNeighOrch;
extern FdbOrch* gFdbOrch;
extern AclOrch* gAclOrch;
MirrorOrch* gMirrorOrch;
VRFOrch* gVrfOrch;

extern sai_acl_api_t* sai_acl_api;
extern sai_switch_api_t* sai_switch_api;
extern sai_port_api_t* sai_port_api;
extern sai_vlan_api_t* sai_vlan_api;
extern sai_bridge_api_t* sai_bridge_api;
extern sai_route_api_t* sai_route_api;

size_t consumerAddToSync(Consumer* consumer, const std::deque<KeyOpFieldsValuesTuple>& entries)
{
    /* Nothing popped */
    if (entries.empty()) {
        return 0;
    }

    for (auto& entry : entries) {
        string key = kfvKey(entry);
        string op = kfvOp(entry);

        /* If a new task comes or if a DEL task comes, we directly put it into getConsumerTable().m_toSync map */
        if (consumer->m_toSync.find(key) == consumer->m_toSync.end() || op == DEL_COMMAND) {
            consumer->m_toSync[key] = entry;
        }
        /* If an old task is still there, we combine the old task with new task */
        else {
            KeyOpFieldsValuesTuple existing_data = consumer->m_toSync[key];

            auto new_values = kfvFieldsValues(entry);
            auto existing_values = kfvFieldsValues(existing_data);

            for (auto it : new_values) {
                string field = fvField(it);
                string value = fvValue(it);

                auto iu = existing_values.begin();
                while (iu != existing_values.end()) {
                    string ofield = fvField(*iu);
                    if (field == ofield)
                        iu = existing_values.erase(iu);
                    else
                        iu++;
                }
                existing_values.push_back(FieldValueTuple(field, value));
            }
            consumer->m_toSync[key] = KeyOpFieldsValuesTuple(key, op, existing_values);
        }
    }
    return entries.size();
}

// FIXME: chnage to lambda function in SetUp()
const char* profile_get_value(
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char* variable)
{
    // UNREFERENCED_PARAMETER(profile_id);

    if (!strcmp(variable, "SAI_KEY_INIT_CONFIG_FILE")) {
        return "/usr/share/sai_2410.xml"; // FIXME: create a json file, and passing the path into test
    } else if (!strcmp(variable, "KV_DEVICE_MAC_ADDRESS")) {
        return "20:03:04:05:06:00";
    } else if (!strcmp(variable, "SAI_KEY_L3_ROUTE_TABLE_SIZE")) {
        return "1000";
    } else if (!strcmp(variable, "SAI_KEY_L3_NEIGHBOR_TABLE_SIZE")) {
        return "2000";
    } else if (!strcmp(variable, "SAI_VS_SWITCH_TYPE")) {
        return "SAI_VS_SWITCH_TYPE_BCM56850";
    }

    return NULL;
}

// FIXME: chnage to lambda function in SetUp()
static int profile_get_next_value(
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char** variable,
    _Out_ const char** value)
{
    if (value == NULL) {
        return 0;
    }

    if (variable == NULL) {
        return -1;
    }

    return -1;
}

// TODO: move to separted file ?
const map<sai_object_id_t, AclTable>& getAclTables(const AclOrch& orch)
{
    return orch.m_AclTables;
}

sai_object_id_t getAclRuleOid(const AclRule& aclrule)
{
    return aclrule.m_ruleOid;
}

TEST(ConvertTest, field_value_to_attribute)
{
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
        { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
        { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
        { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_ACL_TABLE, v, false);

    auto l = attr_list.get_attr_list();
    auto c = attr_list.get_attr_count();
    ASSERT_TRUE(c == 11);
}

struct CreateAclResult {
    bool ret_val;

    std::vector<sai_attribute_t> attr_list;
};

struct AclTableResult {
    bool ret_val;

    sai_object_id_t acl_table_id;
    std::vector<sai_attribute_t> attr_list;
};

struct CreateRuleResult {
    bool ret_val;

    std::vector<sai_attribute_t> counter_attr_list;
    std::vector<sai_attribute_t> rule_attr_list;
};

struct AclRuleResult {
    bool ret_val;
    sai_object_id_t acl_counter_id;
    sai_object_id_t acl_entry_id;
    std::vector<sai_attribute_t> counter_attr_list;
    std::vector<sai_attribute_t> rule_attr_list;
};

class ConsumerExtend_Dont_Use : public Consumer {
public:
    ConsumerExtend_Dont_Use(ConsumerTableBase* select, Orch* orch, const string& name)
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

struct TestBase : public ::testing::Test {
    //
    // spy functions
    //
    static sai_status_t sai_create_acl_table_(sai_object_id_t* acl_table_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_acl_table_fn(acl_table_id, switch_id, attr_count,
            attr_list);
    }

    static sai_status_t sai_remove_acl_table_(_In_ sai_object_id_t acl_table_id)
    {
        return that->sai_remove_acl_table_fn(acl_table_id);
    }

    static sai_status_t sai_create_acl_counter_(_Out_ sai_object_id_t* acl_counter_id,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t* attr_list)
    {
        return that->sai_create_acl_counter_fn(acl_counter_id, switch_id, attr_count, attr_list);
    }

    static sai_status_t sai_create_acl_entry_(_Out_ sai_object_id_t* acl_entry_id,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t* attr_list)
    {
        return that->sai_create_acl_entry_fn(acl_entry_id, switch_id, attr_count, attr_list);
    }

    static sai_status_t sai_create_switch_(_Out_ sai_object_id_t* switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t* attr_list)
    {
        return that->sai_create_switch_fn(switch_id, attr_count, attr_list);
    }

    static sai_status_t sai_get_switch_attribute_(_In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t* attr_list)
    {
        return that->sai_get_switch_attribute_fn(switch_id, attr_count, attr_list);
    }

    static sai_status_t sai_get_port_attribute_(_In_ sai_object_id_t port_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t* attr_list)
    {
        return that->sai_get_port_attribute_fn(port_id, attr_count, attr_list);
    }

    static sai_status_t sai_get_vlan_attribute_(_In_ sai_object_id_t vlan_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t* attr_list)
    {
        return that->sai_get_vlan_attribute_fn(vlan_id, attr_count, attr_list);
    }

    static sai_status_t sai_remove_vlan_member_(_In_ sai_object_id_t vlan_member_id)
    {
        return that->sai_remove_vlan_member_fn(vlan_member_id);
    }

    static sai_status_t sai_get_bridge_attribute_(_In_ sai_object_id_t bridge_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t* attr_list)
    {
        return that->sai_get_bridge_attribute_fn(bridge_id, attr_count, attr_list);
    }

    static sai_status_t sai_get_bridge_port_attribute_(_In_ sai_object_id_t bridge_port_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t* attr_list)
    {
        return that->sai_get_bridge_port_attribute_fn(bridge_port_id, attr_count, attr_list);
    }

    static sai_status_t sai_remove_bridge_port_(_In_ sai_object_id_t bridge_port_id)
    {
        return that->sai_remove_bridge_port_fn(bridge_port_id);
    }

    static sai_status_t sai_create_route_entry_(_In_ const sai_route_entry_t* route_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t* attr_list)
    {
        return that->sai_create_route_entry_fn(route_entry, attr_count, attr_list);
    }

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        sai_create_acl_table_fn;

    std::function<sai_status_t(sai_object_id_t)>
        sai_remove_acl_table_fn;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t, const sai_attribute_t*)>
        sai_create_acl_counter_fn;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t, const sai_attribute_t*)>
        sai_create_acl_entry_fn;

    std::function<sai_status_t(sai_object_id_t*, uint32_t, const sai_attribute_t*)>
        sai_create_switch_fn;

    std::function<sai_status_t(sai_object_id_t, uint32_t, sai_attribute_t*)>
        sai_get_switch_attribute_fn;

    std::function<sai_status_t(sai_object_id_t, uint32_t, sai_attribute_t*)>
        sai_get_port_attribute_fn;

    std::function<sai_status_t(sai_object_id_t, uint32_t, sai_attribute_t*)>
        sai_get_vlan_attribute_fn;

    std::function<sai_status_t(sai_object_id_t)>
        sai_remove_vlan_member_fn;

    std::function<sai_status_t(sai_object_id_t, uint32_t, sai_attribute_t*)>
        sai_get_bridge_attribute_fn;

    std::function<sai_status_t(sai_object_id_t, uint32_t, sai_attribute_t*)>
        sai_get_bridge_port_attribute_fn;

    std::function<sai_status_t(sai_object_id_t)>
        sai_remove_bridge_port_fn;

    std::function<sai_status_t(const sai_route_entry_t*, uint32_t, const sai_attribute_t*)>
        sai_create_route_entry_fn;

    //
    // validation functions (NO NEED TO put into Test class => move to Validation class)
    //
    bool AttrListEq_Miss_objecttype_Dont_Use(const std::vector<sai_attribute_t>& act_attr_list, /*const*/ SaiAttributeList& exp_attr_list)
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

    bool AttrListEq(sai_object_type_t objecttype, const std::vector<sai_attribute_t>& act_attr_list, /*const*/ SaiAttributeList& exp_attr_list)
    {
        if (act_attr_list.size() != exp_attr_list.get_attr_count()) {
            return false;
        }

        auto l = exp_attr_list.get_attr_list();
        for (int i = 0; i < exp_attr_list.get_attr_count(); ++i) {
            sai_attr_id_t id = exp_attr_list.get_attr_list()[i].id;
            auto meta = sai_metadata_get_attr_metadata(objecttype, id);

            assert(meta != nullptr);

            char act_buf[0x4000];
            char exp_buf[0x4000];

            auto act_len = sai_serialize_attribute_value(act_buf, meta, &act_attr_list[i].value);
            auto exp_len = sai_serialize_attribute_value(exp_buf, meta, &exp_attr_list.get_attr_list()[i].value);

            // auto act = sai_serialize_attr_value(*meta, act_attr_list[i].value, false);
            // auto exp = sai_serialize_attr_value(*meta, &exp_attr_list.get_attr_list()[i].value, false);

            assert(act_len < sizeof(act_buf));
            assert(exp_len < sizeof(exp_buf));

            if (act_len != exp_len) {
                std::cout << "AttrListEq failed\n";
                std::cout << "Actual:   " << act_buf << "\n";
                std::cout << "Expected: " << exp_buf << "\n";
                return false;
            }

            if (strcmp(act_buf, exp_buf) != 0) {
                std::cout << "AttrListEq failed\n";
                std::cout << "Actual:   " << act_buf << "\n";
                std::cout << "Expected: " << exp_buf << "\n";
                return false;
            }
        }

        return true;
    }
};

TestBase* TestBase::that = nullptr;

struct AclTestBase : public TestBase {
    std::vector<int32_t*> m_s32list_pool;

    virtual ~AclTestBase()
    {
        for (auto p : m_s32list_pool) {
            free(p);
        }
    }
};

struct AclTest : public AclTestBase {

    // std::shared_ptr<swss::DBConnector> m_app_db;
    std::shared_ptr<swss::DBConnector> m_config_db;
    // std::shared_ptr<swss::DBConnector> m_state_db;
    // sai_object_id_t m_acl_table_num;
    // sai_object_id_t m_acl_entry_num;
    // sai_object_id_t m_acl_counter_num;

    // std::vector<int32_t*> m_s32list_pool;

    AclTest()
    {
        // m_app_db = std::make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_config_db = std::make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        // m_state_db = std::make_shared<swss::DBConnector>(STATE_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);

        // m_acl_table_num = 0;
        // m_acl_entry_num = 0;
        // m_acl_counter_num = 0;
    }

    // virtual ~AclTest()
    // {
    //     for (auto p : m_s32list_pool) {
    //         free(p);
    //     }
    // }

    // static void SetUpTestCase()
    // {
    //     //system(REDIS_START_CMD);
    // }
    //
    // static void TearDownTestCase()
    // {
    //     //system(REDIS_STOP_CMD);
    // }

    void SetUp() override
    {
        // assert(gAclOrch == nullptr);
        // assert(gFdbOrch == nullptr);
        // assert(gMirrorOrch == nullptr);
        // assert(gRouteOrch == nullptr);
        // assert(gNeighOrch == nullptr);
        // assert(gIntfsOrch == nullptr);
        // assert(gVrfOrch == nullptr);
        // assert(gCrmOrch == nullptr);
        // assert(gPortsOrch == nullptr);

        // assert(sai_switch_api == nullptr);
        // assert(sai_port_api == nullptr);
        // assert(sai_vlan_api == nullptr);
        // assert(sai_bridge_api == nullptr);
        // assert(sai_route_api == nullptr);

        // // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_switch = std::shared_ptr<sai_switch_api_t>(new sai_switch_api_t(), [](sai_switch_api_t* p) {
        //     delete p;
        //     sai_switch_api = nullptr;
        // });

        // // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_port = std::shared_ptr<sai_port_api_t>(new sai_port_api_t(), [](sai_port_api_t* p) {
        //     delete p;
        //     sai_port_api = nullptr;
        // });

        // // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_vlan = std::shared_ptr<sai_vlan_api_t>(new sai_vlan_api_t(), [](sai_vlan_api_t* p) {
        //     delete p;
        //     sai_vlan_api = nullptr;
        // });

        // // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_bridge = std::shared_ptr<sai_bridge_api_t>(new sai_bridge_api_t(), [](sai_bridge_api_t* p) {
        //     delete p;
        //     sai_bridge_api = nullptr;
        // });

        // // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_route = std::shared_ptr<sai_route_api_t>(new sai_route_api_t(), [](sai_route_api_t* p) {
        //     delete p;
        //     sai_route_api = nullptr;
        // });

        // // FIXME: Change the following function to "stub" or "dummy" (just return fixed value), just interact with AclTable / AclRule
        // sai_switch_api = sai_switch.get();
        // sai_port_api = sai_port.get();
        // sai_vlan_api = sai_vlan.get();
        // sai_bridge_api = sai_bridge.get();
        // sai_route_api = sai_route.get();

        // // TODO: change these functions .... for init only ??
        // sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;
        // sai_port_api->get_port_attribute = sai_get_port_attribute_;
        // sai_vlan_api->get_vlan_attribute = sai_get_vlan_attribute_;
        // sai_vlan_api->remove_vlan_member = sai_remove_vlan_member_;
        // sai_bridge_api->get_bridge_attribute = sai_get_bridge_attribute_;
        // sai_bridge_api->get_bridge_port_attribute = sai_get_bridge_port_attribute_;
        // sai_bridge_api->remove_bridge_port = sai_remove_bridge_port_;
        // sai_route_api->create_route_entry = sai_create_route_entry_;
        // that = this;

        // sai_create_switch_fn =
        //     [](_Out_ sai_object_id_t* switch_id,
        //         _In_ uint32_t attr_count,
        //         _In_ const sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_get_switch_attribute_fn =
        //     [](_In_ sai_object_id_t switch_id,
        //         _In_ uint32_t attr_count,
        //         _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_get_port_attribute_fn =
        //     [](_In_ sai_object_id_t port_id,
        //         _In_ uint32_t attr_count,
        //         _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_get_vlan_attribute_fn =
        //     [](_In_ sai_object_id_t vlan_id,
        //         _In_ uint32_t attr_count,
        //         _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_remove_vlan_member_fn =
        //     [](_In_ sai_object_id_t vlan_member_id) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_get_bridge_attribute_fn =
        //     [](_In_ sai_object_id_t bridge_id,
        //         _In_ uint32_t attr_count,
        //         _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_get_bridge_port_attribute_fn =
        //     [](_In_ sai_object_id_t bridge_port_id,
        //         _In_ uint32_t attr_count,
        //         _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_remove_bridge_port_fn =
        //     [](_In_ sai_object_id_t bridge_port_id) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // sai_create_route_entry_fn =
        //     [](_In_ const sai_route_entry_t* route_entry,
        //         _In_ uint32_t attr_count,
        //         _In_ const sai_attribute_t* attr_list) -> sai_status_t {
        //     return SAI_STATUS_SUCCESS;
        // };

        // TableConnector confDbAclTable(m_config_db.get(), CFG_ACL_TABLE_NAME);
        // TableConnector confDbAclRuleTable(m_config_db.get(), CFG_ACL_RULE_TABLE_NAME);

        // const int portsorch_base_pri = 40;

        // vector<table_name_with_pri_t> ports_tables = {
        //     { APP_PORT_TABLE_NAME, portsorch_base_pri + 5 },
        //     { APP_VLAN_TABLE_NAME, portsorch_base_pri + 2 },
        //     { APP_VLAN_MEMBER_TABLE_NAME, portsorch_base_pri },
        //     { APP_LAG_TABLE_NAME, portsorch_base_pri + 4 },
        //     { APP_LAG_MEMBER_TABLE_NAME, portsorch_base_pri }
        // };

        // // FIXME: doesn't use global variable !!
        // assert(gPortsOrch == nullptr);
        // gPortsOrch = new PortsOrch(m_app_db.get(), ports_tables);

        // FIXME: doesn't use global variable !!
        assert(gCrmOrch == nullptr);
        gCrmOrch = new CrmOrch(m_config_db.get(), CFG_CRM_TABLE_NAME);

        // // FIXME: doesn't use global variable !!
        // assert(gVrfOrch == nullptr);
        // gVrfOrch = new VRFOrch(m_app_db.get(), APP_VRF_TABLE_NAME);

        // // FIXME: doesn't use global variable !!
        // assert(gIntfsOrch == nullptr);
        // gIntfsOrch = new IntfsOrch(m_app_db.get(), APP_INTF_TABLE_NAME, gVrfOrch);

        // // FIXME: doesn't use global variable !!
        // assert(gNeighOrch == nullptr);
        // gNeighOrch = new NeighOrch(m_app_db.get(), APP_NEIGH_TABLE_NAME, gIntfsOrch);

        // // FIXME: doesn't use global variable !!
        // assert(gRouteOrch == nullptr);
        // gRouteOrch = new RouteOrch(m_app_db.get(), APP_ROUTE_TABLE_NAME, gNeighOrch);

        // TableConnector applDbFdb(m_app_db.get(), APP_FDB_TABLE_NAME);
        // TableConnector stateDbFdb(m_state_db.get(), STATE_FDB_TABLE_NAME);

        // // FIXME: doesn't use global variable !!
        // assert(gFdbOrch == nullptr);
        // gFdbOrch = new FdbOrch(applDbFdb, stateDbFdb, gPortsOrch);

        // TableConnector stateDbMirrorSession(m_state_db.get(), APP_MIRROR_SESSION_TABLE_NAME);
        // TableConnector confDbMirrorSession(m_config_db.get(), CFG_MIRROR_SESSION_TABLE_NAME);

        // // FIXME: doesn't use global variable !!
        // assert(gMirrorOrch == nullptr);
        // gMirrorOrch = new MirrorOrch(stateDbMirrorSession, confDbMirrorSession,
        //     gPortsOrch, gRouteOrch, gNeighOrch, gFdbOrch);

        // vector<TableConnector> acl_table_connectors = { confDbAclTable, confDbAclRuleTable };

        // // FIXME: Using local variable or data member for aclorch ... ??
        // gAclOrch = new AclOrch(acl_table_connectors, gPortsOrch, gMirrorOrch,
        //     gNeighOrch, gRouteOrch);

        // auto consumerStateTable = new ConsumerStateTable(m_app_db.get(), APP_PORT_TABLE_NAME, 1, 1); // free by consumerStateTable
        // auto consumerExt = std::make_shared<ConsumerExtend_Dont_Use>(consumerStateTable, gPortsOrch, APP_PORT_TABLE_NAME);

        // auto setData = std::deque<KeyOpFieldsValuesTuple>(
        //     { { "PortInitDone",
        //         EMPTY_PREFIX,
        //         { { "", "" } } } });
        // consumerExt->addToSync(setData);

        // Consumer* consumer = consumerExt.get();
        // static_cast<Orch*>(gPortsOrch)->doTask(*consumer);
    }

    void TearDown() override
    {
        // delete gAclOrch; // FIXME: using auto ptr
        // gAclOrch = nullptr;
        // delete gFdbOrch; // FIXME: using auto ptr
        // gFdbOrch = nullptr;
        // delete gMirrorOrch; // FIXME: using auto ptr
        // gMirrorOrch = nullptr;
        // delete gRouteOrch; // FIXME: using auto ptr
        // gRouteOrch = nullptr;
        // delete gNeighOrch; // FIXME: using auto ptr
        // gNeighOrch = nullptr;
        // delete gIntfsOrch; // FIXME: using auto ptr
        // gIntfsOrch = nullptr;
        // delete gVrfOrch; // FIXME: using auto ptr
        // gVrfOrch = nullptr;
        delete gCrmOrch; // FIXME: using auto ptr
        gCrmOrch = nullptr;
        // delete gPortsOrch; // FIXME: using auto ptr
        // gPortsOrch = nullptr;
    }

    std::shared_ptr<CreateAclResult> createAclTable_4(AclTable& acl)
    {
        assert(sai_acl_api == nullptr);

        sai_acl_api = new sai_acl_api_t();
        auto sai_acl = std::shared_ptr<sai_acl_api_t>(sai_acl_api, [](sai_acl_api_t* p) {
            delete p;
            sai_acl_api = nullptr;
        });

        sai_acl_api->create_acl_table = sai_create_acl_table_;
        that = this;

        auto ret = std::make_shared<CreateAclResult>();

        sai_create_acl_table_fn =
            [&](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            // return SAI_STATUS_FAILURE;
            return SAI_STATUS_SUCCESS;
        };

        ret->ret_val = acl.create();
        return ret;
    }
};

TEST_F(AclTest, create_default_acl_table_4)
{
    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    auto res = createAclTable_4(acltable);

    ASSERT_TRUE(res->ret_val == true);

    auto v = std::vector<swss::FieldValueTuple>(
        { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
            { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
            { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
            { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_ACL_TABLE, v, false);

    ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

struct AclTestRedis_Old_Test_Refine_Then_Remove : public ::testing::Test {
    AclTestRedis_Old_Test_Refine_Then_Remove() {}

    void start_server_and_remote_all_data()
    {
        //.....
    }

    // override
    void SetUp() override { start_server_and_remote_all_data(); }

    void InjectData(int instance, void* data)
    {
        if (instance == APPL_DB) {
            ///
        } else if (instance == CONFIG_DB) {
            ///
        } else if (instance == STATE_DB) {
            ///
        }
    }

    int GetData(int instance) { return 0; }
};

TEST_F(AclTestRedis_Old_Test_Refine_Then_Remove, create_default_acl_table_on_redis)
{
    sai_status_t status;
    AclTable acltable;

    // DBConnector appl_db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    DBConnector config_db(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
    // DBConnector state_db(STATE_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);

    initSaiApi();
    gCrmOrch = new CrmOrch(&config_db, CFG_CRM_TABLE_NAME);

    sai_attribute_t attr;
    vector<sai_attribute_t> attrs;
    attr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    attr.value.booldata = true;
    attrs.push_back(attr);

    status = sai_switch_api->create_switch(&gSwitchId, (uint32_t)attrs.size(),
        attrs.data());
    ASSERT_EQ(status, SAI_STATUS_SUCCESS);
    sleep(1);

    acltable.create();
    sleep(2);
    // validate ...
    // auto x = GetData(ASIC_DB);
    {
        redisContext* c;
        redisReply* reply;

        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        c = redisConnectUnixWithTimeout(DBConnector::DEFAULT_UNIXSOCKET, timeout);
        if (c == NULL || c->err) {
            ASSERT_TRUE(0);
        }

        reply = (redisReply*)redisCommand(c, "SELECT %d", ASIC_DB);
        ASSERT_NE(reply->type, REDIS_REPLY_ERROR);
        // printf("SELECT: %s\n", reply->str);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, " LRANGE %s 0 -1",
            "ASIC_STATE_KEY_VALUE_OP_QUEUE");
        ASSERT_NE(reply->type, REDIS_REPLY_ERROR);
        ASSERT_EQ(reply->elements, 6);
        for (int i = 0; i < reply->elements; ++i) {
            redisReply* sub_reply = reply->element[i];
            // printf("(%d)LRANGE: %s\n", i, sub_reply->str);

            if (i == 0) {
                string op = string("Screate");
                ASSERT_TRUE(0 == strncmp(op.c_str(), sub_reply->str, op.length()));
            }
        }
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "FLUSHALL");
        freeReplyObject(reply);
        redisFree(c);
    }

    delete gCrmOrch;
    sai_api_uninitialize();
}

struct AclOrchTest : public AclTest {
    // std::shared_ptr<AclTableResult> createAclTable_DONT_USE(Consumer& consumer)
    // {
    //     auto ret = std::make_shared<AclTableResult>();
    //
    //     assert(sai_acl_api == nullptr);
    //
    //     auto sai_acl = std::shared_ptr<sai_acl_api_t>(new sai_acl_api_t(), [](sai_acl_api_t* p) {
    //         delete p;
    //         sai_acl_api = nullptr;
    //     });
    //
    //     sai_acl_api = sai_acl.get();
    //     sai_acl_api->create_acl_table = sai_create_acl_table_;
    //
    //     sai_create_acl_table_fn =
    //         [&](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
    //             uint32_t attr_count,
    //             const sai_attribute_t* attr_list) -> sai_status_t {
    //         *acl_table_id = (++m_acl_table_num);
    //         for (auto i = 0; i < attr_count; ++i) {
    //             ret->attr_list.emplace_back(attr_list[i]);
    //         }
    //         ret->acl_table_id = *acl_table_id;
    //         ret->ret_val = true;
    //         return SAI_STATUS_SUCCESS;
    //     };
    //
    //     // FIXME: passing aclorch or using self member data (DON'T using global variable)
    //     static_cast<Orch*>(gAclOrch)->doTask(consumer);
    //
    //     return ret;
    // }
    //
    // std::shared_ptr<AclTableResult> deleteAclTable_DONT_USE(Consumer& consumer)
    // {
    //     auto ret = std::make_shared<AclTableResult>();
    //
    //     assert(sai_acl_api == nullptr);
    //
    //     auto sai_acl = std::shared_ptr<sai_acl_api_t>(new sai_acl_api_t(), [](sai_acl_api_t* p) {
    //         delete p;
    //         sai_acl_api = nullptr;
    //     });
    //
    //     sai_acl_api = sai_acl.get();
    //     sai_acl_api->remove_acl_table = sai_remove_acl_table_;
    //
    //     sai_remove_acl_table_fn =
    //         [&](sai_object_id_t acl_table_id) -> sai_status_t {
    //         if (acl_table_id != m_acl_table_num)
    //             return SAI_STATUS_FAILURE;
    //
    //         ret->acl_table_id = acl_table_id;
    //         ret->ret_val = true;
    //         return SAI_STATUS_SUCCESS;
    //     };
    //
    //     // FIXME: passing aclorch or using self member data (DON'T using global variable)
    //     static_cast<Orch*>(gAclOrch)->doTask(consumer);
    //
    //     return ret;
    // }
    //
    // std::shared_ptr<AclRuleResult> createAclRule_DONT_USE(Consumer& consumer)
    // {
    //     auto ret = std::make_shared<AclRuleResult>();
    //
    //     assert(sai_acl_api == nullptr);
    //
    //     auto sai_acl = std::shared_ptr<sai_acl_api_t>(new sai_acl_api_t(), [](sai_acl_api_t* p) {
    //         delete p;
    //         sai_acl_api = nullptr;
    //     });
    //
    //     sai_acl_api = sai_acl.get();
    //     sai_acl_api->create_acl_counter = sai_create_acl_counter_;
    //     sai_acl_api->create_acl_entry = sai_create_acl_entry_;
    //
    //     sai_create_acl_counter_fn =
    //         [&](_Out_ sai_object_id_t* acl_counter_id,
    //             _In_ sai_object_id_t switch_id,
    //             _In_ uint32_t attr_count,
    //             _In_ const sai_attribute_t* attr_list) -> sai_status_t {
    //         *acl_counter_id = (++m_acl_counter_num);
    //         for (auto i = 0; i < attr_count; ++i) {
    //             ret->counter_attr_list.emplace_back(attr_list[i]);
    //         }
    //         ret->acl_counter_id = *acl_counter_id;
    //         return SAI_STATUS_SUCCESS;
    //     };
    //
    //     sai_create_acl_entry_fn =
    //         [&](_Out_ sai_object_id_t* acl_entry_id,
    //             _In_ sai_object_id_t switch_id,
    //             _In_ uint32_t attr_count,
    //             _In_ const sai_attribute_t* attr_list) -> sai_status_t {
    //         *acl_entry_id = (++m_acl_entry_num);
    //         for (auto i = 0; i < attr_count; ++i) {
    //             ret->rule_attr_list.emplace_back(attr_list[i]);
    //         }
    //         ret->acl_entry_id = *acl_entry_id;
    //         ret->ret_val = true;
    //         return SAI_STATUS_SUCCESS;
    //     };
    //
    //     // FIXME: passing aclorch or using self member data (DON'T using global variable)
    //     static_cast<Orch*>(gAclOrch)->doTask(consumer);
    //
    //     return ret;
    // }

    std::shared_ptr<swss::DBConnector> m_app_db;
    std::shared_ptr<swss::DBConnector> m_config_db;
    std::shared_ptr<swss::DBConnector> m_state_db;

    AclOrchTest()
    {
        m_app_db = std::make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_config_db = std::make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_state_db = std::make_shared<swss::DBConnector>(STATE_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    void SetUp() override
    {
        AclTestBase::SetUp();

        assert(gAclOrch == nullptr);
        assert(gFdbOrch == nullptr);
        assert(gMirrorOrch == nullptr);
        assert(gRouteOrch == nullptr);
        assert(gNeighOrch == nullptr);
        assert(gIntfsOrch == nullptr);
        assert(gVrfOrch == nullptr);
        assert(gCrmOrch == nullptr);
        assert(gPortsOrch == nullptr);

        ///////////////////////////////////////////////////////////////////////
        sai_service_method_table_t test_services = {
            profile_get_value,
            profile_get_next_value
        };

        auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

        // FIXME: using clone not just assign
        sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);

        // FIXME: using clone not just assign
        sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);

        sai_attribute_t swattr;

        swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
        swattr.value.booldata = true;

        status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
        ///////////////////////////////////////////////////////////////////////

        // assert(sai_switch_api == nullptr);
        assert(sai_port_api == nullptr);
        assert(sai_vlan_api == nullptr);
        assert(sai_bridge_api == nullptr);
        assert(sai_route_api == nullptr);

        // FIXME: BUG ! the scope is not correct ! why not error ?
        // auto sai_switch = std::shared_ptr<sai_switch_api_t>(new sai_switch_api_t(), [](sai_switch_api_t* p) {
        //     delete p;
        //     sai_switch_api = nullptr;
        // });

        // FIXME: BUG ! the scope is not correct ! why not error ?
        auto sai_port = std::shared_ptr<sai_port_api_t>(new sai_port_api_t(), [](sai_port_api_t* p) {
            delete p;
            sai_port_api = nullptr;
        });

        // FIXME: BUG ! the scope is not correct ! why not error ?
        auto sai_vlan = std::shared_ptr<sai_vlan_api_t>(new sai_vlan_api_t(), [](sai_vlan_api_t* p) {
            delete p;
            sai_vlan_api = nullptr;
        });

        // FIXME: BUG ! the scope is not correct ! why not error ?
        auto sai_bridge = std::shared_ptr<sai_bridge_api_t>(new sai_bridge_api_t(), [](sai_bridge_api_t* p) {
            delete p;
            sai_bridge_api = nullptr;
        });

        // FIXME: BUG ! the scope is not correct ! why not error ?
        auto sai_route = std::shared_ptr<sai_route_api_t>(new sai_route_api_t(), [](sai_route_api_t* p) {
            delete p;
            sai_route_api = nullptr;
        });

        // FIXME: Change the following function to "stub" or "dummy" (just return fixed value), just interact with AclTable / AclRule
        // sai_switch_api = sai_switch.get();
        sai_port_api = sai_port.get();
        sai_vlan_api = sai_vlan.get();
        sai_bridge_api = sai_bridge.get();
        sai_route_api = sai_route.get();

        // TODO: change these functions .... for init only ??
        // sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;
        sai_port_api->get_port_attribute = sai_get_port_attribute_;
        sai_vlan_api->get_vlan_attribute = sai_get_vlan_attribute_;
        sai_vlan_api->remove_vlan_member = sai_remove_vlan_member_;
        sai_bridge_api->get_bridge_attribute = sai_get_bridge_attribute_;
        sai_bridge_api->get_bridge_port_attribute = sai_get_bridge_port_attribute_;
        sai_bridge_api->remove_bridge_port = sai_remove_bridge_port_;
        sai_route_api->create_route_entry = sai_create_route_entry_;
        that = this;

        sai_create_switch_fn =
            [](_Out_ sai_object_id_t* switch_id,
                _In_ uint32_t attr_count,
                _In_ const sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_get_switch_attribute_fn =
            [](_In_ sai_object_id_t switch_id,
                _In_ uint32_t attr_count,
                _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_get_port_attribute_fn =
            [](_In_ sai_object_id_t port_id,
                _In_ uint32_t attr_count,
                _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_get_vlan_attribute_fn =
            [](_In_ sai_object_id_t vlan_id,
                _In_ uint32_t attr_count,
                _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_remove_vlan_member_fn =
            [](_In_ sai_object_id_t vlan_member_id) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_get_bridge_attribute_fn =
            [](_In_ sai_object_id_t bridge_id,
                _In_ uint32_t attr_count,
                _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_get_bridge_port_attribute_fn =
            [](_In_ sai_object_id_t bridge_port_id,
                _In_ uint32_t attr_count,
                _Inout_ sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_remove_bridge_port_fn =
            [](_In_ sai_object_id_t bridge_port_id) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        sai_create_route_entry_fn =
            [](_In_ const sai_route_entry_t* route_entry,
                _In_ uint32_t attr_count,
                _In_ const sai_attribute_t* attr_list) -> sai_status_t {
            return SAI_STATUS_SUCCESS;
        };

        TableConnector confDbAclTable(m_config_db.get(), CFG_ACL_TABLE_NAME);
        TableConnector confDbAclRuleTable(m_config_db.get(), CFG_ACL_RULE_TABLE_NAME);

        const int portsorch_base_pri = 40;

        vector<table_name_with_pri_t> ports_tables = {
            { APP_PORT_TABLE_NAME, portsorch_base_pri + 5 },
            { APP_VLAN_TABLE_NAME, portsorch_base_pri + 2 },
            { APP_VLAN_MEMBER_TABLE_NAME, portsorch_base_pri },
            { APP_LAG_TABLE_NAME, portsorch_base_pri + 4 },
            { APP_LAG_MEMBER_TABLE_NAME, portsorch_base_pri }
        };

        // FIXME: doesn't use global variable !!
        assert(gPortsOrch == nullptr);
        gPortsOrch = new PortsOrch(m_app_db.get(), ports_tables);

        // FIXME: doesn't use global variable !!
        assert(gCrmOrch == nullptr);
        gCrmOrch = new CrmOrch(m_config_db.get(), CFG_CRM_TABLE_NAME);

        // FIXME: doesn't use global variable !!
        assert(gVrfOrch == nullptr);
        gVrfOrch = new VRFOrch(m_app_db.get(), APP_VRF_TABLE_NAME);

        // FIXME: doesn't use global variable !!
        assert(gIntfsOrch == nullptr);
        gIntfsOrch = new IntfsOrch(m_app_db.get(), APP_INTF_TABLE_NAME, gVrfOrch);

        // FIXME: doesn't use global variable !!
        assert(gNeighOrch == nullptr);
        gNeighOrch = new NeighOrch(m_app_db.get(), APP_NEIGH_TABLE_NAME, gIntfsOrch);

        // FIXME: doesn't use global variable !!
        assert(gRouteOrch == nullptr);
        gRouteOrch = new RouteOrch(m_app_db.get(), APP_ROUTE_TABLE_NAME, gNeighOrch);

        TableConnector applDbFdb(m_app_db.get(), APP_FDB_TABLE_NAME);
        TableConnector stateDbFdb(m_state_db.get(), STATE_FDB_TABLE_NAME);

        // FIXME: doesn't use global variable !!
        assert(gFdbOrch == nullptr);
        gFdbOrch = new FdbOrch(applDbFdb, stateDbFdb, gPortsOrch);

        TableConnector stateDbMirrorSession(m_state_db.get(), APP_MIRROR_SESSION_TABLE_NAME);
        TableConnector confDbMirrorSession(m_config_db.get(), CFG_MIRROR_SESSION_TABLE_NAME);

        // FIXME: doesn't use global variable !!
        assert(gMirrorOrch == nullptr);
        gMirrorOrch = new MirrorOrch(stateDbMirrorSession, confDbMirrorSession,
            gPortsOrch, gRouteOrch, gNeighOrch, gFdbOrch);

        vector<TableConnector> acl_table_connectors = { confDbAclTable, confDbAclRuleTable };

        // FIXME: Using local variable or data member for aclorch ... ??
        gAclOrch = new AclOrch(acl_table_connectors, gPortsOrch, gMirrorOrch,
            gNeighOrch, gRouteOrch);

        auto consumerStateTable = new ConsumerStateTable(m_app_db.get(), APP_PORT_TABLE_NAME, 1, 1); // free by consumerStateTable
        auto consumerExt = std::make_shared<ConsumerExtend_Dont_Use>(consumerStateTable, gPortsOrch, APP_PORT_TABLE_NAME);

        auto setData = std::deque<KeyOpFieldsValuesTuple>(
            { { "PortInitDone",
                EMPTY_PREFIX,
                { { "", "" } } } });
        consumerExt->addToSync(setData);

        Consumer* consumer = consumerExt.get();
        static_cast<Orch*>(gPortsOrch)->doTask(*consumer);
    }

    void TearDown() override
    {
        AclTestBase::TearDown();

        delete gAclOrch; // FIXME: using auto ptr
        gAclOrch = nullptr;
        delete gFdbOrch; // FIXME: using auto ptr
        gFdbOrch = nullptr;
        delete gMirrorOrch; // FIXME: using auto ptr
        gMirrorOrch = nullptr;
        delete gRouteOrch; // FIXME: using auto ptr
        gRouteOrch = nullptr;
        delete gNeighOrch; // FIXME: using auto ptr
        gNeighOrch = nullptr;
        delete gIntfsOrch; // FIXME: using auto ptr
        gIntfsOrch = nullptr;
        delete gVrfOrch; // FIXME: using auto ptr
        gVrfOrch = nullptr;
        delete gCrmOrch; // FIXME: using auto ptr
        gCrmOrch = nullptr;
        delete gPortsOrch; // FIXME: using auto ptr
        gPortsOrch = nullptr;

        ///////////////////////////////////////////////////////////////////////

        auto status = sai_switch_api->remove_switch(gSwitchId);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
        gSwitchId = 0;

        sai_api_uninitialize();

        sai_switch_api = nullptr;
        sai_acl_api = nullptr;
    }

    std::shared_ptr<SaiAttributeList> GetAclTableAttributeList(sai_object_type_t objecttype, const AclTable& acl_table)
    {
        // const sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
        std::vector<swss::FieldValueTuple> fields;

        switch (acl_table.type) {
        case ACL_TABLE_L3:
            // sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
            // auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            //     { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
            //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
            //         { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
            // // SaiAttributeList exp_attrlist(objecttype, exp_fields, false);

            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" });

            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" });
            break;

        case ACL_TABLE_L3V6:
            // auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            // { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6", "true" },
            //     //                          ^^^^^^^^ sip v6
            //     { "SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6", "true" },
            //     //                          ^^^^^^^^ dip v6
            //     { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
            //     { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
            //     { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });

            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6", "true" });

            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" });
            break;

        default:
            assert(false);
        }

        return std::shared_ptr<SaiAttributeList>(new SaiAttributeList(objecttype, fields, false));
    }

    std::shared_ptr<SaiAttributeList> GetAclRuleAttributeList(sai_object_type_t objecttype, const AclRule& acl_rule, sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        std::vector<swss::FieldValueTuple> fields;

        auto table_id = sai_serialize_object_id(acl_table_oid);
        auto counter_id = sai_serialize_object_id(const_cast<AclRule&>(acl_rule).getCounterOid()); // FIXME: getcounterOid() should be const

        switch (acl_table.type) {
        case ACL_TABLE_L3:
            //     auto table_id = sai_serialize_object_id(acl_table_oid);
            //     auto counter_id = sai_serialize_object_id(acl_rule->getCounterOid());
            //
            //     sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY; // <----------
            //     auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            //         {
            //             { "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id },
            //             { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
            //             { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
            //             { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id },
            //
            //             // cfg fields
            //             { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "1.2.3.4&mask:255.255.255.255" },
            //             { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" }
            //             //                                            SAI_PACKET_ACTION_FORWARD
            //
            //         });
            //     SaiAttributeList exp_attrlist(objecttype, exp_fields, false);

            fields.push_back({ "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id });

            fields.push_back({ "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "1.2.3.4&mask:255.255.255.255" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" });
            break;

        case ACL_TABLE_L3V6:
            // auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            // {
            //     { "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id },
            //     { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
            //     { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
            //     { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id },
            //
            //     // cfg fields
            //     { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6", "::1.2.3.4&mask:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" },
            //     { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" }
            //     //                                            SAI_PACKET_ACTION_FORWARD
            //
            // });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id });

            fields.push_back({ "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6", "::1.2.3.4&mask:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" });
            fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" });
            break;

        default:
            assert(false);
        }

        return std::shared_ptr<SaiAttributeList>(new SaiAttributeList(objecttype, fields, false));
    }

    bool ValidateAclRule(const std::string acl_rule_sid, const AclRule& acl_rule, sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY; // <----------
        auto exp_attrlist_2 = GetAclRuleAttributeList(objecttype, acl_rule, acl_table_oid, acl_table);

        // auto it = acl_tables.find(acl_table_oid);
        // ASSERT_TRUE(it != acl_tables.end());

        // //auto acl_rule_oid = it->second.rules.begin()->first;
        // auto acl_rule = it->second.rules.begin()->second; // FIXME: assumpt only one rule inside
        auto acl_rule_oid = getAclRuleOid(acl_rule);

        {
            //     auto table_id = sai_serialize_object_id(acl_table_oid);
            //     auto counter_id = sai_serialize_object_id(acl_rule->getCounterOid());
            //
            //     sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY; // <----------
            //     auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            //         {
            //             { "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id },
            //             { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
            //             { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
            //             { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id },
            //
            //             // cfg fields
            //             { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "1.2.3.4&mask:255.255.255.255" },
            //             { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" }
            //             //                                            SAI_PACKET_ACTION_FORWARD
            //
            //         });
            //     SaiAttributeList exp_attrlist(objecttype, exp_fields, false);
            auto& exp_attrlist = *exp_attrlist_2;

            std::vector<sai_attribute_t> act_attr;

            for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
                const auto attr = exp_attrlist.get_attr_list()[i];
                auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

                // ASSERT_TRUE(meta != nullptr);
                if (meta == nullptr) {
                    return false;
                }

                sai_attribute_t new_attr = { 0 };
                new_attr.id = attr.id;

                switch (meta->attrvaluetype) {
                case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                    new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                    new_attr.value.s32list.count = attr.value.s32list.count;
                    m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                    break;

                default:
                    std::cout << "";
                    ;
                }

                act_attr.emplace_back(new_attr);
            }

            auto status = sai_acl_api->get_acl_entry_attribute(acl_rule_oid, act_attr.size(), act_attr.data()); // <----------
            // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
            if (status != SAI_STATUS_SUCCESS) {
                return false;
            }

            // ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
            auto b_attr_eq = AttrListEq(objecttype, act_attr, exp_attrlist);
            if (!b_attr_eq) {
                return false;
            }
        }

        return true;
    }

    bool ValidateAclTable(sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        const sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
        auto exp_attrlist_2 = GetAclTableAttributeList(objecttype, acl_table);

        {
            //     sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
            //     auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            //         { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
            //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
            //             { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
            //     SaiAttributeList exp_attrlist(objecttype, exp_fields, false);
            auto& exp_attrlist = *exp_attrlist_2;

            std::vector<sai_attribute_t> act_attr;

            for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
                const auto attr = exp_attrlist.get_attr_list()[i];
                auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

                //ASSERT_TRUE(meta != nullptr);
                if (meta == nullptr) {
                    return false;
                }

                sai_attribute_t new_attr = { 0 };
                new_attr.id = attr.id;

                switch (meta->attrvaluetype) {
                case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                    new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                    new_attr.value.s32list.count = attr.value.s32list.count;
                    m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                    break;

                default:
                    std::cout << "";
                    ;
                }

                act_attr.emplace_back(new_attr);
            }

            auto status = sai_acl_api->get_acl_table_attribute(acl_table_oid, act_attr.size(), act_attr.data()); // <----------
            // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
            if (status != SAI_STATUS_SUCCESS) {
                return false;
            }

            // ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
            auto b_attr_eq = AttrListEq(objecttype, act_attr, exp_attrlist);
            if (!b_attr_eq) {
                return false;
            }
        }

        for (const auto& sid_acl_rule : acl_table.rules) {
            auto b_valid = ValidateAclRule(sid_acl_rule.first, *sid_acl_rule.second, acl_table_oid, acl_table);
            if (!b_valid) {
                return false;
            }
        }

        return true;
    }

    bool Validate(const AclOrch* orch)
    {
        assert(orch != nullptr);

        const auto& acl_tables = getAclTables(*gAclOrch);

        for (const auto& id_acl_table : acl_tables) {
            if (!ValidateAclTable(id_acl_table.first, id_acl_table.second)) {
                return false;
            }
        }

        return true;
    }
};

// TEST_F(AclTest, create_l3_rule_filter_sip)
// {
//     //createL3AclTableAndRule
//     AclTable aclTable;
//     aclTable.type = ACL_TABLE_L3;
//
//     auto talbe_ret = createAclTable(aclTable);
//     ASSERT_TRUE(talbe_ret->ret_val == true);
//
//     auto v = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
//             { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
//     SaiAttributeList attr_list(SAI_OBJECT_TYPE_ACL_TABLE, v, false);
//
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(talbe_ret->attr_list, attr_list));
//
//     string rule_id("acl_rule_l3");
//
//     auto filedValues = std::vector<swss::FieldValueTuple>(
//         { { ACTION_PACKET_ACTION, PACKET_ACTION_FORWARD },
//             { MATCH_SRC_IP, "10.0.0.1" } });
//
//     auto rule_ret = createRuleToAcl(aclTable, rule_id, filedValues);
//     ASSERT_TRUE(rule_ret->ret_val == true);
//
//     auto counter = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_COUNTER_ATTR_TABLE_ID", "oid:0x1" },
//             { "SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT", "true" },
//             { "SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT", "true" } });
//     SaiAttributeList counter_attr_list(SAI_OBJECT_TYPE_ACL_COUNTER, counter, false);
//     auto rule = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_ENTRY_ATTR_TABLE_ID", "oid:0x1" },
//             { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
//             { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
//             { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", "disabled" },
//             { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" },
//             { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "10.0.0.1&mask:255.255.255.255" } });
//     SaiAttributeList rule_attr_list(SAI_OBJECT_TYPE_ACL_ENTRY, rule, false);
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(rule_ret->counter_attr_list, counter_attr_list));
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(rule_ret->rule_attr_list, rule_attr_list));
// }
//
// TEST_F(AclOrchTest, createL3AclTable_will_remove)
// {
//     std::string acl_table_name = "acl_table_1";
//     auto consumerStateTable = new ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1); // free by consumerStateTable
//     auto consumerExt = std::make_shared<ConsumerExtend_Dont_Use>(consumerStateTable, gAclOrch, CFG_ACL_TABLE_NAME);
//     auto setData = std::deque<KeyOpFieldsValuesTuple>(
//         { { acl_table_name,
//             SET_COMMAND,
//             { { TABLE_DESCRIPTION, "filter source IP" },
//                 { TABLE_TYPE, TABLE_TYPE_L3 },
//                 { TABLE_STAGE, TABLE_INGRESS },
//                 { TABLE_PORTS, "1,2" } } } });
//     consumerExt->addToSync(setData);
//     Consumer* consumer = consumerExt.get();
//
//     // TODO: vs create_default_acl_table_4
//     auto ret = createAclTable_DONT_USE(*consumer);
//     ASSERT_TRUE(ret->ret_val == true);
//
//     auto v = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
//             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
//             { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
//     SaiAttributeList attr_list(SAI_OBJECT_TYPE_ACL_TABLE, v, false);
//
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(ret->attr_list, attr_list));
// }
//
// TEST_F(AclOrchTest, deleteL3AclTable_will_remove)
// {
//     std::string acl_table_name = "acl_table_1";
//     auto consumerStateTable = new ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1); // free by consumerStateTable
//     auto consumerExt = std::make_shared<ConsumerExtend_Dont_Use>(consumerStateTable, gAclOrch, CFG_ACL_TABLE_NAME);
//     auto setData = std::deque<KeyOpFieldsValuesTuple>(
//         { { acl_table_name,
//             SET_COMMAND,
//             { { TABLE_DESCRIPTION, "filter source IP" },
//                 { TABLE_TYPE, TABLE_TYPE_L3 },
//                 { TABLE_STAGE, TABLE_INGRESS },
//                 { TABLE_PORTS, "1,2" } } } });
//     consumerExt->addToSync(setData);
//     Consumer* consumer = consumerExt.get();
//
//     auto ret = createAclTable_DONT_USE(*consumer);
//     ASSERT_TRUE(ret->ret_val == true);
//
//     auto setData2 = std::deque<KeyOpFieldsValuesTuple>(
//         { { acl_table_name, DEL_COMMAND, {} } });
//     consumerExt->addToSync(setData2);
//
//     auto ret2 = deleteAclTable_DONT_USE(*consumer);
//     ASSERT_TRUE(ret2->ret_val == true);
// }
//
// TEST_F(AclOrchTest, createL3AclRule_will_remove)
// {
//     std::string acl_table_name = "acl_table_1";
//     auto consumerStateTable = new ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1); // free by consumerStateTable
//     auto consumerExt = std::make_shared<ConsumerExtend_Dont_Use>(consumerStateTable, gAclOrch, CFG_ACL_TABLE_NAME);
//     auto setData = std::deque<KeyOpFieldsValuesTuple>(
//         { { acl_table_name,
//             SET_COMMAND,
//             { { TABLE_DESCRIPTION, "filter source IP" },
//                 { TABLE_TYPE, TABLE_TYPE_L3 },
//                 { TABLE_STAGE, TABLE_INGRESS },
//                 { TABLE_PORTS, "1,2" } } } });
//     consumerExt->addToSync(setData);
//     Consumer* consumer = consumerExt.get();
//
//     auto ret = createAclTable_DONT_USE(*consumer);
//     ASSERT_TRUE(ret->ret_val == true);
//
//     auto ruleConsumerStateTable = new ConsumerStateTable(m_config_db.get(), CFG_ACL_RULE_TABLE_NAME, 1, 1); // free by consumerStateTable
//     auto ruleConsumerExt = std::make_shared<ConsumerExtend_Dont_Use>(ruleConsumerStateTable, gAclOrch, CFG_ACL_RULE_TABLE_NAME);
//
//     auto setRuleData = std::deque<KeyOpFieldsValuesTuple>(
//         { { "RULE_SIP_1",
//             SET_COMMAND,
//             { { ACTION_PACKET_ACTION, PACKET_ACTION_FORWARD },
//                 { MATCH_SRC_IP, "10.0.0.1" } } } });
//     ruleConsumerExt->addToSync(setRuleData);
//     consumer = ruleConsumerExt.get();
//
//     // FIXME: still failed to call gAclOrch to create rule
//     // FIXME: this is not correct passing ref converted from *pointer and point to derived class
//     //                                                       ^-- the ref will ref to nullptr !
//     //                                                                             ^-- ref to base class, that just all base function not derived class
//     auto ruleRet = createAclRule_DONT_USE(*consumer);
//     ASSERT_TRUE(ruleRet->ret_val == true);
//
//     auto counter = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_COUNTER_ATTR_TABLE_ID", "oid:0x1" },
//             { "SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT", "true" },
//             { "SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT", "true" } });
//     SaiAttributeList counter_attr_list(SAI_OBJECT_TYPE_ACL_COUNTER, counter, false);
//     auto rule = std::vector<swss::FieldValueTuple>(
//         { { "SAI_ACL_ENTRY_ATTR_TABLE_ID", "oid:0x1" },
//             { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
//             { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
//             { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", "disabled" },
//             { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" },
//             { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "10.0.0.1&mask:255.255.255.255" } });
//     SaiAttributeList rule_attr_list(SAI_OBJECT_TYPE_ACL_ENTRY, rule, false);
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(ruleRet->counter_attr_list, counter_attr_list));
//     ASSERT_TRUE(AttrListEq_Miss_objecttype_Dont_Use(ruleRet->rule_attr_list, rule_attr_list));
// }

TEST_F(AclOrchTest, Create_L3Acl_Table)
{
    // sai_service_method_table_t test_services = {
    //     profile_get_value,
    //     profile_get_next_value
    // };
    //
    // auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
    // sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);
    //
    // sai_attribute_t swattr;
    //
    // swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    // swattr.value.booldata = true;
    //
    // status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: vs create_default_acl_table_4
    std::string acl_table_name = "acl_table_1";

    auto consumer = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_TABLE_NAME));

    auto setData = std::deque<KeyOpFieldsValuesTuple>(
        { { acl_table_name,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3 },
                //            ^^^^^^^^^^^^^ L3 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    consumerAddToSync(consumer.get(), setData);

    ///////////////////////////////////////////////////////////////////////////

    static_cast<Orch*>(gAclOrch)->doTask(*consumer);
    auto id = gAclOrch->getTableById(acl_table_name);
    ASSERT_TRUE(id != SAI_NULL_OBJECT_ID);

    const auto& acl_tables = getAclTables(*gAclOrch);
    ASSERT_TRUE(acl_tables.at(id).type == ACL_TABLE_L3); // FIXME: using find() instead at()
    ASSERT_TRUE(acl_tables.at(id).stage == ACL_STAGE_INGRESS);

    // auto exp_fields = std::vector<swss::FieldValueTuple>(
    //     { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
    //         { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    // SaiAttributeList exp_attrlist(SAI_OBJECT_TYPE_ACL_TABLE, exp_fields, false);
    //
    // sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE;
    //
    // std::vector<sai_attribute_t> act_attr;
    //
    // for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
    //     const auto attr = exp_attrlist.get_attr_list()[i];
    //     auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);
    //
    //     ASSERT_TRUE(meta != nullptr);
    //
    //     sai_attribute_t new_attr = { 0 };
    //     new_attr.id = attr.id;
    //
    //     switch (meta->attrvaluetype) {
    //     case SAI_ATTR_VALUE_TYPE_INT32_LIST:
    //         new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
    //         new_attr.value.s32list.count = attr.value.s32list.count;
    //         m_s32list_pool.emplace_back(new_attr.value.s32list.list);
    //         break;
    //
    //     default:
    //         std::cout << "";
    //         ;
    //     }
    //
    //     act_attr.emplace_back(new_attr);
    // }
    //
    // auto status = sai_acl_api->get_acl_table_attribute(id, act_attr.size(), act_attr.data());
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // ASSERT_TRUE(AttrListEq(SAI_OBJECT_TYPE_ACL_TABLE, act_attr, exp_attrlist));

    Validate(gAclOrch); // <----------

    ///////////////////////////////////////////////////////////////////////////

    // status = sai_switch_api->remove_switch(gSwitchId);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    // gSwitchId = 0;
    //
    // sai_api_uninitialize();
    //
    // sai_switch_api = nullptr;
    // sai_acl_api = nullptr;
}

TEST_F(AclOrchTest, Create_L3v6Acl_Table)
{
    // sai_service_method_table_t test_services = {
    //     profile_get_value,
    //     profile_get_next_value
    // };
    //
    // auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
    // sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);
    //
    // sai_attribute_t swattr;
    //
    // swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    // swattr.value.booldata = true;
    //
    // status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: vs create_default_acl_table_4
    std::string acl_table_name = "acl_table_1";

    auto consumer = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_TABLE_NAME));

    auto setData = std::deque<KeyOpFieldsValuesTuple>(
        { { acl_table_name,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3V6 },
                //            ^^^^^^^^^^^^^^^ L3V6 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    consumerAddToSync(consumer.get(), setData);

    ///////////////////////////////////////////////////////////////////////////

    static_cast<Orch*>(gAclOrch)->doTask(*consumer);
    auto id = gAclOrch->getTableById(acl_table_name);

    ASSERT_TRUE(id != SAI_NULL_OBJECT_ID);

    const auto& acl_tables = getAclTables(*gAclOrch);
    ASSERT_TRUE(acl_tables.at(id).type == ACL_TABLE_L3V6); // FIXME: using find() instead at()
    ASSERT_TRUE(acl_tables.at(id).stage == ACL_STAGE_INGRESS);

    // auto exp_fields = std::vector<swss::FieldValueTuple>(
    //     { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6", "true" },
    //         //                          ^^^^^^^^ sip v6
    //         { "SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6", "true" },
    //         //                          ^^^^^^^^ dip v6
    //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
    //         { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
    //         { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    // SaiAttributeList exp_attrlist(SAI_OBJECT_TYPE_ACL_TABLE, exp_fields, false);
    //
    // sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE;
    //
    // std::vector<sai_attribute_t> act_attr;
    //
    // for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
    //     const auto attr = exp_attrlist.get_attr_list()[i];
    //     auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);
    //
    //     ASSERT_TRUE(meta != nullptr);
    //
    //     sai_attribute_t new_attr = { 0 };
    //     new_attr.id = attr.id;
    //
    //     switch (meta->attrvaluetype) {
    //     case SAI_ATTR_VALUE_TYPE_INT32_LIST:
    //         new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
    //         new_attr.value.s32list.count = attr.value.s32list.count;
    //         m_s32list_pool.emplace_back(new_attr.value.s32list.list);
    //         break;
    //
    //     default:
    //         std::cout << "";
    //         ;
    //     }
    //
    //     act_attr.emplace_back(new_attr);
    // }
    //
    // auto status = sai_acl_api->get_acl_table_attribute(id, act_attr.size(), act_attr.data());
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // ASSERT_TRUE(AttrListEq(SAI_OBJECT_TYPE_ACL_TABLE, act_attr, exp_attrlist));

    Validate(gAclOrch); // <----------

    ///////////////////////////////////////////////////////////////////////////

    // status = sai_switch_api->remove_switch(gSwitchId);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    // gSwitchId = 0;
    // sai_api_uninitialize();
    //
    // sai_switch_api = nullptr;
    // sai_acl_api = nullptr;
}

TEST_F(AclOrchTest, Create_L3Acl_Table_and_then_Add_L3Rule)
{
    // sai_service_method_table_t test_services = {
    //     profile_get_value,
    //     profile_get_next_value
    // };
    //
    // auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
    // sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);
    //
    // sai_attribute_t swattr;
    //
    // swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    // swattr.value.booldata = true;
    //
    // status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: vs create_default_acl_table_4
    std::string acl_table_name = "acl_table_1";
    std::string acl_rule_id = "acl_rule_1";

    auto consumer_acl_table = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_TABLE_NAME));

    auto consumer_acl_rule = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_RULE_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_RULE_TABLE_NAME));

    ///////////////////////////////////////////////////////////////////////////

    auto acl_cfg = std::deque<KeyOpFieldsValuesTuple>(
        { { acl_table_name,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3 },
                //            ^^^^^^^^^^^^^ L3 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    consumerAddToSync(consumer_acl_table.get(), acl_cfg);
    consumerAddToSync(consumer_acl_rule.get(),
        { { acl_table_name + "|" + acl_rule_id, SET_COMMAND,
            { { ACTION_PACKET_ACTION, PACKET_ACTION_FORWARD },

                // if (attr_name == ACTION_PACKET_ACTION || attr_name == ACTION_MIRROR_ACTION ||
                // attr_name == ACTION_DTEL_FLOW_OP || attr_name == ACTION_DTEL_INT_SESSION ||
                // attr_name == ACTION_DTEL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_TAIL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_FLOW_SAMPLE_PERCENT ||
                // attr_name == ACTION_DTEL_REPORT_ALL_PACKETS)
                //
                // TODO: required field (add new test cases for that ....)
                //

                { MATCH_SRC_IP, "1.2.3.4" } } } });

    // TODO: RULE_PRIORITY (important field)
    // TODO: MATCH_DSCP / MATCH_SRC_IPV6 || attr_name == MATCH_DST_IPV6

    // Logger::setMinPrio(Logger::SWSS_DEBUG);

    ///////////////////////////////////////////////////////////////////////////
    for (auto consumer : { consumer_acl_table.get(), consumer_acl_rule.get() }) {
        static_cast<Orch*>(gAclOrch)->doTask(*consumer);
        // Logger::setMinPrio(Logger::SWSS_DEBUG);
    }
    // static_cast<Orch*>(gAclOrch)->doTask(*consumer_acl_table);

    ///////////////////////////////////////////////////////////////////////////

    auto acl_table_oid = gAclOrch->getTableById(acl_table_name);
    const auto& acl_tables = getAclTables(*gAclOrch);

    ASSERT_TRUE(acl_table_oid != SAI_NULL_OBJECT_ID);

    ASSERT_TRUE(acl_tables.at(acl_table_oid).type == ACL_TABLE_L3); // FIXME: using find() instead at()
    ASSERT_TRUE(acl_tables.at(acl_table_oid).stage == ACL_STAGE_INGRESS);

    // acl_table->rules[acl_rule_oid??];
    auto it = acl_tables.find(acl_table_oid);
    ASSERT_TRUE(it != acl_tables.end());

    //auto acl_rule_oid = it->second.rules.begin()->first;
    auto acl_rule = it->second.rules.begin()->second; // FIXME: assumpt only one rule inside
    auto acl_rule_oid = getAclRuleOid(*acl_rule.get());

    // ///////////////////////////////////////////////////////////////////////////
    // {
    //     sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
    //     auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
    //         { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
    //             { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    //     SaiAttributeList exp_attrlist(objecttype, exp_fields, false);
    //
    //     std::vector<sai_attribute_t> act_attr;
    //
    //     for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
    //         const auto attr = exp_attrlist.get_attr_list()[i];
    //         auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);
    //
    //         ASSERT_TRUE(meta != nullptr);
    //
    //         sai_attribute_t new_attr = { 0 };
    //         new_attr.id = attr.id;
    //
    //         switch (meta->attrvaluetype) {
    //         case SAI_ATTR_VALUE_TYPE_INT32_LIST:
    //             new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
    //             new_attr.value.s32list.count = attr.value.s32list.count;
    //             m_s32list_pool.emplace_back(new_attr.value.s32list.list);
    //             break;
    //
    //         default:
    //             std::cout << "";
    //             ;
    //         }
    //
    //         act_attr.emplace_back(new_attr);
    //     }
    //
    //     auto status = sai_acl_api->get_acl_table_attribute(acl_table_oid, act_attr.size(), act_attr.data()); // <----------
    //     ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    //     ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
    // }
    ///////////////////////////////////////////////////////////////////////////
    {
        auto table_id = sai_serialize_object_id(acl_table_oid);
        auto counter_id = sai_serialize_object_id(acl_rule->getCounterOid());

        sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY; // <----------
        auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            {
                { "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id },
                { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
                { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
                { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id },

                // cfg fields
                { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "1.2.3.4&mask:255.255.255.255" },
                { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" }
                //                                            SAI_PACKET_ACTION_FORWARD

            });
        SaiAttributeList exp_attrlist(objecttype, exp_fields, false);

        std::vector<sai_attribute_t> act_attr;

        for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
            const auto attr = exp_attrlist.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

            ASSERT_TRUE(meta != nullptr);

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;

            switch (meta->attrvaluetype) {
            case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                new_attr.value.s32list.count = attr.value.s32list.count;
                m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                break;

            default:
                std::cout << "";
                ;
            }

            act_attr.emplace_back(new_attr);
        }

        auto status = sai_acl_api->get_acl_entry_attribute(acl_rule_oid, act_attr.size(), act_attr.data()); // <----------
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

        ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
    }
    ///////////////////////////////////////////////////////////////////////////

    Validate(gAclOrch); // <----------

    // status = sai_switch_api->remove_switch(gSwitchId);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    // gSwitchId = 0;
    //
    // sai_api_uninitialize();
    //
    // sai_switch_api = nullptr;
    // sai_acl_api = nullptr;
}

TEST_F(AclOrchTest, Create_L3v6Acl_Table_and_then_Add_L3Rule)
{
    // sai_service_method_table_t test_services = {
    //     profile_get_value,
    //     profile_get_next_value
    // };
    //
    // auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    // sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
    // sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);
    //
    // sai_attribute_t swattr;
    //
    // swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    // swattr.value.booldata = true;
    //
    // status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: vs create_default_acl_table_4
    std::string acl_table_name = "acl_table_1";
    std::string acl_rule_id = "acl_rule_1";

    auto consumer_acl_table = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_TABLE_NAME));

    auto consumer_acl_rule = std::unique_ptr<Consumer>(new Consumer(
        new swss::ConsumerStateTable(m_config_db.get(), CFG_ACL_RULE_TABLE_NAME, 1, 1), gAclOrch, CFG_ACL_RULE_TABLE_NAME));

    ///////////////////////////////////////////////////////////////////////////

    auto acl_cfg = std::deque<KeyOpFieldsValuesTuple>(
        { { acl_table_name,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3V6 },
                //            ^^^^^^^^^^^^^^^ L3V6 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    consumerAddToSync(consumer_acl_table.get(), acl_cfg);
    consumerAddToSync(consumer_acl_rule.get(),
        { { acl_table_name + "|" + acl_rule_id, SET_COMMAND,
            { { ACTION_PACKET_ACTION, PACKET_ACTION_FORWARD },

                // if (attr_name == ACTION_PACKET_ACTION || attr_name == ACTION_MIRROR_ACTION ||
                // attr_name == ACTION_DTEL_FLOW_OP || attr_name == ACTION_DTEL_INT_SESSION ||
                // attr_name == ACTION_DTEL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_TAIL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_FLOW_SAMPLE_PERCENT ||
                // attr_name == ACTION_DTEL_REPORT_ALL_PACKETS)
                //
                // TODO: required field (add new test cases for that ....)
                //

                { MATCH_SRC_IPV6, "::1.2.3.4" } } } });

    // TODO: RULE_PRIORITY (important field)
    // TODO: MATCH_DSCP / MATCH_SRC_IPV6 || attr_name == MATCH_DST_IPV6

    // Logger::setMinPrio(Logger::SWSS_DEBUG);

    ///////////////////////////////////////////////////////////////////////////
    for (auto consumer : { consumer_acl_table.get(), consumer_acl_rule.get() }) {
        static_cast<Orch*>(gAclOrch)->doTask(*consumer);
        Logger::setMinPrio(Logger::SWSS_DEBUG);
    }
    // static_cast<Orch*>(gAclOrch)->doTask(*consumer_acl_table);

    ///////////////////////////////////////////////////////////////////////////

    auto acl_table_oid = gAclOrch->getTableById(acl_table_name);
    const auto& acl_tables = getAclTables(*gAclOrch);

    ASSERT_TRUE(acl_table_oid != SAI_NULL_OBJECT_ID);

    ASSERT_TRUE(acl_tables.at(acl_table_oid).type == ACL_TABLE_L3V6); // FIXME: using find() instead at()
    ASSERT_TRUE(acl_tables.at(acl_table_oid).stage == ACL_STAGE_INGRESS);

    // acl_table->rules[acl_rule_oid??];
    auto it = acl_tables.find(acl_table_oid);
    ASSERT_TRUE(it != acl_tables.end());

    //auto acl_rule_oid = it->second.rules.begin()->first;
    auto acl_rule = it->second.rules.begin()->second; // FIXME: assumpt only one rule inside
    auto acl_rule_oid = getAclRuleOid(*acl_rule.get());

    // ///////////////////////////////////////////////////////////////////////////
    // {
    //     sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE; // <----------
    //     auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
    //         { { "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6", "true" },
    //             //                          ^^^^^^^^ sip v6
    //             { "SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6", "true" },
    //             //                          ^^^^^^^^ dip v6
    //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" },
    //             { "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" },
    //             { "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" } });
    //     SaiAttributeList exp_attrlist(objecttype, exp_fields, false);
    //
    //     std::vector<sai_attribute_t> act_attr;
    //
    //     for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
    //         const auto attr = exp_attrlist.get_attr_list()[i];
    //         auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);
    //
    //         ASSERT_TRUE(meta != nullptr);
    //
    //         sai_attribute_t new_attr = { 0 };
    //         new_attr.id = attr.id;
    //
    //         switch (meta->attrvaluetype) {
    //         case SAI_ATTR_VALUE_TYPE_INT32_LIST:
    //             new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
    //             new_attr.value.s32list.count = attr.value.s32list.count;
    //             m_s32list_pool.emplace_back(new_attr.value.s32list.list);
    //             break;
    //
    //         default:
    //             std::cout << "";
    //             ;
    //         }
    //
    //         act_attr.emplace_back(new_attr);
    //     }
    //
    //     auto status = sai_acl_api->get_acl_table_attribute(acl_table_oid, act_attr.size(), act_attr.data()); // <----------
    //     ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    //
    //     ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
    // }
    ///////////////////////////////////////////////////////////////////////////
    {
        auto table_id = sai_serialize_object_id(acl_table_oid);
        auto counter_id = sai_serialize_object_id(acl_rule->getCounterOid());

        sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY; // <----------
        auto exp_fields = std::vector<swss::FieldValueTuple>( // <----------
            {
                { "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id },
                { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
                { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
                { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id },

                // cfg fields
                { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6", "::1.2.3.4&mask:ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" },
                { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "1" }
                //                                            SAI_PACKET_ACTION_FORWARD

            });
        SaiAttributeList exp_attrlist(objecttype, exp_fields, false);

        std::vector<sai_attribute_t> act_attr;

        for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
            const auto attr = exp_attrlist.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

            ASSERT_TRUE(meta != nullptr);

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;

            switch (meta->attrvaluetype) {
            case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                new_attr.value.s32list.count = attr.value.s32list.count;
                m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                break;

            default:
                std::cout << "";
                ;
            }

            act_attr.emplace_back(new_attr);
        }

        auto status = sai_acl_api->get_acl_entry_attribute(acl_rule_oid, act_attr.size(), act_attr.data()); // <----------
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

        ASSERT_TRUE(AttrListEq(objecttype, act_attr, exp_attrlist));
    }
    ///////////////////////////////////////////////////////////////////////////

    Validate(gAclOrch); // <----------

    // status = sai_switch_api->remove_switch(gSwitchId);
    // ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
    // gSwitchId = 0;
    //
    // sai_api_uninitialize();
    //
    // sai_switch_api = nullptr;
    // sai_acl_api = nullptr;
}

/* FIXME: test case pseudo code
// 1. RULE_CTRL_UT_Apply_ACL()
for (type : {MAC, IP_STD, IP_EXTEND, IPv6_STD, IPv6_EXT})
    acl = new ACL()
    for (i : {0..max_number_ace_of_acl})
        ace = new ACE()
        a.addAce(ace)

    //for (port : {1,2,3, max+1, ...})
    //    for (dir : {ingress, egress})
    //        acl.bindTo(port, dir)
    //        validate rule

    //for (port : {1,2,3, max+1, ...})
    //    for (dir : {ingress, egress})
    //        acl.unBindTo(port, dir)
    //        validate rule

    delete acl


// 2. RULE_CTRL_UT_Set_QoS_With_Modifying_ACE_On_Fly()
//Bind Policy-map and ACL on port
p1 = create Policy Map
c = create Class Map
p1.add_class(c)
a2 = create ACL
c.add_acl(a2)

for (port : {1, 2, 3, 4 ...})
    for (dir : {ingress, egress})
        p1.bindTo(port, dir);
        validate rule
//Add on fly
ace = new ACE();
a2.addAce(ace)
validate rule
a2.removeAce(ace)
validate rule


// 3. RULE_OM_TEST_Max_ACE_Of_ACL()
acls = []
for (type : {MAC, IP_STD, IP_EXTEND, IPv6_STD, IPv6_EXT})
    acls[type] = new ACL()

for (type : {MAC, IP_STD, IP_EXTEND, IPv6_STD, IPv6_EXT})
    for (i : {0...max_number_ace-1})
        new_ace = new ACE()
        acls[type].add_ace(new_ace)

    delete acls[type] validate rule


// 4. RULE_OM_TEST_Max_ACE_Of_System()
ace_count = 0
acls = []
while true
    acl = new ACL()
    acls.push(acl)

    for (i : {0...max_number_ace_of_acl - 1})
        ace = new ACE()
        acl.add_ace(ace) if fail, goto exit
        ace_count++

exit :
check the ace_count == max_number_ace_of_system


// 5. RULE_OM_TEST_Max_ACL_Of_System()
acl_count = 0
while true
    acl = new ACL() if fail, break
    acl_count++;

check the acl_count == max_number_acl
*/
