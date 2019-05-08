#include "converter.h"
#include "ut_helper.h"

extern sai_object_id_t gSwitchId;

extern CrmOrch* gCrmOrch;
extern PortsOrch* gPortsOrch;
extern RouteOrch* gRouteOrch;
extern IntfsOrch* gIntfsOrch;
extern NeighOrch* gNeighOrch;

extern FdbOrch* gFdbOrch;
extern MirrorOrch* gMirrorOrch;
extern VRFOrch* gVrfOrch;

extern sai_acl_api_t* sai_acl_api;
extern sai_switch_api_t* sai_switch_api;
extern sai_port_api_t* sai_port_api;
extern sai_vlan_api_t* sai_vlan_api;
extern sai_bridge_api_t* sai_bridge_api;
extern sai_route_api_t* sai_route_api;
extern sai_hostif_api_t* sai_hostif_api;

namespace nsAclOrchTest {

using namespace std;

size_t consumerAddToSync(Consumer* consumer, const deque<KeyOpFieldsValuesTuple>& entries)
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

struct AclTestBase : public ::testing::Test {
    vector<int32_t*> m_s32list_pool;

    virtual ~AclTestBase()
    {
        for (auto p : m_s32list_pool) {
            free(p);
        }
    }
};

struct AclTest : public AclTestBase {

    struct CreateAclResult {
        bool ret_val;

        vector<sai_attribute_t> attr_list;
    };

    shared_ptr<swss::DBConnector> m_config_db;

    AclTest()
    {
        m_config_db = make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    void SetUp() override
    {
        ASSERT_EQ(gCrmOrch, nullptr);
        gCrmOrch = new CrmOrch(m_config_db.get(), CFG_CRM_TABLE_NAME);

        ASSERT_EQ(sai_acl_api, nullptr);
        sai_acl_api = new sai_acl_api_t();
    }

    void TearDown() override
    {
        delete gCrmOrch;
        gCrmOrch = nullptr;

        delete sai_acl_api;
        sai_acl_api = nullptr;
    }

    shared_ptr<CreateAclResult> createAclTable(AclTable& acl)
    {
        auto ret = make_shared<CreateAclResult>();

        auto spy = SpyOn<SAI_API_ACL, SAI_OBJECT_TYPE_ACL_TABLE>(&sai_acl_api->create_acl_table);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (uint32_t i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        ret->ret_val = acl.create();
        return ret;
    }
};

TEST_F(AclTest, Create_L3_Acl_Table)
{
    AclTable acltable;
    acltable.type = ACL_TABLE_L3;
    auto res = createAclTable(acltable);

    ASSERT_TRUE(res->ret_val);

    auto v = vector<swss::FieldValueTuple>(
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

    ASSERT_TRUE(Check::AttrListEq(SAI_OBJECT_TYPE_ACL_TABLE, res->attr_list, attr_list));
}

struct MockAclOrch {
    AclOrch* m_aclOrch;
    swss::DBConnector* config_db;

    MockAclOrch(swss::DBConnector* config_db, swss::DBConnector* state_db,
        PortsOrch* portsOrch, MirrorOrch* mirrorOrch, NeighOrch* neighOrch, RouteOrch* routeOrch)
        : config_db(config_db)
    {
        TableConnector confDbAclTable(config_db, CFG_ACL_TABLE_NAME);
        TableConnector confDbAclRuleTable(config_db, CFG_ACL_RULE_TABLE_NAME);

        vector<TableConnector> acl_table_connectors = { confDbAclTable, confDbAclRuleTable };

        TableConnector stateDbSwitchTable(state_db, "SWITCH_CAPABILITY");

        m_aclOrch = new AclOrch(acl_table_connectors, stateDbSwitchTable, portsOrch, mirrorOrch,
            neighOrch, routeOrch);
    }

    ~MockAclOrch()
    {
        delete m_aclOrch;
    }

    operator const AclOrch*() const
    {
        return m_aclOrch;
    }

    void doAclTableTask(const deque<KeyOpFieldsValuesTuple>& entries)
    {
        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(config_db, CFG_ACL_TABLE_NAME, 1, 1), m_aclOrch, CFG_ACL_TABLE_NAME));

        consumerAddToSync(consumer.get(), entries);

        static_cast<Orch*>(m_aclOrch)->doTask(*consumer);
    }

    void doAclRuleTask(const deque<KeyOpFieldsValuesTuple>& entries)
    {
        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(config_db, CFG_ACL_RULE_TABLE_NAME, 1, 1), m_aclOrch, CFG_ACL_RULE_TABLE_NAME));

        consumerAddToSync(consumer.get(), entries);

        static_cast<Orch*>(m_aclOrch)->doTask(*consumer);
    }

    sai_object_id_t getTableById(const string& table_id)
    {
        return m_aclOrch->getTableById(table_id);
    }

    const map<sai_object_id_t, AclTable>& getAclTables() const
    {
        return Portal::AclOrchInternal::getAclTables(m_aclOrch);
    }
};

struct AclOrchTest : public AclTest {

    shared_ptr<swss::DBConnector> m_app_db;
    shared_ptr<swss::DBConnector> m_config_db;
    shared_ptr<swss::DBConnector> m_state_db;

    AclOrchTest()
    {
        // FIXME: move out from constructor
        m_app_db = make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_config_db = make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_state_db = make_shared<swss::DBConnector>(STATE_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    static map<string, string> gProfileMap;
    static map<string, string>::iterator gProfileIter;

    static const char* profile_get_value(
        sai_switch_profile_id_t profile_id,
        const char* variable)
    {
        map<string, string>::const_iterator it = gProfileMap.find(variable);
        if (it == gProfileMap.end()) {
            return NULL;
        }

        return it->second.c_str();
    }

    static int profile_get_next_value(
        sai_switch_profile_id_t profile_id,
        const char** variable,
        const char** value)
    {
        if (value == NULL) {
            gProfileIter = gProfileMap.begin();
            return 0;
        }

        if (variable == NULL) {
            return -1;
        }

        if (gProfileIter == gProfileMap.end()) {
            return -1;
        }

        *variable = gProfileIter->first.c_str();
        *value = gProfileIter->second.c_str();

        gProfileIter++;

        return 0;
    }

    void SetUp() override
    {
        AclTestBase::SetUp();

        // Init switch and create dependencies

        gProfileMap.emplace("SAI_VS_SWITCH_TYPE", "SAI_VS_SWITCH_TYPE_BCM56850");
        gProfileMap.emplace("KV_DEVICE_MAC_ADDRESS", "20:03:04:05:06:00");

        sai_service_method_table_t test_services = {
            AclOrchTest::profile_get_value,
            AclOrchTest::profile_get_next_value
        };

        auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

#if WITH_SAI == LIBVS
        sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
        sai_acl_api = const_cast<sai_acl_api_t*>(&vs_acl_api);
        sai_port_api = const_cast<sai_port_api_t*>(&vs_port_api);
        sai_vlan_api = const_cast<sai_vlan_api_t*>(&vs_vlan_api);
        sai_bridge_api = const_cast<sai_bridge_api_t*>(&vs_bridge_api);
        sai_route_api = const_cast<sai_route_api_t*>(&vs_route_api);
        sai_hostif_api = const_cast<sai_hostif_api_t*>(&vs_hostif_api);
#endif

        sai_attribute_t attr;

        attr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
        attr.value.booldata = true;

        status = sai_switch_api->create_switch(&gSwitchId, 1, &attr);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        // Get switch source MAC address
        attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);

        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        gMacAddress = attr.value.mac;

        // Get the default virtual router ID
        attr.id = SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID;
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);

        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        gVirtualRouterId = attr.value.oid;

        // Create dependencies ...

        const int portsorch_base_pri = 40;

        vector<table_name_with_pri_t> ports_tables = {
            { APP_PORT_TABLE_NAME, portsorch_base_pri + 5 },
            { APP_VLAN_TABLE_NAME, portsorch_base_pri + 2 },
            { APP_VLAN_MEMBER_TABLE_NAME, portsorch_base_pri },
            { APP_LAG_TABLE_NAME, portsorch_base_pri + 4 },
            { APP_LAG_MEMBER_TABLE_NAME, portsorch_base_pri }
        };

        ASSERT_EQ(gPortsOrch, nullptr);
        gPortsOrch = new PortsOrch(m_app_db.get(), ports_tables);

        ASSERT_EQ(gCrmOrch, nullptr);
        gCrmOrch = new CrmOrch(m_config_db.get(), CFG_CRM_TABLE_NAME);

        ASSERT_EQ(gVrfOrch, nullptr);
        gVrfOrch = new VRFOrch(m_app_db.get(), APP_VRF_TABLE_NAME);

        ASSERT_EQ(gIntfsOrch, nullptr);
        gIntfsOrch = new IntfsOrch(m_app_db.get(), APP_INTF_TABLE_NAME, gVrfOrch);

        ASSERT_EQ(gNeighOrch, nullptr);
        gNeighOrch = new NeighOrch(m_app_db.get(), APP_NEIGH_TABLE_NAME, gIntfsOrch);

        ASSERT_EQ(gRouteOrch, nullptr);
        gRouteOrch = new RouteOrch(m_app_db.get(), APP_ROUTE_TABLE_NAME, gNeighOrch);

        TableConnector applDbFdb(m_app_db.get(), APP_FDB_TABLE_NAME);
        TableConnector stateDbFdb(m_state_db.get(), STATE_FDB_TABLE_NAME);

        ASSERT_EQ(gFdbOrch, nullptr);
        gFdbOrch = new FdbOrch(applDbFdb, stateDbFdb, gPortsOrch);

        TableConnector stateDbMirrorSession(m_state_db.get(), APP_MIRROR_SESSION_TABLE_NAME);
        TableConnector confDbMirrorSession(m_config_db.get(), CFG_MIRROR_SESSION_TABLE_NAME);

        ASSERT_EQ(gMirrorOrch, nullptr);
        gMirrorOrch = new MirrorOrch(stateDbMirrorSession, confDbMirrorSession,
            gPortsOrch, gRouteOrch, gNeighOrch, gFdbOrch);

        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(m_app_db.get(), APP_PORT_TABLE_NAME, 1, 1), gPortsOrch, APP_PORT_TABLE_NAME));

        /* Get port number */
        auto port_count = attr.value.u32;
        attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        port_count = attr.value.u32;

        /* Get port list */
        vector<sai_object_id_t> port_list;
        port_list.resize(port_count);
        attr.id = SAI_SWITCH_ATTR_PORT_LIST;
        attr.value.objlist.count = (uint32_t)port_list.size();
        attr.value.objlist.list = port_list.data();
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &attr);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        deque<KeyOpFieldsValuesTuple> port_init_tuple;
        for (auto i = 0; i < port_count; i++) {
            string lan_map_str = "";
            sai_uint32_t lanes[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            attr.id = SAI_PORT_ATTR_HW_LANE_LIST;
            attr.value.u32list.count = 8;
            attr.value.u32list.list = lanes;

            status = sai_port_api->get_port_attribute(port_list[i], 1, &attr);
            ASSERT_EQ(status, SAI_STATUS_SUCCESS);

            for (auto j = 0; j < attr.value.u32list.count; j++) {
                if (j != 0) {
                    lan_map_str += ",";
                }
                lan_map_str += to_string(attr.value.u32list.list[j]);
            }

            port_init_tuple.push_back(
                { to_string(i), SET_COMMAND, { { "lanes", lan_map_str }, { "admin_status", "up" }, { "speed", "1000" } } });
        }
        port_init_tuple.push_back({ "PortConfigDone", SET_COMMAND, { { "count", to_string(port_count) } } });
        port_init_tuple.push_back({ "PortInitDone", EMPTY_PREFIX, { { "", "" } } });
        consumerAddToSync(consumer.get(), port_init_tuple);

        static_cast<Orch*>(gPortsOrch)->doTask(*consumer.get());
    }

    void TearDown() override
    {
        AclTestBase::TearDown();

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

        auto status = sai_switch_api->remove_switch(gSwitchId);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);
        gSwitchId = 0;

        sai_api_uninitialize();

        sai_switch_api = nullptr;
        sai_acl_api = nullptr;
        sai_port_api = nullptr;
        sai_vlan_api = nullptr;
        sai_bridge_api = nullptr;
        sai_route_api = nullptr;
        sai_hostif_api = nullptr;
    }

    shared_ptr<MockAclOrch> createAclOrch()
    {
        return make_shared<MockAclOrch>(m_config_db.get(), m_state_db.get(), gPortsOrch, gMirrorOrch,
            gNeighOrch, gRouteOrch);
    }

    shared_ptr<SaiAttributeList> getAclTableAttributeList(sai_object_type_t objecttype, const AclTable& acl_table)
    {
        vector<swss::FieldValueTuple> fields;

        fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST", "2:SAI_ACL_BIND_POINT_TYPE_PORT,SAI_ACL_BIND_POINT_TYPE_LAG" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE", "true" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE", "true" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL", "true" });

        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT", "true" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT", "true" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS", "true" });
        fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE", "2:SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE,SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE" });

        switch (acl_table.type) {
        case ACL_TABLE_L3:
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_SRC_IP", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_DST_IP", "true" });
            break;

        case ACL_TABLE_L3V6:
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6", "true" });
            fields.push_back({ "SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6", "true" });
            break;

        default:
            ADD_FAILURE() << "Unknow acl_table.type: " << acl_table.type;
        }

        if (ACL_STAGE_INGRESS == acl_table.stage) {
            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_INGRESS" });
        } else if (ACL_STAGE_EGRESS == acl_table.stage) {
            fields.push_back({ "SAI_ACL_TABLE_ATTR_ACL_STAGE", "SAI_ACL_STAGE_EGRESS" });
        } else {
            ADD_FAILURE() << "Unknow acl_table.stage: " << acl_table.stage;
        }

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(objecttype, fields, false));
    }

    shared_ptr<SaiAttributeList> getAclRuleAttributeList(sai_object_type_t objecttype, const AclRule& acl_rule, sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        vector<swss::FieldValueTuple> fields;

        auto table_id = sai_serialize_object_id(acl_table_oid);
        auto counter_id = sai_serialize_object_id(const_cast<AclRule&>(acl_rule).getCounterOid()); // FIXME: getcounterOid() should be const

        fields.push_back({ "SAI_ACL_ENTRY_ATTR_TABLE_ID", table_id });
        fields.push_back({ "SAI_ACL_ENTRY_ATTR_PRIORITY", "0" });
        fields.push_back({ "SAI_ACL_ENTRY_ATTR_ADMIN_STATE", "true" });
        fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_COUNTER", counter_id });

        const auto& rule_actions = Portal::AclRuleInternal::getActions(&acl_rule);
        auto actionIt = rule_actions.find(SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION);
        assert(actionIt != rule_actions.end());
        assert(actionIt->second.aclaction.enable == true);
        fields.push_back({ "SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION",
            to_string(actionIt->second.aclaction.parameter.s32) });

        // SAI_ACL_ENTRY_ATTR_FIELD_*
        const auto& rule_matches = Portal::AclRuleInternal::getMatches(&acl_rule);
        for (auto matchIt = rule_matches.begin(); matchIt != rule_matches.end(); ++matchIt) {
            // matchIt = map<sai_acl_entry_attr_t (enum), sai_attribute_value_t (union)>
            auto meta = sai_metadata_get_attr_metadata(objecttype, matchIt->first);
            assert(meta != nullptr);

            sai_attribute_t saiAttribute = { matchIt->first, matchIt->second };
            saiAttribute.value.aclfield.enable = true; // TODO: check why m_matches didn't set this ?
            auto attrValString = sai_serialize_attr_value(*meta, saiAttribute, false);

            fields.push_back({ meta->attridname, attrValString });
        }

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(objecttype, fields, false));
    }

    bool validateAclRule(const string acl_rule_sid, const AclRule& acl_rule, sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_ENTRY;
        auto exp_attrlist_2 = getAclRuleAttributeList(objecttype, acl_rule, acl_table_oid, acl_table);

        auto acl_rule_oid = Portal::AclRuleInternal::getRuleOid(&acl_rule);

        {
            auto& exp_attrlist = *exp_attrlist_2;

            vector<sai_attribute_t> act_attr;

            for (uint32_t i = 0; i < exp_attrlist.get_attr_count(); ++i) {
                const auto attr = exp_attrlist.get_attr_list()[i];
                auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

                if (meta == nullptr) {
                    return false;
                }

                sai_attribute_t new_attr;
                memset(&new_attr, 0, sizeof(new_attr));

                new_attr.id = attr.id;

                switch (meta->attrvaluetype) {
                case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                    new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                    new_attr.value.s32list.count = attr.value.s32list.count;
                    m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                    break;

                default:
                    cout << "";
                    ;
                }

                act_attr.emplace_back(new_attr);
            }

            auto status = sai_acl_api->get_acl_entry_attribute(acl_rule_oid, (uint32_t)act_attr.size(), act_attr.data());
            if (status != SAI_STATUS_SUCCESS) {
                return false;
            }

            auto b_attr_eq = Check::AttrListEq(objecttype, act_attr, exp_attrlist);
            if (!b_attr_eq) {
                return false;
            }
        }

        return true;
    }

    bool validateAclTable(sai_object_id_t acl_table_oid, const AclTable& acl_table)
    {
        const sai_object_type_t objecttype = SAI_OBJECT_TYPE_ACL_TABLE;
        auto exp_attrlist_2 = getAclTableAttributeList(objecttype, acl_table);

        {
            auto& exp_attrlist = *exp_attrlist_2;

            vector<sai_attribute_t> act_attr;

            for (uint32_t i = 0; i < exp_attrlist.get_attr_count(); ++i) {
                const auto attr = exp_attrlist.get_attr_list()[i];
                auto meta = sai_metadata_get_attr_metadata(objecttype, attr.id);

                if (meta == nullptr) {
                    return false;
                }

                sai_attribute_t new_attr;
                memset(&new_attr, 0, sizeof(new_attr));

                new_attr.id = attr.id;

                switch (meta->attrvaluetype) {
                case SAI_ATTR_VALUE_TYPE_INT32_LIST:
                    new_attr.value.s32list.list = (int32_t*)malloc(sizeof(int32_t) * attr.value.s32list.count);
                    new_attr.value.s32list.count = attr.value.s32list.count;
                    m_s32list_pool.emplace_back(new_attr.value.s32list.list);
                    break;

                default:
                    // do nothing
                    ;
                }

                act_attr.emplace_back(new_attr);
            }

            auto status = sai_acl_api->get_acl_table_attribute(acl_table_oid, (uint32_t)act_attr.size(), act_attr.data());
            if (status != SAI_STATUS_SUCCESS) {
                return false;
            }

            auto b_attr_eq = Check::AttrListEq(objecttype, act_attr, exp_attrlist);
            if (!b_attr_eq) {
                return false;
            }
        }

        for (const auto& sid_acl_rule : acl_table.rules) {
            auto b_valid = validateAclRule(sid_acl_rule.first, *sid_acl_rule.second, acl_table_oid, acl_table);
            if (!b_valid) {
                return false;
            }
        }

        return true;
    }

    // consistency validation with CRM
    bool validateResourceCountWithCrm(const AclOrch* aclOrch, CrmOrch* crmOrch)
    {
        auto resourceMap = Portal::CrmOrchInternal::getResourceMap(crmOrch);
        auto ifpPortKey = Portal::CrmOrchInternal::getCrmAclKey(crmOrch, SAI_ACL_STAGE_INGRESS, SAI_ACL_BIND_POINT_TYPE_PORT);
        auto efpPortKey = Portal::CrmOrchInternal::getCrmAclKey(crmOrch, SAI_ACL_STAGE_EGRESS, SAI_ACL_BIND_POINT_TYPE_PORT);

        auto ifpPortAclTableCount = resourceMap.at(CrmResourceType::CRM_ACL_TABLE).countersMap[ifpPortKey].usedCounter;
        auto efpPortAclTableCount = resourceMap.at(CrmResourceType::CRM_ACL_TABLE).countersMap[efpPortKey].usedCounter;

        return ifpPortAclTableCount + efpPortAclTableCount == Portal::AclOrchInternal::getAclTables(aclOrch).size();

        // TODO: add rule check
    }

    // leakage check
    bool validateResourceCountWithLowerLayerDb(const AclOrch* aclOrch)
    {
        // TODO: Using the need to include "sai_vs_state.h". That will need the include path from `configure`
        //       Do this later ...
#if WITH_SAI == LIBVS
        // {
        //     auto& aclTableHash = g_switch_state_map.at(gSwitchId)->objectHash.at(SAI_OBJECT_TYPE_ACL_TABLE);
        //
        //     return aclTableHash.size() == Portal::AclOrchInternal::getAclTables(aclOrch).size();
        // }
        //
        // TODO: add rule check
#endif

        return true;
    }

    // validate consistency between aclOrch and mock data (via SAI)
    bool validateLowerLayerDb(const MockAclOrch* orch)
    {
        assert(orch != nullptr);

        if (!validateResourceCountWithCrm(orch->m_aclOrch, gCrmOrch)) {
            return false;
        }

        if (!validateResourceCountWithLowerLayerDb(orch->m_aclOrch)) {
            return false;
        }

        const auto& acl_tables = orch->getAclTables();

        for (const auto& id_acl_table : acl_tables) {
            if (!validateAclTable(id_acl_table.first, id_acl_table.second)) {
                return false;
            }
        }

        return true;
    }

    bool validateAclTableByConfOp(const AclTable& acl_table, const vector<swss::FieldValueTuple>& values)
    {
        for (const auto& fv : values) {
            if (fv.first == TABLE_DESCRIPTION) {

            } else if (fv.first == TABLE_TYPE) {
                if (fv.second == TABLE_TYPE_L3) {
                    if (acl_table.type != ACL_TABLE_L3) {
                        return false;
                    }
                } else if (fv.second == TABLE_TYPE_L3V6) {
                    if (acl_table.type != ACL_TABLE_L3V6) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else if (fv.first == TABLE_STAGE) {
                if (fv.second == TABLE_INGRESS) {
                    if (acl_table.stage != ACL_STAGE_INGRESS) {
                        return false;
                    }
                } else if (fv.second == TABLE_EGRESS) {
                    if (acl_table.stage != ACL_STAGE_EGRESS) {
                        return false;
                    }
                } else {
                    return false;
                }
            } else if (fv.first == TABLE_PORTS) {
            }
        }

        return true;
    }

    bool validateAclRuleAction(const AclRule& acl_rule, const string& attr_name, const string& attr_value)
    {
        const auto& rule_actions = Portal::AclRuleInternal::getActions(&acl_rule);

        if (attr_name == ACTION_PACKET_ACTION) {
            auto it = rule_actions.find(SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION);
            if (it == rule_actions.end()) {
                return false;
            }

            if (it->second.aclaction.enable != true) {
                return false;
            }

            if (attr_value == PACKET_ACTION_FORWARD) {
                if (it->second.aclaction.parameter.s32 != SAI_PACKET_ACTION_FORWARD) {
                    return false;
                }
            } else if (attr_value == PACKET_ACTION_DROP) {
                if (it->second.aclaction.parameter.s32 != SAI_PACKET_ACTION_DROP) {
                    return false;
                }
            } else {
                // unkonw attr_value
                return false;
            }
        } else {
            // unknow attr_name
            return false;
        }

        return true;
    }

    bool validateAclRuleMatch(const AclRule& acl_rule, const string& attr_name, const string& attr_value)
    {
        static const acl_rule_attr_lookup_t aclMatchLookup = {
            { MATCH_IN_PORTS, SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS },
            { MATCH_OUT_PORTS, SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS },
            { MATCH_SRC_IP, SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP },
            { MATCH_DST_IP, SAI_ACL_ENTRY_ATTR_FIELD_DST_IP },
            { MATCH_SRC_IPV6, SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6 },
            { MATCH_DST_IPV6, SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6 },
            { MATCH_L4_SRC_PORT, SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT },
            { MATCH_L4_DST_PORT, SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT },
            { MATCH_ETHER_TYPE, SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE },
            { MATCH_IP_PROTOCOL, SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL },
            { MATCH_TCP_FLAGS, SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS },
            { MATCH_IP_TYPE, SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE },
            { MATCH_DSCP, SAI_ACL_ENTRY_ATTR_FIELD_DSCP },
            { MATCH_TC, SAI_ACL_ENTRY_ATTR_FIELD_TC },
            { MATCH_L4_SRC_PORT_RANGE, (sai_acl_entry_attr_t)SAI_ACL_RANGE_TYPE_L4_SRC_PORT_RANGE },
            { MATCH_L4_DST_PORT_RANGE, (sai_acl_entry_attr_t)SAI_ACL_RANGE_TYPE_L4_DST_PORT_RANGE },
            { MATCH_TUNNEL_VNI, SAI_ACL_ENTRY_ATTR_FIELD_TUNNEL_VNI },
            { MATCH_INNER_ETHER_TYPE, SAI_ACL_ENTRY_ATTR_FIELD_INNER_ETHER_TYPE },
            { MATCH_INNER_IP_PROTOCOL, SAI_ACL_ENTRY_ATTR_FIELD_INNER_IP_PROTOCOL },
            { MATCH_INNER_L4_SRC_PORT, SAI_ACL_ENTRY_ATTR_FIELD_INNER_L4_SRC_PORT },
            { MATCH_INNER_L4_DST_PORT, SAI_ACL_ENTRY_ATTR_FIELD_INNER_L4_DST_PORT }
        };
        auto itAclMatchLookup = aclMatchLookup.find(attr_name);
        if (itAclMatchLookup == aclMatchLookup.end()) {
            return false;
        }

        const auto& rule_matches = Portal::AclRuleInternal::getMatches(&acl_rule);
        auto it_field = rule_matches.find(itAclMatchLookup->second);
        if (it_field == rule_matches.end()) {
            return false;
        }

        if (attr_name == MATCH_SRC_IP || attr_name == MATCH_DST_IP) {
            IpPrefix ip(attr_value);
            if (!ip.isV4()) {
                return false;
            }

            if (it_field->second.aclfield.data.ip4 != ip.getIp().getV4Addr()
                || it_field->second.aclfield.mask.ip4 != ip.getMask().getV4Addr()) {
                return false;
            }

        } else if (attr_name == MATCH_SRC_IPV6 || attr_name == MATCH_DST_IPV6) {
            IpPrefix ip(attr_value);
            if (ip.isV4()) {
                return false;
            }

            if (0 != memcmp(it_field->second.aclfield.data.ip6, ip.getIp().getV6Addr(), 16)
                || 0 != memcmp(it_field->second.aclfield.mask.ip6, ip.getMask().getV6Addr(), 16)) {
                return false;
            }
        } else if (attr_name == MATCH_ETHER_TYPE || attr_name == MATCH_L4_SRC_PORT || attr_name == MATCH_L4_DST_PORT) {
            if (swss::to_uint<uint16_t>(attr_value) != it_field->second.aclfield.data.u16
                || 0xffff != it_field->second.aclfield.mask.u16) {
                return false;
            }
        } else if (attr_name == MATCH_IP_PROTOCOL) {
            if (swss::to_uint<uint8_t>(attr_value) != it_field->second.aclfield.data.u8
                || 0xff != it_field->second.aclfield.mask.u8) {
                return false;
            }
        } else if (attr_name == MATCH_IP_TYPE) {
            static const acl_ip_type_lookup_t aclIpTypeLookup = {
                { IP_TYPE_ANY, SAI_ACL_IP_TYPE_ANY },
                { IP_TYPE_IP, SAI_ACL_IP_TYPE_IP },
                { IP_TYPE_NON_IP, SAI_ACL_IP_TYPE_NON_IP },
                { IP_TYPE_IPv4ANY, SAI_ACL_IP_TYPE_IPV4ANY },
                { IP_TYPE_NON_IPv4, SAI_ACL_IP_TYPE_NON_IPV4 },
                { IP_TYPE_IPv6ANY, SAI_ACL_IP_TYPE_IPV6ANY },
                { IP_TYPE_NON_IPv6, SAI_ACL_IP_TYPE_NON_IPV6 },
                { IP_TYPE_ARP, SAI_ACL_IP_TYPE_ARP },
                { IP_TYPE_ARP_REQUEST, SAI_ACL_IP_TYPE_ARP_REQUEST },
                { IP_TYPE_ARP_REPLY, SAI_ACL_IP_TYPE_ARP_REPLY }
            };
            auto itAclIpTypeLookup = aclIpTypeLookup.find(swss::to_upper(attr_value));
            if (itAclIpTypeLookup == aclIpTypeLookup.end()) {
                return false;
            }

            if (it_field->second.aclfield.data.u32 != itAclIpTypeLookup->second
                || 0xFFFFFFFF != it_field->second.aclfield.mask.u32) {
                return false;
            }
        } else if (attr_name == MATCH_TCP_FLAGS) {
            // tcp_flags = h8/h8
            size_t delimiter_pos = attr_value.find("/");
            if (delimiter_pos == string::npos) {
                return false;
            }

            if (swss::to_uint<uint8_t>(attr_value.substr(0, delimiter_pos)) != it_field->second.aclfield.data.u8
                || swss::to_uint<uint8_t>(attr_value.substr(delimiter_pos + 1)) != it_field->second.aclfield.mask.u8) {
                return false;
            }
        } else {
            // unknow attr_name
            return false;
        }

        return true;
    }

    bool validateAclRuleByConfOp(const AclRule& acl_rule, const vector<swss::FieldValueTuple>& values)
    {
        for (const auto& fv : values) {
            auto attr_name = fv.first;
            auto attr_value = fv.second;

            static vector<string> match_attr_field = {
                MATCH_SRC_IP,
                MATCH_DST_IP,
                MATCH_SRC_IPV6,
                MATCH_DST_IPV6,
                MATCH_ETHER_TYPE,
                MATCH_IP_PROTOCOL,
                MATCH_IP_TYPE,
                MATCH_L4_SRC_PORT,
                MATCH_L4_DST_PORT,
                MATCH_TCP_FLAGS
            };

            if (attr_name == ACTION_PACKET_ACTION) {
                if (!validateAclRuleAction(acl_rule, attr_name, attr_value)) {
                    return false;
                }
            } else if (match_attr_field.end() != find(match_attr_field.begin(), match_attr_field.end(), attr_name)) {
                if (!validateAclRuleMatch(acl_rule, attr_name, attr_value)) {
                    return false;
                }
            } else {
                ADD_FAILURE() << "Unknow attr_name: " << attr_name;
                return false;
            }
        }
        return true;
    }
};

map<string, string> AclOrchTest::gProfileMap;
map<string, string>::iterator AclOrchTest::gProfileIter = AclOrchTest::gProfileMap.begin();

// When received ACL table SET_COMMAND, orchagent can create corresponding ACL.
// When received ACL table DEL_COMMAND, orchagent can delete corresponding ACL.
//
// Input by type = {L3, L3V6, PFCCMD ...}, stage = {INGRESS, EGRESS}.
//
// Using fixed ports = {"1,2"} for now.
// The bind operations will be another separately test cases.
TEST_F(AclOrchTest, ACL_Creation_and_Destorying)
{
    auto orch = createAclOrch();

    for (const auto& acl_table_type : { TABLE_TYPE_L3, TABLE_TYPE_L3V6 }) {
        for (const auto& acl_table_stage : { TABLE_INGRESS, TABLE_EGRESS }) {
            string acl_table_id = "acl_table_1";

            auto kvfAclTable = deque<KeyOpFieldsValuesTuple>(
                { { acl_table_id,
                    SET_COMMAND,
                    { { TABLE_DESCRIPTION, "filter source IP" },
                        { TABLE_TYPE, acl_table_type },
                        { TABLE_STAGE, acl_table_stage },
                        { TABLE_PORTS, "1,2" } } } });
            // FIXME:                  ^^^^^^^^^^^^^ fixed port

            orch->doAclTableTask(kvfAclTable);

            auto oid = orch->getTableById(acl_table_id);
            ASSERT_NE(oid, SAI_NULL_OBJECT_ID);

            const auto& acl_tables = orch->getAclTables();

            auto it = acl_tables.find(oid);
            ASSERT_NE(it, acl_tables.end());

            const auto& acl_table = it->second;

            ASSERT_TRUE(validateAclTableByConfOp(acl_table, kfvFieldsValues(kvfAclTable.front())));
            ASSERT_TRUE(validateLowerLayerDb(orch.get()));

            // delete acl table ...

            kvfAclTable = deque<KeyOpFieldsValuesTuple>(
                { { acl_table_id,
                    DEL_COMMAND,
                    {} } });

            orch->doAclTableTask(kvfAclTable);

            oid = orch->getTableById(acl_table_id);
            ASSERT_EQ(oid, SAI_NULL_OBJECT_ID);

            ASSERT_TRUE(validateLowerLayerDb(orch.get()));
        }
    }
}

// When received ACL rule SET_COMMAND, orchagent can create corresponding ACL rule.
// When received ACL rule DEL_COMMAND, orchagent can delete corresponding ACL rule.
//
// Verify ACL table type = { L3 }, stage = { INGRESS, ENGRESS }
// Input by matchs = { SIP, DIP ...}, pkg:actions = { FORWARD, DROP ... }
//
TEST_F(AclOrchTest, L3Acl_Matches_Actions)
{
    string acl_table_id = "acl_table_1";
    string acl_rule_id = "acl_rule_1";

    auto orch = createAclOrch();

    auto kvfAclTable = deque<KeyOpFieldsValuesTuple>(
        { { acl_table_id,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3 },
                //            ^^^^^^^^^^^^^ L3 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    orch->doAclTableTask(kvfAclTable);

    // validate acl table ...

    auto acl_table_oid = orch->getTableById(acl_table_id);
    ASSERT_NE(acl_table_oid, SAI_NULL_OBJECT_ID);

    const auto& acl_tables = orch->getAclTables();
    auto it_table = acl_tables.find(acl_table_oid);
    ASSERT_NE(it_table, acl_tables.end());

    const auto& acl_table = it_table->second;

    ASSERT_TRUE(validateAclTableByConfOp(acl_table, kfvFieldsValues(kvfAclTable.front())));
    ASSERT_TRUE(validateLowerLayerDb(orch.get()));

    // add rule ...
    for (const auto& acl_rule_pkg_action : { PACKET_ACTION_FORWARD, PACKET_ACTION_DROP }) {

        auto kvfAclRule = deque<KeyOpFieldsValuesTuple>({ { acl_table_id + "|" + acl_rule_id,
            SET_COMMAND,
            { { ACTION_PACKET_ACTION, acl_rule_pkg_action },

                // if (attr_name == ACTION_PACKET_ACTION || attr_name == ACTION_MIRROR_ACTION ||
                // attr_name == ACTION_DTEL_FLOW_OP || attr_name == ACTION_DTEL_INT_SESSION ||
                // attr_name == ACTION_DTEL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_TAIL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_FLOW_SAMPLE_PERCENT ||
                // attr_name == ACTION_DTEL_REPORT_ALL_PACKETS)
                //
                // TODO: required field (add new test cases for that ....)
                //

                { MATCH_ETHER_TYPE, "0x0800" },
                { MATCH_SRC_IP, "1.2.3.4/32" },
                { MATCH_DST_IP, "4.3.2.1/32" },
                { MATCH_IP_TYPE, "ipv4any" },
                { MATCH_IP_PROTOCOL, "0x11" },
                { MATCH_L4_SRC_PORT, "0" },
                { MATCH_L4_DST_PORT, "65535" },
                { MATCH_TCP_FLAGS, "0x7/0x3f" } } } });

        // TODO: RULE_PRIORITY (important field)
        // TODO: MATCH_DSCP / MATCH_SRC_IPV6 || attr_name == MATCH_DST_IPV6

        orch->doAclRuleTask(kvfAclRule);

        // validate acl rule ...

        auto it_rule = acl_table.rules.find(acl_rule_id);
        ASSERT_NE(it_rule, acl_table.rules.end());

        ASSERT_TRUE(validateAclRuleByConfOp(*it_rule->second, kfvFieldsValues(kvfAclRule.front())));
        ASSERT_TRUE(validateLowerLayerDb(orch.get()));

        // delete acl rule ...

        kvfAclRule = deque<KeyOpFieldsValuesTuple>({ { acl_table_id + "|" + acl_rule_id,
            DEL_COMMAND,
            {} } });

        orch->doAclRuleTask(kvfAclRule);

        // validate acl rule ...

        it_rule = acl_table.rules.find(acl_rule_id);
        ASSERT_EQ(it_rule, acl_table.rules.end());
        ASSERT_TRUE(validateLowerLayerDb(orch.get()));
    }
}

// When received ACL rule SET_COMMAND, orchagent can create corresponding ACL rule.
// When received ACL rule DEL_COMMAND, orchagent can delete corresponding ACL rule.
//
// Verify ACL table type = { L3V6 }, stage = { INGRESS, ENGRESS }
// Input by matchs = { SIP, DIP ...}, pkg:actions = { FORWARD, DROP ... }
//
TEST_F(AclOrchTest, L3V6Acl_Matches_Actions)
{
    string acl_table_id = "acl_table_1";
    string acl_rule_id = "acl_rule_1";

    auto orch = createAclOrch();

    auto kvfAclTable = deque<KeyOpFieldsValuesTuple>(
        { { acl_table_id,
            SET_COMMAND,
            { { TABLE_DESCRIPTION, "filter source IP" },
                { TABLE_TYPE, TABLE_TYPE_L3V6 },
                //            ^^^^^^^^^^^^^ L3V6 ACL
                { TABLE_STAGE, TABLE_INGRESS },
                // FIXME:      ^^^^^^^^^^^^^ only support / test for ingress ?
                { TABLE_PORTS, "1,2" } } } });
    // FIXME:                  ^^^^^^^^^^^^^ fixed port

    orch->doAclTableTask(kvfAclTable);

    // validate acl table ...

    auto acl_table_oid = orch->getTableById(acl_table_id);
    ASSERT_NE(acl_table_oid, SAI_NULL_OBJECT_ID);

    const auto& acl_tables = orch->getAclTables();
    auto it_table = acl_tables.find(acl_table_oid);
    ASSERT_NE(it_table, acl_tables.end());

    const auto& acl_table = it_table->second;

    ASSERT_TRUE(validateAclTableByConfOp(acl_table, kfvFieldsValues(kvfAclTable.front())));
    ASSERT_TRUE(validateLowerLayerDb(orch.get()));

    // add rule ...
    for (const auto& acl_rule_pkg_action : { PACKET_ACTION_FORWARD, PACKET_ACTION_DROP }) {

        auto kvfAclRule = deque<KeyOpFieldsValuesTuple>({ { acl_table_id + "|" + acl_rule_id,
            SET_COMMAND,
            { { ACTION_PACKET_ACTION, acl_rule_pkg_action },

                // if (attr_name == ACTION_PACKET_ACTION || attr_name == ACTION_MIRROR_ACTION ||
                // attr_name == ACTION_DTEL_FLOW_OP || attr_name == ACTION_DTEL_INT_SESSION ||
                // attr_name == ACTION_DTEL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_TAIL_DROP_REPORT_ENABLE ||
                // attr_name == ACTION_DTEL_FLOW_SAMPLE_PERCENT ||
                // attr_name == ACTION_DTEL_REPORT_ALL_PACKETS)
                //
                // TODO: required field (add new test cases for that ....)
                //

                { MATCH_SRC_IPV6, "::1.2.3.4" },
                { MATCH_DST_IPV6, "::4.3.2.1" } } } });

        // TODO: RULE_PRIORITY (important field)
        // TODO: MATCH_DSCP / MATCH_SRC_IPV6 || attr_name == MATCH_DST_IPV6

        orch->doAclRuleTask(kvfAclRule);

        // validate acl rule ...

        auto it_rule = acl_table.rules.find(acl_rule_id);
        ASSERT_NE(it_rule, acl_table.rules.end());

        ASSERT_TRUE(validateAclRuleByConfOp(*it_rule->second, kfvFieldsValues(kvfAclRule.front())));
        ASSERT_TRUE(validateLowerLayerDb(orch.get()));

        // delete acl rule ...

        kvfAclRule = deque<KeyOpFieldsValuesTuple>({ { acl_table_id + "|" + acl_rule_id,
            DEL_COMMAND,
            {} } });

        orch->doAclRuleTask(kvfAclRule);

        // validate acl rule ...

        it_rule = acl_table.rules.find(acl_rule_id);
        ASSERT_EQ(it_rule, acl_table.rules.end());
        ASSERT_TRUE(validateLowerLayerDb(orch.get()));
    }
}

} // namespace nsAclOrchTest
