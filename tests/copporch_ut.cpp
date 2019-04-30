#include "ut_helper.h"

#include "orchdaemon.h"

extern sai_object_id_t gSwitchId;

extern PortsOrch* gPortsOrch;

extern sai_hostif_api_t* sai_hostif_api;
extern sai_policer_api_t* sai_policer_api;
extern sai_switch_api_t* sai_switch_api;

namespace nsCoppOrchTest {

using namespace std;

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

struct CoppOrchHandler {
    CoppOrch* m_coppOrch;
    swss::DBConnector* app_db;

    CoppOrchHandler(CoppOrch* coppOrch, swss::DBConnector* app_db)
        : m_coppOrch(coppOrch)
        , app_db(app_db)
    {
    }

    operator const CoppOrch*() const
    {
        return m_coppOrch;
    }

    static size_t consumerAddToSync(Consumer* consumer, const std::deque<KeyOpFieldsValuesTuple>& entries)
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

    void doCoppTask(const std::deque<KeyOpFieldsValuesTuple>& entries)
    {
        auto consumer = std::unique_ptr<Consumer>(new Consumer(
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
};

struct CoppTestBase : public ::testing::Test {
    std::vector<int32_t*> m_s32list_pool;

    virtual ~CoppTestBase()
    {
        for (auto p : m_s32list_pool) {
            free(p);
        }
    }
};

struct CoppTest : public CoppTestBase {

    struct CreateCoppResult {
        bool ret_val;

        std::vector<sai_attribute_t> group_attr_list;
        std::vector<sai_attribute_t> trap_attr_list;
        std::vector<sai_attribute_t> policer_attr_list;
    };

    std::shared_ptr<swss::DBConnector> m_app_db;

    void SetUp() override
    {
        CoppTestBase::SetUp();
        m_app_db = std::make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    void TearDown() override
    {
        CoppTestBase::TearDown();
    }

    std::shared_ptr<CoppOrchHandler> createCoppOrch()
    {
        auto copp = new CoppOrch(m_app_db.get(), APP_COPP_TABLE_NAME);
        return std::make_shared<CoppOrchHandler>(copp, m_app_db.get());
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

    static std::map<std::string, std::string> gProfileMap;
    static std::map<std::string, std::string>::iterator gProfileIter;

    void SetUp() override
    {
        CoppTest::SetUp();

#if WITH_SAI == LIBVS
        sai_hostif_api = const_cast<sai_hostif_api_t*>(&vs_hostif_api);
        sai_policer_api = const_cast<sai_policer_api_t*>(&vs_policer_api);
        sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
#endif

        gProfileMap.emplace("SAI_VS_SWITCH_TYPE", "SAI_VS_SWITCH_TYPE_BCM56850");
        gProfileMap.emplace("KV_DEVICE_MAC_ADDRESS", "20:03:04:05:06:00");

        sai_service_method_table_t test_services = {
            profile_get_value,
            profile_get_next_value
        };

        auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

        sai_attribute_t swattr;

        swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
        swattr.value.booldata = true;

        status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

        //call orch->doTask need to initial portsOrch
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
        CoppTest::TearDown();

        auto status = sai_switch_api->remove_switch(gSwitchId);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
        gSwitchId = 0;

        sai_api_uninitialize();

        delete gPortsOrch;
        gPortsOrch = nullptr;

        sai_hostif_api = nullptr;
        sai_policer_api = nullptr;
        sai_switch_api = nullptr;
    }

    std::shared_ptr<SaiAttributeList> getTrapGroupAttributeList(const vector<FieldValueTuple> rule_values)
    {
        std::vector<swss::FieldValueTuple> fields;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_queue_field)
                fields.push_back({ "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", fvValue(it) });
        }

        return std::shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, fields, false));
    }

    std::shared_ptr<SaiAttributeList> getTrapAttributeList(const vector<FieldValueTuple> rule_values)
    {
        std::vector<swss::FieldValueTuple> fields;
        for (auto it : rule_values) {
            if (kfvKey(it) == copp_trap_action_field) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", m_packet_action_map.at(fvValue(it)) });
            } else if (kfvKey(it) == copp_trap_action_field) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", fvValue(it) });
            } else if (kfvKey(it) == copp_trap_id_list) {
                fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", m_trap_type_map.at(fvValue(it)) });
            }
        }
        fields.push_back({ "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" });

        return std::shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_HOSTIF_TRAP, fields, false));
    }

    std::shared_ptr<SaiAttributeList> getPoliceAttributeList(const vector<FieldValueTuple> rule_values)
    {
        std::vector<swss::FieldValueTuple> fields;

        return std::shared_ptr<SaiAttributeList>(new SaiAttributeList(SAI_OBJECT_TYPE_POLICER, fields, false));
    }

    vector<sai_hostif_trap_type_t> getTrapTypeList(const vector<FieldValueTuple> ruleAttr)
    {
        std::vector<sai_hostif_trap_type_t> types;
        for (auto it : ruleAttr) {
            if (kfvKey(it) == copp_trap_id_list) {
                types.push_back({ m_trap_id_map.at(fvValue(it)) });
            }
        }

        return types;
    }

    bool ValidateTrapGroup(sai_object_id_t id, SaiAttributeList& exp_group_attr_list)
    {
        sai_object_type_t trapGroupObjectType = SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP;
        std::vector<sai_attribute_t> trap_group_act_attr;

        for (int i = 0; i < exp_group_attr_list.get_attr_count(); ++i) {
            const auto attr = exp_group_attr_list.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(trapGroupObjectType, attr.id);

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

        auto b_attr_eq = Check::AttrListEq(trapGroupObjectType, trap_group_act_attr, exp_group_attr_list);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }

    bool ValidateTrap(sai_object_id_t id, SaiAttributeList& exp_trap_attr_list)
    {
        sai_object_type_t trapObjectType = SAI_OBJECT_TYPE_HOSTIF_TRAP;
        std::vector<sai_attribute_t> trap_act_attr;

        for (int i = 0; i < exp_trap_attr_list.get_attr_count(); ++i) {
            const auto attr = exp_trap_attr_list.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(trapObjectType, attr.id);

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

        auto b_attr_eq = Check::AttrListEq(trapObjectType, trap_act_attr, exp_trap_attr_list);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }

    bool Validate(CoppOrchHandler* orch, const std::string& groupName, const vector<FieldValueTuple>& rule_values)
    {
        auto exp_group_attr_list = getTrapGroupAttributeList(rule_values);
        auto exp_trap_attr_list = getTrapAttributeList(rule_values);
        auto type_list = getTrapTypeList(rule_values);
        auto exp_police_attr_list = getPoliceAttributeList(rule_values);

        //valid trap group
        auto trap_group_map = orch->getTrapGroupMap();
        auto grpIt = trap_group_map.find(groupName);
        if (grpIt == trap_group_map.end()) {
            return false;
        }

        if (!ValidateTrapGroup(grpIt->second, *exp_group_attr_list.get())) {
            return false;
        }

        //valid trap
        auto trap_map = orch->getTrapIdTrapGroupMap();
        for (auto trap_type : type_list) {
            auto trapIt = trap_map.find(trap_type);
            if (trapIt == trap_map.end()) {
                return false;
            }

            if (!ValidateTrap(trapIt->second, *exp_trap_attr_list.get())) {
                return false;
            }
        }

        return true;
    }
};

std::map<std::string, std::string> CoppOrchTest::gProfileMap;
std::map<std::string, std::string>::iterator CoppOrchTest::gProfileIter = CoppOrchTest::gProfileMap.begin();

TEST_F(CoppOrchTest, create_copp_stp_rule_without_policer)
{
    auto orch = createCoppOrch();

    std::string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = { { "trap_ids", "stp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } };
    auto kvf_copp_value = std::deque<KeyOpFieldsValuesTuple>({ { trap_group_id, "SET", rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));

    // KeyOpFieldsValuesTuple delActionAttr(groupName, "DEL", {});
    // setData = { delActionAttr };
    // consumerAddToSync(consumer.get(), setData);

    // //call CoPP function
    // coppMock.processCoppRule(*consumer);

    // const auto& trapGroupTables = coppMock.getTrapGroupMap();
    // auto grpIt = trapGroupTables.find(groupName);

    // ASSERT_TRUE(grpIt == trapGroupTables.end());
}

TEST_F(CoppOrchTest, create_copp_lacp_rule_without_policer)
{
    auto orch = createCoppOrch();

    std::string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = { { "trap_ids", "lacp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } };
    auto kvf_copp_value = std::deque<KeyOpFieldsValuesTuple>({ { trap_group_id, "SET", rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));

    // KeyOpFieldsValuesTuple delActionAttr(groupName, "DEL", {});
    // setData = { delActionAttr };
    // consumerAddToSync(consumer.get(), setData);

    // //call CoPP function
    // coppMock.processCoppRule(*consumer);

    // const auto& trapGroupTables = coppMock.getTrapGroupMap();
    // auto grpIt = trapGroupTables.find(groupName);

    // ASSERT_TRUE(grpIt == trapGroupTables.end());
}

TEST_F(CoppOrchTest, create_copp_eapol_rule_without_policer)
{
    auto orch = createCoppOrch();

    std::string trap_group_id = "coppRule1";
    vector<FieldValueTuple> rule_values = { { "trap_ids", "eapol" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } };
    auto kvf_copp_value = std::deque<KeyOpFieldsValuesTuple>({ { trap_group_id, "SET", rule_values } });
    orch->doCoppTask(kvf_copp_value);

    ASSERT_TRUE(Validate(orch.get(), trap_group_id, rule_values));

    // KeyOpFieldsValuesTuple delActionAttr(groupName, "DEL", {});
    // setData = { delActionAttr };
    // consumerAddToSync(consumer.get(), setData);

    // //call CoPP function
    // coppMock.processCoppRule(*consumer);

    // const auto& trapGroupTables = coppMock.getTrapGroupMap();
    // auto grpIt = trapGroupTables.find(groupName);

    // ASSERT_TRUE(grpIt == trapGroupTables.end());
}

// TEST_F(CoppTest, delete_copp_eapol_rule_without_policer)
// {
//     sai_hostif_api->create_hostif_trap_group = sai_create_hostif_trap_group_;
//     sai_hostif_api->remove_hostif_trap_group = sai_remove_hostif_trap_group_;
//     sai_hostif_api->create_hostif_trap = sai_create_hostif_trap_;
//     sai_hostif_api->create_hostif_table_entry = sai_create_hostif_table_entry_;
//     sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;

//     that = this;

//     auto ret = std::make_shared<CreateCoppResult>();

//     sai_create_hostif_trap_group_fn =
//         [&](sai_object_id_t* hostif_trap_group_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         for (auto i = 0; i < attr_count; ++i) {
//             ret->group_attr_list.emplace_back(attr_list[i]);
//         }
//         *hostif_trap_group_id = 12345l;
//         return SAI_STATUS_SUCCESS;
//     };

//     bool b_check_delete = false;

//     sai_remove_hostif_trap_group_fn =
//         [&](sai_object_id_t hostif_trap_group_id) -> sai_status_t {
//         b_check_delete = true;
//         return SAI_STATUS_SUCCESS;
//     };

//     sai_create_hostif_trap_fn =
//         [&](sai_object_id_t* hostif_trap_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         bool defaultTrap = false;
//         for (auto i = 0; i < attr_count; ++i) {
//             if (attr_list[i].id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE) {
//                 if (attr_list[i].value.s32 == SAI_HOSTIF_TRAP_TYPE_TTL_ERROR)
//                     defaultTrap = true;
//                 break;
//             }
//         }

//         if (!defaultTrap) {
//             // FIXME: should not hard code !!
//             *hostif_trap_id = 12345l;
//             for (auto i = 0; i < attr_count; ++i) {
//                 ret->trap_attr_list.emplace_back(attr_list[i]);
//             }
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     auto appl_Db = swss::DBConnector(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
//     auto coppMock = CoppOrchMock(&appl_Db, APP_COPP_TABLE_NAME);
//     auto consumer = std::unique_ptr<Consumer>(new Consumer(new swss::ConsumerStateTable(&appl_Db, std::string(APP_COPP_TABLE_NAME), 1, 1), &coppMock, std::string(APP_COPP_TABLE_NAME)));

//     KeyOpFieldsValuesTuple addRuleAttr("coppRule1", "SET", { { "trap_ids", "eapol" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } });
//     std::deque<KeyOpFieldsValuesTuple> setData = { addRuleAttr };
//     consumerAddToSync(consumer.get(), setData);

//     auto groupValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", "1" } });
//     SaiAttributeList group_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, groupValue, false);

//     auto trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "3" }, { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", "1" } });
//     SaiAttributeList trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

//     //call CoPP function
//     coppMock.processCoppRule(*consumer);

//     ASSERT_TRUE(AttrListEq(ret->group_attr_list, group_attr_list));
//     ASSERT_TRUE(AttrListEq(ret->trap_attr_list, trap_attr_list));

//     KeyOpFieldsValuesTuple delAttr("coppRule1", "DEL", { { "trap_ids", "eapol" } });
//     setData = { delAttr };
//     consumerAddToSync(consumer.get(), setData);

//     //call CoPP function
//     coppMock.processCoppRule(*consumer);

//     //verify
//     ASSERT_TRUE(b_check_delete);

//     //teardown
//     sai_hostif_api->create_hostif_trap_group = NULL;
//     sai_hostif_api->create_hostif_trap = NULL;
//     sai_hostif_api->create_hostif_table_entry = NULL;
//     sai_switch_api->get_switch_attribute = NULL;
// }

// TEST_F(CoppTest, create_copp_stp_rule_with_policer)
// {
//     sai_hostif_api->create_hostif_trap_group = sai_create_hostif_trap_group_;
//     sai_hostif_api->create_hostif_trap = sai_create_hostif_trap_;
//     sai_hostif_api->create_hostif_table_entry = sai_create_hostif_table_entry_;
//     sai_hostif_api->set_hostif_trap_group_attribute = sai_set_hostif_trap_group_attribute_;
//     sai_policer_api->create_policer = sai_create_policer_;
//     sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;

//     that = this;
//     auto ret = std::make_shared<CreateCoppResult>();

//     sai_create_hostif_trap_group_fn =
//         [&](sai_object_id_t* hostif_trap_group_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         for (auto i = 0; i < attr_count; ++i) {
//             ret->group_attr_list.emplace_back(attr_list[i]);
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     sai_create_hostif_trap_fn =
//         [&](sai_object_id_t* hostif_trap_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         bool defaultTrap = false;
//         for (auto i = 0; i < attr_count; ++i) {
//             if (attr_list[i].id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE) {
//                 if (attr_list[i].value.s32 == SAI_HOSTIF_TRAP_TYPE_TTL_ERROR) {
//                     defaultTrap = true;
//                     break;
//                 }
//             }
//         }

//         if (!defaultTrap) {
//             // FIXME: should not hard code !!
//             *hostif_trap_id = 12345l;
//             for (auto i = 0; i < attr_count; ++i) {
//                 ret->trap_attr_list.emplace_back(attr_list[i]);
//             }
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     sai_create_policer_fn =
//         [&](sai_object_id_t* policer_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         for (auto i = 0; i < attr_count; ++i) {
//             ret->policer_attr_list.emplace_back(attr_list[i]);
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     auto appl_Db = swss::DBConnector(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
//     auto coppMock = CoppOrchMock(&appl_Db, APP_COPP_TABLE_NAME);
//     auto consumer = std::unique_ptr<Consumer>(new Consumer(new swss::ConsumerStateTable(&appl_Db, std::string(APP_COPP_TABLE_NAME), 1, 1), &coppMock, std::string(APP_COPP_TABLE_NAME)));

//     KeyOpFieldsValuesTuple addRuleAttr("coppRule1", "SET", { { "trap_ids", "stp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" }, { "meter_type", "packets" }, { "mode", "sr_tcm" }, { "color", "aware" }, { "cir", "90" }, { "cbs", "10" }, { "pir", "5" }, { "pbs", "1" }, { "green_action", "forward" }, { "yellow_action", "drop" }, { "red_action", "deny" } });
//     std::deque<KeyOpFieldsValuesTuple> setData = { addRuleAttr };

//     consumerAddToSync(consumer.get(), setData);

//     auto groupValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", "1" } });
//     SaiAttributeList group_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, groupValue, false);

//     auto trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "1" },
//         { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" },
//         { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" },
//         { "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", "1" } });
//     SaiAttributeList trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

//     auto policerValue = std::vector<swss::FieldValueTuple>({ { "SAI_POLICER_ATTR_METER_TYPE", "1" },
//         { "SAI_POLICER_ATTR_MODE", "0" },
//         { "SAI_POLICER_ATTR_COLOR_SOURCE", "2" },
//         { "SAI_POLICER_ATTR_CBS", "1" },
//         { "SAI_POLICER_ATTR_CIR", "1" },
//         { "SAI_POLICER_ATTR_PBS", "1" },
//         { "SAI_POLICER_ATTR_PIR", "1" },
//         { "SAI_POLICER_ATTR_GREEN_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" },
//         { "SAI_POLICER_ATTR_RED_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" },
//         { "SAI_POLICER_ATTR_YELLOW_PACKET_ACTION", "SAI_PACKET_ACTION_DENY" } });
//     SaiAttributeList policer_attr_list(SAI_OBJECT_TYPE_POLICER, policerValue, false);

//     //call CoPP function
//     coppMock.processCoppRule(*consumer);

//     //verify
//     ASSERT_TRUE(AttrListEq(ret->group_attr_list, group_attr_list));
//     ASSERT_TRUE(AttrListEq(ret->trap_attr_list, trap_attr_list));
//     ASSERT_TRUE(AttrListEq(ret->policer_attr_list, policer_attr_list));

//     //teardown
//     sai_hostif_api->create_hostif_trap_group = NULL;
//     sai_hostif_api->create_hostif_trap = NULL;
//     sai_hostif_api->create_hostif_table_entry = NULL;
//     sai_hostif_api->set_hostif_trap_group_attribute = NULL;
//     sai_policer_api->create_policer = NULL;
//     sai_switch_api->get_switch_attribute = NULL;
// }

// TEST_F(CoppTest, delete_copp_stp_rule_with_policer)
// {
//     sai_hostif_api->create_hostif_trap_group = sai_create_hostif_trap_group_;
//     sai_hostif_api->remove_hostif_trap_group = sai_remove_hostif_trap_group_;
//     sai_hostif_api->set_hostif_trap_group_attribute = sai_set_hostif_trap_group_attribute_;
//     sai_hostif_api->create_hostif_trap = sai_create_hostif_trap_;
//     sai_hostif_api->create_hostif_table_entry = sai_create_hostif_table_entry_;
//     sai_policer_api->create_policer = sai_create_policer_;
//     sai_policer_api->remove_policer = sai_remove_policer_;
//     sai_switch_api->get_switch_attribute = sai_get_switch_attribute_;

//     that = this;

//     auto ret = std::make_shared<CreateCoppResult>();

//     sai_create_hostif_trap_group_fn =
//         [&](sai_object_id_t* hostif_trap_group_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         for (auto i = 0; i < attr_count; ++i) {
//             ret->group_attr_list.emplace_back(attr_list[i]);
//         }
//         *hostif_trap_group_id = 12345l;
//         return SAI_STATUS_SUCCESS;
//     };

//     bool b_check_delete = false;

//     sai_remove_hostif_trap_group_fn =
//         [&](sai_object_id_t hostif_trap_group_id) -> sai_status_t {
//         b_check_delete = true;
//         return SAI_STATUS_SUCCESS;
//     };

//     sai_create_hostif_trap_fn =
//         [&](sai_object_id_t* hostif_trap_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         bool defaultTrap = false;
//         for (auto i = 0; i < attr_count; ++i) {
//             if (attr_list[i].id == SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE) {
//                 if (attr_list[i].value.s32 == SAI_HOSTIF_TRAP_TYPE_TTL_ERROR)
//                     defaultTrap = true;
//                 break;
//             }
//         }

//         if (!defaultTrap) {
//             // FIXME: should not hard code !!
//             *hostif_trap_id = 12345l;
//             for (auto i = 0; i < attr_count; ++i) {
//                 ret->trap_attr_list.emplace_back(attr_list[i]);
//             }
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     sai_create_policer_fn =
//         [&](sai_object_id_t* policer_id,
//             sai_object_id_t switch_id,
//             uint32_t attr_count,
//             const sai_attribute_t* attr_list) -> sai_status_t {
//         for (auto i = 0; i < attr_count; ++i) {
//             ret->policer_attr_list.emplace_back(attr_list[i]);
//         }
//         return SAI_STATUS_SUCCESS;
//     };

//     bool check_policer_delete = false;
//     sai_remove_policer_fn =
//         [&](sai_object_id_t policer_id) -> sai_status_t {
//         check_policer_delete = true;
//         return SAI_STATUS_SUCCESS;
//     };

//     auto appl_Db
//         = swss::DBConnector(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
//     auto coppMock = CoppOrchMock(&appl_Db, APP_COPP_TABLE_NAME);
//     auto consumer = std::unique_ptr<Consumer>(new Consumer(new swss::ConsumerStateTable(&appl_Db, std::string(APP_COPP_TABLE_NAME), 1, 1), &coppMock, std::string(APP_COPP_TABLE_NAME)));

//     KeyOpFieldsValuesTuple addRuleAttr("coppRule1", "SET", { { "trap_ids", "stp" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" }, { "meter_type", "packets" }, { "mode", "sr_tcm" }, { "color", "aware" }, { "cir", "90" }, { "cbs", "10" }, { "pir", "5" }, { "pbs", "1" }, { "green_action", "forward" }, { "yellow_action", "drop" }, { "red_action", "deny" } });
//     std::deque<KeyOpFieldsValuesTuple> setData = { addRuleAttr };
//     consumerAddToSync(consumer.get(), setData);

//     auto groupValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_GROUP_ATTR_QUEUE", "1" } });
//     SaiAttributeList group_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, groupValue, false);

//     auto trapValue = std::vector<swss::FieldValueTuple>({ { "SAI_HOSTIF_TRAP_ATTR_TRAP_TYPE", "1" }, { "SAI_HOSTIF_TRAP_ATTR_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_GROUP", "oid:0x3" }, { "SAI_HOSTIF_TRAP_ATTR_TRAP_PRIORITY", "1" } });
//     SaiAttributeList trap_attr_list(SAI_OBJECT_TYPE_HOSTIF_TRAP, trapValue, false);

//     auto policerValue = std::vector<swss::FieldValueTuple>({ { "SAI_POLICER_ATTR_METER_TYPE", "1" },
//         { "SAI_POLICER_ATTR_MODE", "0" },
//         { "SAI_POLICER_ATTR_COLOR_SOURCE", "2" },
//         { "SAI_POLICER_ATTR_CBS", "1" },
//         { "SAI_POLICER_ATTR_CIR", "1" },
//         { "SAI_POLICER_ATTR_PBS", "1" },
//         { "SAI_POLICER_ATTR_PIR", "1" },
//         { "SAI_POLICER_ATTR_GREEN_PACKET_ACTION", "SAI_PACKET_ACTION_FORWARD" },
//         { "SAI_POLICER_ATTR_RED_PACKET_ACTION", "SAI_PACKET_ACTION_DROP" },
//         { "SAI_POLICER_ATTR_YELLOW_PACKET_ACTION", "SAI_PACKET_ACTION_DENY" } });
//     SaiAttributeList policer_attr_list(SAI_OBJECT_TYPE_POLICER, policerValue, false);

//     //call CoPP function
//     coppMock.processCoppRule(*consumer);

//     ASSERT_TRUE(AttrListEq(ret->group_attr_list, group_attr_list));
//     ASSERT_TRUE(AttrListEq(ret->trap_attr_list, trap_attr_list));

//     KeyOpFieldsValuesTuple delAttr("coppRule1", "DEL", { { "trap_ids", "stp" } });
//     setData = { delAttr };
//     consumerAddToSync(consumer.get(), setData);

//     //call CoPP function
//     coppMock.processCoppRule(*consumer);

//     //verify
//     ASSERT_TRUE(b_check_delete);
//     ASSERT_TRUE(check_policer_delete);

//     //teardown
//     sai_hostif_api->create_hostif_trap_group = NULL;
//     sai_hostif_api->create_hostif_trap = NULL;
//     sai_hostif_api->create_hostif_table_entry = NULL;
//     sai_hostif_api->set_hostif_trap_group_attribute = NULL;
//     sai_policer_api->create_policer = NULL;
//     sai_policer_api->remove_policer = NULL;
//     sai_switch_api->get_switch_attribute = NULL;
// }
}