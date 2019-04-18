#include "gtest/gtest.h"

#include "converter.h"
#include "hiredis.h"
#include "orchdaemon.h"
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

uint32_t set_attr_count;
sai_attribute_t set_attr_list[20];
vector<int32_t> bpoint_list;
vector<int32_t> range_types_list;

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

int fake_create_acl_table(sai_object_id_t* acl_table_id,
    sai_object_id_t switch_id, uint32_t attr_count,
    const sai_attribute_t* attr_list);

TEST(foo, foo)
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

struct CreateRuleResult {
    bool ret_val;

    std::vector<sai_attribute_t> counter_attr_list;
    std::vector<sai_attribute_t> rule_attr_list;
};

struct TestBase : public ::testing::Test {
    static sai_status_t sai_create_acl_table_(sai_object_id_t* acl_table_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_acl_table_fn(acl_table_id, switch_id, attr_count,
            attr_list);
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

    bool createAclTable_3(AclTable* acl)
    {
        assert(sai_acl_api == nullptr);

        sai_acl_api = new sai_acl_api_t();
        auto sai_acl = std::shared_ptr<sai_acl_api_t>(sai_acl_api, [](sai_acl_api_t* p) {
            delete p;
            sai_acl_api = nullptr;
        });

        sai_acl_api->create_acl_table = sai_create_acl_table_;
        that = this;

        sai_create_acl_table_fn =
            [](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            return fake_create_acl_table(acl_table_id, switch_id, attr_count, attr_list);
        };

        return acl->create();
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

struct AclTest : public TestBase {

    std::shared_ptr<swss::DBConnector> m_app_db;
    std::shared_ptr<swss::DBConnector> m_config_db;
    std::shared_ptr<swss::DBConnector> m_state_db;
    sai_object_id_t acl_table_num;
    sai_object_id_t acl_entry_num;
    sai_object_id_t acl_counter_num;

    AclTest()
    {
        m_app_db = std::make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_config_db = std::make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_state_db = std::make_shared<swss::DBConnector>(STATE_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);

        acl_table_num = 0;
        acl_entry_num = 0;
        acl_counter_num = 0;
    }

    static void SetUpTestCase()
    {
        //system(REDIS_START_CMD);
    }

    static void TearDownTestCase()
    {
        //system(REDIS_STOP_CMD);
    }

    void allocateGlobalOrch()
    {
        assert(gAclOrch == nullptr);
        assert(gFdbOrch == nullptr);
        assert(gMirrorOrch == nullptr);
        assert(gRouteOrch == nullptr);
        assert(gNeighOrch == nullptr);
        assert(gIntfsOrch == nullptr);
        assert(gVrfOrch == nullptr);
        assert(gCrmOrch == nullptr);
        assert(gPortsOrch == nullptr);

        assert(sai_switch_api == nullptr);
        assert(sai_port_api == nullptr);
        assert(sai_vlan_api == nullptr);
        assert(sai_bridge_api == nullptr);
        assert(sai_route_api == nullptr);

        auto sai_switch = std::shared_ptr<sai_switch_api_t>(new sai_switch_api_t(), [](sai_switch_api_t* p) {
            delete p;
            sai_switch_api = nullptr;
        });
        auto sai_port = std::shared_ptr<sai_port_api_t>(new sai_port_api_t(), [](sai_port_api_t* p) {
            delete p;
            sai_port_api = nullptr;
        });
        auto sai_vlan = std::shared_ptr<sai_vlan_api_t>(new sai_vlan_api_t(), [](sai_vlan_api_t* p) {
            delete p;
            sai_vlan_api = nullptr;
        });
        auto sai_bridge = std::shared_ptr<sai_bridge_api_t>(new sai_bridge_api_t(), [](sai_bridge_api_t* p) {
            delete p;
            sai_bridge_api = nullptr;
        });
        auto sai_route = std::shared_ptr<sai_route_api_t>(new sai_route_api_t(), [](sai_route_api_t* p) {
            delete p;
            sai_route_api = nullptr;
        });

        sai_switch_api = sai_switch.get();
        sai_port_api = sai_port.get();
        sai_vlan_api = sai_vlan.get();
        sai_bridge_api = sai_bridge.get();
        sai_route_api = sai_route.get();

        sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;
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
        gPortsOrch = new PortsOrch(m_app_db.get(), ports_tables);

        gCrmOrch = new CrmOrch(m_config_db.get(), CFG_CRM_TABLE_NAME);
        gVrfOrch = new VRFOrch(m_app_db.get(), APP_VRF_TABLE_NAME);
        gIntfsOrch = new IntfsOrch(m_app_db.get(), APP_INTF_TABLE_NAME, gVrfOrch);
        gNeighOrch = new NeighOrch(m_app_db.get(), APP_NEIGH_TABLE_NAME, gIntfsOrch);
        gRouteOrch = new RouteOrch(m_app_db.get(), APP_ROUTE_TABLE_NAME, gNeighOrch);

        TableConnector applDbFdb(m_app_db.get(), APP_FDB_TABLE_NAME);
        TableConnector stateDbFdb(m_state_db.get(), STATE_FDB_TABLE_NAME);
        gFdbOrch = new FdbOrch(applDbFdb, stateDbFdb, gPortsOrch);

        TableConnector stateDbMirrorSession(m_state_db.get(), APP_MIRROR_SESSION_TABLE_NAME);
        TableConnector confDbMirrorSession(m_config_db.get(), CFG_MIRROR_SESSION_TABLE_NAME);
        gMirrorOrch = new MirrorOrch(stateDbMirrorSession, confDbMirrorSession,
            gPortsOrch, gRouteOrch, gNeighOrch, gFdbOrch);

        vector<TableConnector> acl_table_connectors = { confDbAclTable, confDbAclRuleTable };

        gAclOrch = new AclOrch(acl_table_connectors, gPortsOrch, gMirrorOrch,
            gNeighOrch, gRouteOrch);
    }

    void deleteGlobalOrch()
    {
        delete gAclOrch;
        gAclOrch = nullptr;
        delete gFdbOrch;
        gFdbOrch = nullptr;
        delete gMirrorOrch;
        gMirrorOrch = nullptr;
        delete gRouteOrch;
        gRouteOrch = nullptr;
        delete gNeighOrch;
        gNeighOrch = nullptr;
        delete gIntfsOrch;
        gIntfsOrch = nullptr;
        delete gVrfOrch;
        gVrfOrch = nullptr;
        delete gCrmOrch;
        gCrmOrch = nullptr;
        delete gPortsOrch;
        gPortsOrch = nullptr;
    }

    void SetUp() override
    {
        set_attr_count = 0;
        memset(set_attr_list, 0, sizeof(set_attr_list));

        allocateGlobalOrch();
        //assert(gCrmOrch == nullptr);
        //gCrmOrch = new CrmOrch(m_app_db.get(), CFG_CRM_TABLE_NAME);
    }

    void TearDown() override
    {
        deleteGlobalOrch();
        //delete gCrmOrch;
        //gCrmOrch = nullptr;
    }

    std::shared_ptr<CreateAclResult> createAclTable(AclTable& acl)
    {
        auto ret = std::make_shared<CreateAclResult>();

        assert(sai_acl_api == nullptr);

        auto sai_acl = std::shared_ptr<sai_acl_api_t>(new sai_acl_api_t(), [](sai_acl_api_t* p) {
            delete p;
            sai_acl_api = nullptr;
        });

        sai_acl_api = sai_acl.get();
        sai_acl_api->create_acl_table = sai_create_acl_table_;

        sai_create_acl_table_fn =
            [&](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            *acl_table_id = (++acl_table_num);
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        };

        ret->ret_val = gAclOrch->addAclTable(acl, acl.id);

        return ret;
    }

    std::shared_ptr<CreateRuleResult> createRuleToAcl(
        AclTable& acl, string& rule_id, std::vector<FieldValueTuple> filedValues)
    {
        auto ret = std::make_shared<CreateRuleResult>();

        assert(sai_acl_api == nullptr);

        auto sai_acl = std::shared_ptr<sai_acl_api_t>(new sai_acl_api_t(), [](sai_acl_api_t* p) {
            delete p;
            sai_acl_api = nullptr;
        });

        sai_acl_api = sai_acl.get();

        sai_acl_api->create_acl_counter = sai_create_acl_counter_;
        sai_acl_api->create_acl_entry = sai_create_acl_entry_;

        sai_create_acl_counter_fn =
            [&](_Out_ sai_object_id_t* acl_counter_id,
                _In_ sai_object_id_t switch_id,
                _In_ uint32_t attr_count,
                _In_ const sai_attribute_t* attr_list) -> sai_status_t {
            *acl_counter_id = (++acl_counter_num);
            for (auto i = 0; i < attr_count; ++i) {
                ret->counter_attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        };

        sai_create_acl_entry_fn =
            [&](_Out_ sai_object_id_t* acl_entry_id,
                _In_ sai_object_id_t switch_id,
                _In_ uint32_t attr_count,
                _In_ const sai_attribute_t* attr_list) -> sai_status_t {
            *acl_entry_id = (++acl_entry_num);
            for (auto i = 0; i < attr_count; ++i) {
                ret->rule_attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        };

        KeyOpFieldsValuesTuple data; // key, op, fieldValue
        string rule_key = acl.id + ":" + rule_id;
        string rule_cmd(SET_COMMAND);
        data = std::make_tuple(rule_key, rule_cmd, filedValues);

        shared_ptr<AclRule> newRule = AclRule::makeShared(acl.type, gAclOrch, gMirrorOrch,
            nullptr /*m_dTelOrch*/, rule_id, acl.id, data);

        for (const auto& itr : filedValues) {
            string attr_name = swss::to_upper(fvField(itr));
            string attr_value = fvValue(itr);

            if (newRule->validateAddPriority(attr_name, attr_value)) {
                //SWSS_LOG_INFO("Added priority attribute");
            } else if (newRule->validateAddMatch(attr_name, attr_value)) {
                //SWSS_LOG_INFO("Added match attribute '%s'", attr_name.c_str());
            } else if (newRule->validateAddAction(attr_name, attr_value)) {
                //SWSS_LOG_INFO("Added action attribute '%s'", attr_name.c_str());
            } else {
                ret->ret_val = false;
                return ret;
            }
        }

        if (!newRule->validate()) {
            ret->ret_val = false;
            return ret;
        }

        ret->ret_val = gAclOrch->addAclRule(newRule, acl.id);

        return ret;
    }
};

struct AclTestRedis : public ::testing::Test {
    AclTestRedis() {}

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

int fake_create_acl_table(sai_object_id_t* acl_table_id,
    sai_object_id_t switch_id, uint32_t attr_count,
    const sai_attribute_t* attr_list)
{
    set_attr_count = attr_count;
    memcpy(set_attr_list, attr_list, sizeof(sai_attribute_t) * attr_count);
    // return SAI_STATUS_FAILURE;
    return SAI_STATUS_SUCCESS;
}

void assign_default_acltable_attr(vector<sai_attribute_t>& table_attrs)
{
    sai_attribute_t attr;

    memset(&attr, 0, sizeof(attr));

    bpoint_list = { SAI_ACL_BIND_POINT_TYPE_PORT, SAI_ACL_BIND_POINT_TYPE_LAG };
    attr.id = SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST;
    attr.value.s32list.count = static_cast<uint32_t>(bpoint_list.size());
    attr.value.s32list.list = bpoint_list.data(); // FIXME: not good !!
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_IP;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_IP;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS;
    attr.value.booldata = true;
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    range_types_list = { SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,
        SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE };
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE;
    attr.value.s32list.count = static_cast<uint32_t>(range_types_list.size());
    attr.value.s32list.list = range_types_list.data();
    table_attrs.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    sai_acl_stage_t acl_stage;
    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    acl_stage = SAI_ACL_STAGE_INGRESS;
    attr.value.s32 = acl_stage;
    table_attrs.push_back(attr);
}

bool verify_acltable_attr(vector<sai_attribute_t>& expect_table_attrs,
    sai_attribute_t* verify_attr_p)
{
    for (auto it : expect_table_attrs) {
        if (it.id == verify_attr_p->id) {
            switch (it.id) {
            case SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE:
            case SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE:
            case SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL:
            case SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT:
            case SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT:
            case SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS:
            case SAI_ACL_TABLE_ATTR_FIELD_SRC_IP:
            case SAI_ACL_TABLE_ATTR_FIELD_DST_IP:
            case SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6:
            case SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6:
                if (it.value.booldata == verify_attr_p->value.booldata)
                    return true;
            case SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST:
                if ((it.value.s32list.count == verify_attr_p->value.s32list.count) && 0 == memcmp(it.value.s32list.list, verify_attr_p->value.s32list.list, sizeof(it.value.s32list.list[0]) * it.value.s32list.count))
                    return true;
            case SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE:
                if ((it.value.s32list.count == verify_attr_p->value.s32list.count) && 0 == memcmp(it.value.s32list.list, verify_attr_p->value.s32list.list, sizeof(it.value.s32list.list[0]) * it.value.s32list.count))
                    return true;
            case SAI_ACL_TABLE_ATTR_ACL_STAGE:
                if (it.value.s32 == verify_attr_p->value.s32)
                    return true;
            default:
                cout << it.id;
            }

            return false;
        }
    }

    return false;
}

TEST_F(AclTest, create_default_acl_table)
{
    sai_acl_api = new sai_acl_api_t();

    sai_acl_api->create_acl_table = fake_create_acl_table;

    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    acltable.create();

    // set expected data
    uint32_t expected_attr_count = 11;
    vector<sai_attribute_t> expected_attr_list;

    assign_default_acltable_attr(expected_attr_list);

    // validate ...
    EXPECT_EQ(expected_attr_count, set_attr_count);
    for (int i = 0; i < set_attr_count; ++i) {
        auto b_ret = verify_acltable_attr(expected_attr_list, &set_attr_list[i]);
        ASSERT_EQ(b_ret, true);
    }

    sai_acl_api->create_acl_table = NULL;
    delete sai_acl_api;
    sai_acl_api = nullptr;
}

TEST_F(AclTest, create_default_acl_table_2)
{
    sai_acl_api = new sai_acl_api_t();

    // sai_acl_api->create_acl_table = fake_create_acl_table;
    sai_acl_api->create_acl_table = sai_create_acl_table_;
    that = this;

    sai_create_acl_table_fn =
        [](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
            uint32_t attr_count,
            const sai_attribute_t* attr_list) -> sai_status_t {
        return fake_create_acl_table(acl_table_id, switch_id, attr_count, attr_list);
    };

    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    acltable.create();

    // set expected data
    uint32_t expected_attr_count = 11;
    vector<sai_attribute_t> expected_attr_list;

    assign_default_acltable_attr(expected_attr_list);

    // validate ...
    EXPECT_EQ(expected_attr_count, set_attr_count);
    for (int i = 0; i < set_attr_count; ++i) {
        auto b_ret = verify_acltable_attr(expected_attr_list, &set_attr_list[i]);
        ASSERT_EQ(b_ret, true);
    }

    sai_acl_api->create_acl_table = NULL;
    delete sai_acl_api;
    sai_acl_api = nullptr;
}

TEST_F(AclTest, create_default_acl_table_3)
{
    // sai_acl_api = new sai_acl_api_t();
    //
    // // sai_acl_api->create_acl_table = fake_create_acl_table;
    // sai_acl_api->create_acl_table = sai_create_acl_table_;
    // that = this;
    //
    // sai_create_acl_table_fn =
    //     [](sai_object_id_t* acl_table_id, sai_object_id_t switch_id,
    //         uint32_t attr_count,
    //         const sai_attribute_t* attr_list) -> sai_status_t {
    //     return sai_status_t(0);
    // };

    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    // acltable.create();
    createAclTable_3(&acltable);

    // set expected data
    uint32_t expected_attr_count = 11;
    vector<sai_attribute_t> expected_attr_list;

    assign_default_acltable_attr(expected_attr_list);

    // validate ...
    EXPECT_EQ(expected_attr_count, set_attr_count);
    for (int i = 0; i < set_attr_count; ++i) {
        auto b_ret = verify_acltable_attr(expected_attr_list, &set_attr_list[i]);
        ASSERT_EQ(b_ret, true);
    }

    // sai_acl_api->create_acl_table = NULL;
    // delete sai_acl_api;
}

TEST_F(AclTest, create_default_acl_table_4)
{
    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    auto res = createAclTable_4(acltable);

    ASSERT_TRUE(res->ret_val == true);

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

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(AclTest, create_l3_rule_filter_sip)
{
    //createL3AclTableAndRule
    AclTable aclTable;
    aclTable.type = ACL_TABLE_L3;

    auto talbe_ret = createAclTable(aclTable);
    ASSERT_TRUE(talbe_ret->ret_val == true);

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

    ASSERT_TRUE(AttrListEq(talbe_ret->attr_list, attr_list));

    string rule_id("acl_rule_l3");
    FieldValueTuple tmpFieldValues;
    std::vector<FieldValueTuple> filedValues;

    tmpFieldValues = std::make_pair(string(ACTION_PACKET_ACTION), string(PACKET_ACTION_FORWARD));
    filedValues.push_back(tmpFieldValues);
    tmpFieldValues = std::make_pair(string(MATCH_SRC_IP), string("10.0.0.1"));
    filedValues.push_back(tmpFieldValues);

    auto rule_ret = createRuleToAcl(aclTable, rule_id, filedValues);
    ASSERT_TRUE(rule_ret->ret_val == true);

    auto counter = std::vector<swss::FieldValueTuple>(
        { { "SAI_ACL_COUNTER_ATTR_TABLE_ID", "oid:0x1" },
            { "SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT", "true" },
            { "SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT", "true" } });
    SaiAttributeList counter_attr_list(SAI_OBJECT_TYPE_ACL_COUNTER, counter, false);
    auto rule = std::vector<swss::FieldValueTuple>(
        { { "SAI_ACL_ENTRY_ATTR_TABLE_ID", "oid:0x1" },
            { "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" },
            { "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" },
            { "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", "disabled" },
            { "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" },
            { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "10.0.0.1&mask:255.255.255.255" } });
    SaiAttributeList rule_attr_list(SAI_OBJECT_TYPE_ACL_ENTRY, rule, false);
    ASSERT_TRUE(AttrListEq(rule_ret->counter_attr_list, counter_attr_list));
    ASSERT_TRUE(AttrListEq(rule_ret->rule_attr_list, rule_attr_list));
}

TEST_F(AclTestRedis, create_default_acl_table_on_redis)
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
