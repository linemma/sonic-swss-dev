#include "ut_helper.h"

#include "orchdaemon.h"

extern sai_object_id_t gSwitchId;

extern PortsOrch* gPortsOrch;

extern sai_hostif_api_t* sai_hostif_api;
extern sai_policer_api_t* sai_policer_api;

// portOrch dependency start
extern sai_port_api_t* sai_port_api;
extern sai_vlan_api_t* sai_vlan_api;
extern sai_bridge_api_t* sai_bridge_api;
// portOrch dependency end

// system dependency start
extern sai_switch_api_t* sai_switch_api;
// system dependency end

namespace nsCoppOrchTest {

using namespace std;

map<string, string> m_policer_meter_map = {
    { "packets", "SAI_METER_TYPE_PACKETS" },
    { "bytes", "SAI_METER_TYPE_BYTES" }
};

map<string, string> m_policer_mode_map = {
    { "sr_tcm", "SAI_POLICER_MODE_SR_TCM" },
    { "tr_tcm", "SAI_POLICER_MODE_TR_TCM" },
    { "storm", "SAI_POLICER_MODE_STORM_CONTROL" }
};

map<string, string> m_policer_color_aware_map = {
    { "aware", "SAI_POLICER_COLOR_SOURCE_AWARE" },
    { "blind", "SAI_POLICER_COLOR_SOURCE_BLIND" }
};

map<string, string> m_trap_type_map = {
    { "stp", "SAI_HOSTIF_TRAP_TYPE_STP" },
    { "lacp", "SAI_HOSTIF_TRAP_TYPE_LACP" },
    { "eapol", "SAI_HOSTIF_TRAP_TYPE_EAPOL" },
    { "lldp", "SAI_HOSTIF_TRAP_TYPE_LLDP" },
    { "pvrst", "SAI_HOSTIF_TRAP_TYPE_PVRST" },
    { "igmp_query", "SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_QUERY" },
    { "igmp_leave", "SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_LEAVE" },
    { "igmp_v1_report", "SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V1_REPORT" },
    { "igmp_v2_report", "SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V2_REPORT" },
    { "igmp_v3_report", "SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V3_REPORT" },
    { "sample_packet", "SAI_HOSTIF_TRAP_TYPE_SAMPLEPACKET" },
    { "switch_cust_range", "SAI_HOSTIF_TRAP_TYPE_SWITCH_CUSTOM_RANGE_BASE" },
    { "arp_req", "SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST" },
    { "arp_resp", "SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE" },
    { "dhcp", "SAI_HOSTIF_TRAP_TYPE_DHCP" },
    { "ospf", "SAI_HOSTIF_TRAP_TYPE_OSPF" },
    { "pim", "SAI_HOSTIF_TRAP_TYPE_PIM" },
    { "vrrp", "SAI_HOSTIF_TRAP_TYPE_VRRP" },
    { "bgp", "SAI_HOSTIF_TRAP_TYPE_BGP" },
    { "dhcpv6", "SAI_HOSTIF_TRAP_TYPE_DHCPV6" },
    { "ospfv6", "SAI_HOSTIF_TRAP_TYPE_OSPFV6" },
    { "vrrpv6", "SAI_HOSTIF_TRAP_TYPE_VRRPV6" },
    { "bgpv6", "SAI_HOSTIF_TRAP_TYPE_BGPV6" },
    { "neigh_discovery", "SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY" },
    { "mld_v1_v2", "SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_V2" },
    { "mld_v1_report", "SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_REPORT" },
    { "mld_v2_done", "SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_DONE" },
    { "mld_v2_report", "SAI_HOSTIF_TRAP_TYPE_MLD_V2_REPORT" },
    { "ip2me", "SAI_HOSTIF_TRAP_TYPE_IP2ME" },
    { "ssh", "SAI_HOSTIF_TRAP_TYPE_SSH" },
    { "snmp", "SAI_HOSTIF_TRAP_TYPE_SNMP" },
    { "router_custom_range", "SAI_HOSTIF_TRAP_TYPE_ROUTER_CUSTOM_RANGE_BASE" },
    { "l3_mtu_error", "SAI_HOSTIF_TRAP_TYPE_L3_MTU_ERROR" },
    { "ttl_error", "SAI_HOSTIF_TRAP_TYPE_TTL_ERROR" },
    { "udld", "SAI_HOSTIF_TRAP_TYPE_UDLD" }
};

map<string, sai_hostif_trap_type_t> m_trap_id_map = {
    { "stp", SAI_HOSTIF_TRAP_TYPE_STP },
    { "lacp", SAI_HOSTIF_TRAP_TYPE_LACP },
    { "eapol", SAI_HOSTIF_TRAP_TYPE_EAPOL },
    { "lldp", SAI_HOSTIF_TRAP_TYPE_LLDP },
    { "pvrst", SAI_HOSTIF_TRAP_TYPE_PVRST },
    { "igmp_query", SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_QUERY },
    { "igmp_leave", SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_LEAVE },
    { "igmp_v1_report", SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V1_REPORT },
    { "igmp_v2_report", SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V2_REPORT },
    { "igmp_v3_report", SAI_HOSTIF_TRAP_TYPE_IGMP_TYPE_V3_REPORT },
    { "sample_packet", SAI_HOSTIF_TRAP_TYPE_SAMPLEPACKET },
    { "switch_cust_range", SAI_HOSTIF_TRAP_TYPE_SWITCH_CUSTOM_RANGE_BASE },
    { "arp_req", SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST },
    { "arp_resp", SAI_HOSTIF_TRAP_TYPE_ARP_RESPONSE },
    { "dhcp", SAI_HOSTIF_TRAP_TYPE_DHCP },
    { "ospf", SAI_HOSTIF_TRAP_TYPE_OSPF },
    { "pim", SAI_HOSTIF_TRAP_TYPE_PIM },
    { "vrrp", SAI_HOSTIF_TRAP_TYPE_VRRP },
    { "bgp", SAI_HOSTIF_TRAP_TYPE_BGP },
    { "dhcpv6", SAI_HOSTIF_TRAP_TYPE_DHCPV6 },
    { "ospfv6", SAI_HOSTIF_TRAP_TYPE_OSPFV6 },
    { "vrrpv6", SAI_HOSTIF_TRAP_TYPE_VRRPV6 },
    { "bgpv6", SAI_HOSTIF_TRAP_TYPE_BGPV6 },
    { "neigh_discovery", SAI_HOSTIF_TRAP_TYPE_IPV6_NEIGHBOR_DISCOVERY },
    { "mld_v1_v2", SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_V2 },
    { "mld_v1_report", SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_REPORT },
    { "mld_v2_done", SAI_HOSTIF_TRAP_TYPE_IPV6_MLD_V1_DONE },
    { "mld_v2_report", SAI_HOSTIF_TRAP_TYPE_MLD_V2_REPORT },
    { "ip2me", SAI_HOSTIF_TRAP_TYPE_IP2ME },
    { "ssh", SAI_HOSTIF_TRAP_TYPE_SSH },
    { "snmp", SAI_HOSTIF_TRAP_TYPE_SNMP },
    { "router_custom_range", SAI_HOSTIF_TRAP_TYPE_ROUTER_CUSTOM_RANGE_BASE },
    { "l3_mtu_error", SAI_HOSTIF_TRAP_TYPE_L3_MTU_ERROR },
    { "ttl_error", SAI_HOSTIF_TRAP_TYPE_TTL_ERROR },
    { "udld", SAI_HOSTIF_TRAP_TYPE_UDLD }
};

map<string, string> m_packet_action_map = {
    { "drop", "SAI_PACKET_ACTION_DROP" },
    { "forward", "SAI_PACKET_ACTION_FORWARD" },
    { "copy", "SAI_PACKET_ACTION_COPY" },
    { "copy_cancel", "SAI_PACKET_ACTION_COPY_CANCEL" },
    { "trap", "SAI_PACKET_ACTION_TRAP" },
    { "log", "SAI_PACKET_ACTION_LOG" },
    { "deny", "SAI_PACKET_ACTION_DENY" },
    { "transit", "SAI_PACKET_ACTION_TRANSIT" }
};

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

struct CoppOrchHandler {
    CoppOrch* m_coppOrch;
    swss::DBConnector* app_db;

    CoppOrchHandler(swss::DBConnector* app_db)
        : app_db(app_db)
    {
        m_coppOrch = new CoppOrch(app_db, APP_COPP_TABLE_NAME);
    }

    operator const CoppOrch*() const
    {
        return m_coppOrch;
    }

    void doCoppTask(const deque<KeyOpFieldsValuesTuple>& entries)
    {
        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(app_db, APP_COPP_TABLE_NAME, 1, 1), m_coppOrch, CFG_ACL_TABLE_NAME));

        consumerAddToSync(consumer.get(), entries);

        static_cast<Orch*>(m_coppOrch)->doTask(*consumer);
    }

    const object_map& getTrapGroupMap()
    {
        return Portal::CoppOrchInternal::getTrapGroupMap(m_coppOrch);
    }

    const TrapIdTrapGroupTable& getTrapIdTrapGroupMap() const
    {
        return Portal::CoppOrchInternal::getTrapIdTrapGroupMap(m_coppOrch);
    }

    const TrapGroupPolicerTable& getTrapGroupPolicerMap() const
    {
        return Portal::CoppOrchInternal::getTrapGroupPolicerMap(m_coppOrch);
    }
};

struct CoppTestBase : public ::testing::Test {
};

struct CoppTest : public CoppTestBase {

    struct CoppResult {
        bool ret_val;

        vector<sai_attribute_t> group_attr_list;
        vector<sai_attribute_t> trap_attr_list;
        vector<sai_attribute_t> policer_attr_list;
    };

    shared_ptr<swss::DBConnector> m_app_db;

    void SetUp() override
    {
        m_app_db = make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    void TearDown() override
    {
    }

    shared_ptr<CoppOrchHandler> createCoppOrch()
    {
        return make_shared<CoppOrchHandler>(m_app_db.get());
    }

    vector<string> getTrapTypeList(const vector<FieldValueTuple>& rule_values)
    {
        vector<string> types;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_trap_id_list) {
                types = tokenize(fvValue(it), list_item_delimiter);
            }
        }

        return types;
    }

    shared_ptr<SaiAttributeList> getTrapGroupAttributeList(const vector<FieldValueTuple>& rule_values)
    {
        vector<swss::FieldValueTuple> fields;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_queue_field)
                fields.push_back({ "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", fvValue(it) });
        }

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, fields, false));
    }

    void replaceTrapType(vector<FieldValueTuple>& rule_values, string trap_type)
    {
        for (auto& it : rule_values) {
            if (kfvKey(it) == copp_trap_id_list) {
                it.second = trap_type;
            }
        }
    }

    shared_ptr<SaiAttributeList> getTrapAttributeList(const sai_object_id_t group_id, const vector<FieldValueTuple>& rule_values)
    {
        vector<swss::FieldValueTuple> fields;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_trap_action_field) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", m_packet_action_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_trap_priority_field) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", fvValue(it) });
            } else if (kfvKey(it) == copp_trap_id_list) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", m_trap_type_map.at(fvValue(it)) });
            }
        }
        auto table_id = sai_serialize_object_id(group_id);
        fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", table_id });

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_HOSTIF_TRAP, fields, false));
    }

    shared_ptr<SaiAttributeList> getPoliceAttributeList(const vector<FieldValueTuple>& rule_values)
    {
        vector<swss::FieldValueTuple> fields;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_policer_meter_type_field) {
                fields.push_back({ "SAI_POLICER_ATTR_METER_TYPE", m_policer_meter_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_policer_mode_field) {
                fields.push_back({ "SAI_POLICER_ATTR_MODE", m_policer_mode_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_policer_color_field) {
                fields.push_back({ "SAI_POLICER_ATTR_COLOR_SOURCE", m_policer_color_aware_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_policer_cbs_field) {
                fields.push_back({ "SAI_POLICER_ATTR_CBS", fvValue(it) });
            } else if (kfvKey(it) == copp_policer_cir_field) {
                fields.push_back({ "SAI_POLICER_ATTR_CIR", fvValue(it) });
            } else if (kfvKey(it) == copp_policer_pbs_field) {
                fields.push_back({ "SAI_POLICER_ATTR_PBS", fvValue(it) });
            } else if (kfvKey(it) == copp_policer_pir_field) {
                fields.push_back({ "SAI_POLICER_ATTR_PIR", fvValue(it) });
            } else if (kfvKey(it) == copp_policer_action_green_field) {
                fields.push_back({ "SAI_POLICER_ATTR_GREEN_PACKET_ACTION", m_packet_action_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_policer_action_red_field) {
                fields.push_back({ "SAI_POLICER_ATTR_RED_PACKET_ACTION", m_packet_action_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_policer_action_yellow_field) {
                fields.push_back({ "SAI_POLICER_ATTR_YELLOW_PACKET_ACTION", m_packet_action_map.at(fvValue(it)) });
            }
        }

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_POLICER, fields, false));
    }
};

struct CoppOrchTest : public CoppTest {

    CoppOrchTest()
    {
    }
    ~CoppOrchTest()
    {
    }

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
            printf("resetting profile map iterator");
            gProfileIter = gProfileMap.begin();
            return 0;
        }

        if (variable == NULL) {
            printf("variable is null");
            return -1;
        }

        if (gProfileIter == gProfileMap.end()) {
            printf("iterator reached end");
            return -1;
        }

        *variable = gProfileIter->first.c_str();
        *value = gProfileIter->second.c_str();

        printf("key: %s:%s", *variable, *value);

        gProfileIter++;

        return 0;
    }

    static map<string, string> gProfileMap;
    static map<string, string>::iterator gProfileIter;

    void SetUp() override
    {
        CoppTest::SetUp();

#if WITH_SAI == LIBVS
        sai_hostif_api = const_cast<sai_hostif_api_t*>(&vs_hostif_api);
        sai_policer_api = const_cast<sai_policer_api_t*>(&vs_policer_api);
        sai_port_api = const_cast<sai_port_api_t*>(&vs_port_api);
        sai_vlan_api = const_cast<sai_vlan_api_t*>(&vs_vlan_api);
        sai_bridge_api = const_cast<sai_bridge_api_t*>(&vs_bridge_api);
        sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
#endif

        gProfileMap.emplace("SAI_VS_SWITCH_TYPE", "SAI_VS_SWITCH_TYPE_BCM56850");
        gProfileMap.emplace("KV_DEVICE_MAC_ADDRESS", "20:03:04:05:06:00");

        sai_service_method_table_t test_services = {
            profile_get_value,
            profile_get_next_value
        };

        auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        sai_attribute_t swattr;

        swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
        swattr.value.booldata = true;

        status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        // Get switch source MAC address
        swattr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &swattr);

        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        gMacAddress = swattr.value.mac;

        // Get the default virtual router ID
        swattr.id = SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID;
        status = sai_switch_api->get_switch_attribute(gSwitchId, 1, &swattr);

        ASSERT_EQ(status, SAI_STATUS_SUCCESS);

        gVirtualRouterId = swattr.value.oid;

        //call copporch->doTask need to initial portsOrch
        const int portsorch_base_pri = 40;

        vector<table_name_with_pri_t> ports_tables = {
            { APP_PORT_TABLE_NAME, portsorch_base_pri + 5 },
            { APP_VLAN_TABLE_NAME, portsorch_base_pri + 2 },
            { APP_VLAN_MEMBER_TABLE_NAME, portsorch_base_pri },
            { APP_LAG_TABLE_NAME, portsorch_base_pri + 4 },
            { APP_LAG_MEMBER_TABLE_NAME, portsorch_base_pri }
        };

        // FIXME: doesn't use global variable !!
        ASSERT_EQ(gPortsOrch, nullptr);
        gPortsOrch = new PortsOrch(m_app_db.get(), ports_tables);

        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(m_app_db.get(), APP_PORT_TABLE_NAME, 1, 1), gPortsOrch, APP_PORT_TABLE_NAME));

        consumerAddToSync(consumer.get(), { { "PortInitDone", EMPTY_PREFIX, { { "", "" } } } });

        static_cast<Orch*>(gPortsOrch)->doTask(*consumer.get());
    }

    void TearDown() override
    {
        CoppTest::TearDown();

        auto status = sai_switch_api->remove_switch(gSwitchId);
        ASSERT_EQ(status, SAI_STATUS_SUCCESS);
        gSwitchId = 0;

        sai_api_uninitialize();

        delete gPortsOrch;
        gPortsOrch = nullptr;

        sai_hostif_api = nullptr;
        sai_policer_api = nullptr;
        sai_switch_api = nullptr;
        sai_port_api = nullptr;
        sai_vlan_api = nullptr;
        sai_bridge_api = nullptr;
    }

    bool ValidateTrapGroup(sai_object_id_t id, SaiAttributeList& exp_group_attr_list)
    {
        sai_object_type_t trap_group_object_type = SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP;
        vector<sai_attribute_t> trap_group_act_attr;

        for (int i = 0; i < exp_group_attr_list.get_attr_count(); ++i) {
            const auto attr = exp_group_attr_list.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(trap_group_object_type, attr.id);

            if (meta == nullptr) {
                return false;
            }

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;
            trap_group_act_attr.emplace_back(new_attr);
        }

        auto status = sai_hostif_api->get_hostif_trap_group_attribute(id, trap_group_act_attr.size(), trap_group_act_attr.data());
        if (status != SAI_STATUS_SUCCESS) {
            return false;
        }

        auto b_attr_eq = Check::AttrListEq(trap_group_object_type, trap_group_act_attr, exp_group_attr_list);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }

    bool ValidateTrap(sai_object_id_t id, SaiAttributeList& exp_trap_attr_list)
    {
        sai_object_type_t trap_object_type = SAI_OBJECT_TYPE_HOSTIF_TRAP;
        vector<sai_attribute_t> trap_act_attr;

        for (int i = 0; i < exp_trap_attr_list.get_attr_count(); ++i) {
            const auto attr = exp_trap_attr_list.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(trap_object_type, attr.id);

            if (meta == nullptr) {
                return false;
            }

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;
            trap_act_attr.emplace_back(new_attr);
        }

        auto status = sai_hostif_api->get_hostif_trap_attribute(id, trap_act_attr.size(), trap_act_attr.data());
        if (status != SAI_STATUS_SUCCESS) {
            return false;
        }

        auto b_attr_eq = Check::AttrListEq(trap_object_type, trap_act_attr, exp_trap_attr_list);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }

    bool ValidatePolicer(sai_object_id_t id, SaiAttributeList& exp_policer_attr_list)
    {
        sai_object_type_t policer_object_type = SAI_OBJECT_TYPE_POLICER;
        vector<sai_attribute_t> policer_act_attr;

        for (int i = 0; i < exp_policer_attr_list.get_attr_count(); ++i) {
            const auto attr = exp_policer_attr_list.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(policer_object_type, attr.id);

            if (meta == nullptr) {
                return false;
            }

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;
            policer_act_attr.emplace_back(new_attr);
        }

        auto status = sai_policer_api->get_policer_attribute(id, policer_act_attr.size(), policer_act_attr.data());
        if (status != SAI_STATUS_SUCCESS) {
            return false;
        }

        auto b_attr_eq = Check::AttrListEq(policer_object_type, policer_act_attr, exp_policer_attr_list);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }

    bool Validate(CoppOrchHandler* orch, const string& groupName, const vector<FieldValueTuple>& rule_values)
    {
        auto exp_group_attr_list = getTrapGroupAttributeList(rule_values);
        auto exp_police_attr_list = getPoliceAttributeList(rule_values);

        //valid trap group
        auto trap_group_map = orch->getTrapGroupMap();
        auto grp_itr = trap_group_map.find(groupName);
        if (grp_itr == trap_group_map.end()) {
            return false;
        }

        if (!ValidateTrapGroup(grp_itr->second, *exp_group_attr_list.get())) {
            return false;
        }

        //valid policer
        auto group_policer_map = orch->getTrapGroupPolicerMap();
        auto policer_itr = group_policer_map.find(grp_itr->second);
        if (policer_itr != group_policer_map.end()) {
            if (!ValidatePolicer(policer_itr->second, *exp_police_attr_list.get())) {
                return false;
            }
        }

        //valid trap
        auto trap_type_list = getTrapTypeList(rule_values);
        auto trap_map = orch->getTrapIdTrapGroupMap();
        for (auto trap_type : trap_type_list) {
            vector<FieldValueTuple> temp_rule_values;
            temp_rule_values.assign(rule_values.begin(), rule_values.end());
            replaceTrapType(temp_rule_values, trap_type);

            auto exp_trap_attr_list = getTrapAttributeList(grp_itr->second, temp_rule_values);
            auto trap_itr = trap_map.find(m_trap_id_map.at(trap_type));

            if (trap_itr == trap_map.end()) {
                return false;
            }

            if (!ValidateTrap(trap_itr->second, *exp_trap_attr_list.get())) {
                return false;
            }
        }

        return true;
    }
};

map<string, string> CoppOrchTest::gProfileMap;
map<string, string>::iterator CoppOrchTest::gProfileIter = CoppOrchTest::gProfileMap.begin();

TEST_F(CoppOrchTest, COPP_Create_STP_Rule)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "stp" },
        { copp_trap_action_field, "copy" },
        { copp_queue_field, "1" },
        { copp_trap_priority_field, "5" }        
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_STP_Rule_And_TRTCM_Policer)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "stp" },
        { copp_trap_action_field, "copy" },
        { copp_queue_field, "1" },
        { copp_trap_priority_field, "5" },
        { copp_policer_meter_type_field, "packets" },
        { copp_policer_mode_field, "tr_tcm" },
        { copp_policer_color_field, "blind" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_pir_field, "5" },
        { copp_policer_pbs_field, "1" },      
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_STP_Rule_And_SRTCM_Policer)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "stp" },
        { copp_trap_action_field, "copy" },
        { copp_queue_field, "1" },
        { copp_trap_priority_field, "5" },
        { copp_policer_meter_type_field, "packets" },
        { copp_policer_mode_field, "sr_tcm" },
        { copp_policer_color_field, "blind" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_STP_Rule_And_Storm_Policer)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "stp" },
        { copp_trap_action_field, "copy" },
        { copp_queue_field, "1" },
        { copp_trap_priority_field, "5" },
        { copp_policer_meter_type_field, "bytes" },
        { copp_policer_mode_field, "storm" },
        { copp_policer_color_field, "blind" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_pir_field, "5" },
        { copp_policer_pbs_field, "1" },
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_LACP_Rule)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "lacp" },
        { copp_trap_action_field, "deny" },
        { copp_queue_field, "7" },
        { copp_trap_priority_field, "4" },
        { copp_policer_meter_type_field, "bytes" },
        { copp_policer_mode_field, "sr_tcm" },
        { copp_policer_color_field, "blind" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_pir_field, "5" },
        { copp_policer_pbs_field, "1" },
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_EAPOL_Rule)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "eapol" },
        { copp_trap_action_field, "forward" },
        { copp_queue_field, "8" },
        { copp_trap_priority_field, "9" },
        { copp_policer_meter_type_field, "packets" },
        { copp_policer_mode_field, "storm" },
        { copp_policer_color_field, "aware" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_pir_field, "5" },
        { copp_policer_pbs_field, "1" },
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

TEST_F(CoppOrchTest, COPP_Create_All_Rule_In_One_Group)
{
    auto orch = createCoppOrch();

    string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = {
        { copp_trap_id_list, "stp,lacp,eapol" },
        { copp_trap_action_field, "drop" },
        { copp_queue_field, "3" },
        { copp_trap_priority_field, "1" },
        { copp_policer_meter_type_field, "bytes" },
        { copp_policer_mode_field, "tr_tcm" },
        { copp_policer_color_field, "blind" },
        { copp_policer_cir_field, "90" },
        { copp_policer_cbs_field, "10" },
        { copp_policer_pir_field, "5" },
        { copp_policer_pbs_field, "1" },
        { copp_policer_action_green_field, "forward" },
        { copp_policer_action_yellow_field, "drop" },
        { copp_policer_action_red_field, "deny" }
    };
    auto kvf_copp_value = deque<KeyOpFieldsValuesTuple>({ { trap_group_id, SET_COMMAND, rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));
}

}