#include "ut_helper.h"

#include "orchdaemon.h"

#include "jansson.hpp"

namespace json {

    class Object : public json::Value {
    public:
        Object() : Value(json::object()) {
        }

        Object(std::initializer_list< std::pair<const char*, Value> > l)
        : Object() {
            for (auto p: l) {
                this->operator[](p.first) = p.second;
            }
        }
    };

    class Array : public json::Value {
    public:
        Array() : Value(json::array()) {
        }

        Array(std::initializer_list< Value > l)
        : Array() {
            int i = 0;
            for (auto p: l) {
                set_at(i++, p);
            }
        }
    };
}

extern sai_object_id_t gSwitchId;

extern PortsOrch* gPortsOrch;

extern sai_switch_api_t* sai_switch_api;
extern sai_qos_map_api_t* sai_qos_map_api;
extern sai_wred_api_t* sai_wred_api;
extern sai_port_api_t* sai_port_api;
extern sai_vlan_api_t* sai_vlan_api;
extern sai_bridge_api_t* sai_bridge_api;

namespace nsQosOrchTest {

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

struct TestBase : public ::testing::Test {

    bool AttrListEq(sai_object_type_t object_type, const vector<sai_attribute_t>& act_attr_list, /*const*/ SaiAttributeList& exp_attr_list)
    {
        if (act_attr_list.size() != exp_attr_list.get_attr_count()) {
            return false;
        }

        auto l = exp_attr_list.get_attr_list();
        for (int i = 0; i < exp_attr_list.get_attr_count(); ++i) {
            sai_attr_id_t id = exp_attr_list.get_attr_list()[i].id;
            auto meta = sai_metadata_get_attr_metadata(object_type, id);

            assert(meta != nullptr);

            char act_buf[0x4000];
            char exp_buf[0x4000];

            int act_len;
            int exp_len;

            if (meta->attrvaluetype != _sai_attr_value_type_t::SAI_ATTR_VALUE_TYPE_QOS_MAP_LIST) {
                act_len = sai_serialize_attribute_value(act_buf, meta, &act_attr_list[i].value);
                exp_len = sai_serialize_attribute_value(exp_buf, meta, &exp_attr_list.get_attr_list()[i].value);
            } else {
                act_len = sai_serialize_qos_map_list(act_buf, &act_attr_list[i].value.qosmap);
                exp_len = sai_serialize_qos_map_list(exp_buf, &exp_attr_list.get_attr_list()[i].value.qosmap);
            }

            // auto act = sai_serialize_attr_value(*meta, act_attr_list[i].value, false);
            // auto exp = sai_serialize_attr_value(*meta, &exp_attr_list.get_attr_list()[i].value, false);

            assert(act_len < sizeof(act_buf));
            assert(exp_len < sizeof(exp_buf));

            if (act_len != exp_len) {
                cout << "AttrListEq failed\n";
                cout << "Actual:   " << act_buf << "\n";
                cout << "Expected: " << exp_buf << "\n";
                return false;
            }

            if (strcmp(act_buf, exp_buf) != 0) {
                cout << "AttrListEq failed\n";
                cout << "Actual:   " << act_buf << "\n";
                cout << "Expected: " << exp_buf << "\n";
                return false;
            }
        }

        return true;
    }
};

struct QosMapHandlerTest : public TestBase {
    struct SetQosResult {
        bool ret_val;
        sai_object_id_t sai_object_id;
        vector<sai_attribute_t> attr_list;
    };

    shared_ptr<SetQosResult> setDscp2Tc(DscpToTcMapHandler& dscp_to_tc, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_QOS_MAP, SAI_OBJECT_TYPE_QOS_MAP>(&sai_qos_map_api->create_qos_map);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        dscp_to_tc.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = dscp_to_tc.addQosItem(attrs);
        return ret;
    }

    shared_ptr<SetQosResult> setTc2Queue(TcToQueueMapHandler& tc_to_queue, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_QOS_MAP, SAI_OBJECT_TYPE_QOS_MAP>(&sai_qos_map_api->create_qos_map);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        tc_to_queue.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = tc_to_queue.addQosItem(attrs);
        return ret;
    }

    shared_ptr<SetQosResult> setTc2Pg(TcToPgHandler& tc_to_pg, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_QOS_MAP, SAI_OBJECT_TYPE_QOS_MAP>(&sai_qos_map_api->create_qos_map);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        tc_to_pg.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = tc_to_pg.addQosItem(attrs);
        return ret;
    }

    shared_ptr<SetQosResult> setPfcPrio2Pg(PfcPrioToPgHandler& pfc_prio_to_pg, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_QOS_MAP, SAI_OBJECT_TYPE_QOS_MAP>(&sai_qos_map_api->create_qos_map);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        pfc_prio_to_pg.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = pfc_prio_to_pg.addQosItem(attrs);
        return ret;
    }

    shared_ptr<SetQosResult> setPfc2Queue(PfcToQueueHandler& pfc_to_queue, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_QOS_MAP, SAI_OBJECT_TYPE_QOS_MAP>(&sai_qos_map_api->create_qos_map);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        pfc_to_queue.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = pfc_to_queue.addQosItem(attrs);
        return ret;
    }

    shared_ptr<SetQosResult> addWredProfile(WredMapHandler& wred_map, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_wred_api == nullptr);

        sai_wred_api = new sai_wred_api_t();
        auto sai_qos = shared_ptr<sai_wred_api_t>(sai_wred_api, [](sai_wred_api_t* p) {
            delete p;
            sai_wred_api = nullptr;
        });

        auto ret = make_shared<SetQosResult>();

        auto spy = SpyOn<SAI_API_WRED, SAI_OBJECT_TYPE_WRED>(&sai_wred_api->create_wred);
        spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_SUCCESS;
        });

        vector<sai_attribute_t> attrs;
        wred_map.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = wred_map.addQosItem(attrs);
        return ret;
    }

    // shared_ptr<SetQosResult> deleteWredProfile(WredMapHandler& wred_map, KeyOpFieldsValuesTuple& tuple)
    // {
    //     assert(sai_wred_api == nullptr);

    //     sai_wred_api = new sai_wred_api_t();
    //     auto sai_qos = shared_ptr<sai_wred_api_t>(sai_wred_api, [](sai_wred_api_t* p) {
    //         delete p;
    //         sai_wred_api = nullptr;
    //     });

    //     auto ret = make_shared<SetQosResult>();

    //     auto spy = SpyOn<SAI_API_WRED, SAI_OBJECT_TYPE_WRED>(&sai_wred_api->create_wred);
    //     spy->callFake([&](sai_object_id_t* oid, sai_object_id_t, uint32_t attr_count, const sai_attribute_t* attr_list) -> sai_status_t {
    //         for (auto i = 0; i < attr_count; ++i) {
    //             ret->attr_list.emplace_back(attr_list[i]);
    //         }
    //         return SAI_STATUS_SUCCESS;
    //     });

    //     spy = SpyOn<SAI_API_WRED, SAI_OBJECT_TYPE_WRED>(&sai_wred_api->remove_wred);
    //     spy->callFake([&](sai_object_id_t oid) -> sai_status_t {
    //         if (ret->sai_object_id == sai_object_id) {
    //             return SAI_STATUS_SUCCESS;
    //         } else {
    //             return SAI_STATUS_FAILURE;
    //         }
    //     });

    //     vector<sai_attribute_t> attrs;
    //     wred_map.convertFieldValuesToAttributes(tuple, attrs);
    //     wred_map.addQosItem(attrs);

    //     ret->ret_val = wred_map.removeQosItem(ret->sai_object_id);
    //     return ret;
    // }
};

TEST_F(QosMapHandlerTest, Dscp_To_Tc_Map)
{
    DscpToTcMapHandler dscp_to_tc_map_handler;
    KeyOpFieldsValuesTuple dscp_to_tc_tuple(CFG_DSCP_TO_TC_MAP_TABLE_NAME, SET_COMMAND,
        { { "1", "0" }, { "2", "0" }, { "3", "3" } });

    auto res = setDscp2Tc(dscp_to_tc_map_handler, dscp_to_tc_tuple);

    ASSERT_TRUE(res->ret_val == true);

    json::Object map_to_value_list {
        {"count", json::Value(3)},
        {"list", json::Array {
            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(1)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(2)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(3)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(3)},
                }},
            },
        }}
    };
    // auto v = vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_DSCP_TO_TC" },
    //     { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":1,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //      \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":2,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":3,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3}}]}" } });
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_DSCP_TO_TC" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", map_to_value_list.save_string(JSON_COMPACT) } });

    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, Tc_To_Queue_Map)
{
    TcToQueueMapHandler tc_to_queue_map_handler;
    KeyOpFieldsValuesTuple tc_to_queue_tuple(CFG_TC_TO_QUEUE_MAP_TABLE_NAME, SET_COMMAND,
        { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setTc2Queue(tc_to_queue_map_handler, tc_to_queue_tuple);

    ASSERT_TRUE(res->ret_val == true);

    json::Object map_to_value_list {
        {"count", json::Value(3)},
        {"list", json::Array {
            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(1)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(1)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(3)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(3)},
                    {"tc", json::Value(0)},
                }},
            },
        }}
    };
    // auto v = vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
    //     { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":1},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":1,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", map_to_value_list.save_string(JSON_COMPACT) } });

    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, Tc_To_Pg_Map)
{
    TcToPgHandler tc_to_pg_handler;
    KeyOpFieldsValuesTuple tc_to_pg_tuple(CFG_TC_TO_PRIORITY_GROUP_MAP_TABLE_NAME, SET_COMMAND,
        { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setTc2Pg(tc_to_pg_handler, tc_to_pg_tuple);

    ASSERT_TRUE(res->ret_val == true);

    json::Object map_to_value_list {
        {"count", json::Value(3)},
        {"list", json::Array {
            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(1)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(1)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(3)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(3)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },
        }}
    };
    // auto v = vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP" },
    //     { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":1},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":1,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":3,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", map_to_value_list.save_string(JSON_COMPACT) } });

    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, Pfc_Prio_To_Pg_Map)
{
    PfcPrioToPgHandler pfc_prio_to_pg_handler;
    KeyOpFieldsValuesTuple pfc_prio_to_pg_tuple(CFG_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP_TABLE_NAME, SET_COMMAND,
        { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setPfcPrio2Pg(pfc_prio_to_pg_handler, pfc_prio_to_pg_tuple);

    ASSERT_TRUE(res->ret_val == true);

    json::Object map_to_value_list {
        {"count", json::Value(3)},
        {"list", json::Array {
            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(1)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(1)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(3)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(3)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },
        }}
    };
    // auto v = vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP" },
    //     { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":1,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":1,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":3,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":3,\"prio\":0,\"qidx\":0,\"tc\":0}}]}" } });
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", map_to_value_list.save_string(JSON_COMPACT) } });

    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, Pfc_To_Queue_Map)
{
    PfcToQueueHandler pfc_to_queue_handler;
    KeyOpFieldsValuesTuple pfc_to_queue_tuple(CFG_PFC_PRIORITY_TO_QUEUE_MAP_TABLE_NAME, SET_COMMAND,
        { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setPfc2Queue(pfc_to_queue_handler, pfc_to_queue_tuple);

    ASSERT_TRUE(res->ret_val == true);

    json::Object map_to_value_list {
        {"count", json::Value(3)},
        {"list", json::Array {
            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(1)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(1)},
                    {"tc", json::Value(0)},
                }},
            },

            json::Object {
                {"key", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_RED")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(3)},
                    {"qidx", json::Value(0)},
                    {"tc", json::Value(0)},
                }},
                {"value", json::Object {
                    {"color", json::Value("SAI_PACKET_COLOR_GREEN")},
                    {"dot1p", json::Value(0)},
                    {"dscp", json::Value(0)},
                    {"pg", json::Value(0)},
                    {"prio", json::Value(0)},
                    {"qidx", json::Value(3)},
                    {"tc", json::Value(0)},
                }},
            },
        }}
    };
    // auto v = vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE" },
    //     { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":1,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":1,\"tc\":0}},{\
    //     \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":3,\"qidx\":0,\"tc\":0},\
    //     \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", map_to_value_list.save_string(JSON_COMPACT) } });

    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, Add_Wred_Profile)
{
    WredMapHandler wred_map_handler;
    KeyOpFieldsValuesTuple wred_profile_tuple(CFG_WRED_PROFILE_TABLE_NAME, SET_COMMAND,
        { { "wred_yellow_enable", "true" },
            { "wred_red_enable", "true" },
            { "ecn", "ecn_all" },
            { "red_max_threshold", "312000" },
            { "red_min_threshold", "104000" },
            { "yellow_max_threshold", "312000" },
            { "yellow_min_threshold", "104000" },
            { "green_max_threshold", "312000" },
            { "green_min_threshold", "104000" } });

    auto res = addWredProfile(wred_map_handler, wred_profile_tuple);

    ASSERT_TRUE(res->ret_val == true);

    auto v = vector<swss::FieldValueTuple>({ { "SAI_WRED_ATTR_WEIGHT", "0" },
        { "SAI_WRED_ATTR_YELLOW_ENABLE", "true" },
        { "SAI_WRED_ATTR_RED_ENABLE", "true" },
        { "SAI_WRED_ATTR_ECN_MARK_MODE", "SAI_ECN_MARK_MODE_ALL" },
        { "SAI_WRED_ATTR_RED_MAX_THRESHOLD", "312000" },
        { "SAI_WRED_ATTR_RED_MIN_THRESHOLD", "104000" },
        { "SAI_WRED_ATTR_YELLOW_MAX_THRESHOLD", "312000" },
        { "SAI_WRED_ATTR_YELLOW_MIN_THRESHOLD", "104000" },
        { "SAI_WRED_ATTR_GREEN_MAX_THRESHOLD", "312000" },
        { "SAI_WRED_ATTR_GREEN_MIN_THRESHOLD", "104000" },
        { "SAI_WRED_ATTR_GREEN_DROP_PROBABILITY", "100" },
        { "SAI_WRED_ATTR_YELLOW_DROP_PROBABILITY", "100" },
        { "SAI_WRED_ATTR_RED_DROP_PROBABILITY", "100" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_WRED, v, false);

    ASSERT_TRUE(Check::AttrListEq_Miss_objecttype_Dont_Use(res->attr_list, attr_list));
}

// TEST_F(QosMapHandlerTest, DeleteWredProfile)
// {
//     WredMapHandler wred_map_handler;
//     KeyOpFieldsValuesTuple wred_profile_tuple(CFG_WRED_PROFILE_TABLE_NAME, SET_COMMAND,
//         { { "wred_yellow_enable", "true" },
//             { "wred_red_enable", "true" },
//             { "ecn", "ecn_all" },
//             { "red_max_threshold", "312000" },
//             { "red_min_threshold", "104000" },
//             { "yellow_max_threshold", "312000" },
//             { "yellow_min_threshold", "104000" },
//             { "green_max_threshold", "312000" },
//             { "green_min_threshold", "104000" } });

//     auto res = deleteWredProfile(wred_map_handler, wred_profile_tuple);

//     ASSERT_TRUE(res->ret_val == true);
// }

struct MockQosOrch {
    QosOrch* m_qos_orch;
    swss::DBConnector* config_db;

    MockQosOrch(QosOrch* qos_orch, swss::DBConnector* config_db)
        : m_qos_orch(qos_orch)
        , config_db(config_db)
    {
    }

    operator const QosOrch*() const
    {
        return m_qos_orch;
    }

    type_map& getTypeMap()
    {
        //SWSS_LOG_ENTER();
        return m_qos_orch->m_qos_maps;
    }

    void doQosMapTask(const deque<KeyOpFieldsValuesTuple>& entries, const string& tableName)
    {
        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(config_db, tableName, 1, 1), m_qos_orch, tableName));

        consumerAddToSync(consumer.get(), entries);

        static_cast<Orch*>(m_qos_orch)->doTask(*consumer);
    }
};

struct QosOrchTest : public TestBase {

    shared_ptr<swss::DBConnector> m_app_db;
    shared_ptr<swss::DBConnector> m_config_db;
    vector<sai_qos_map_t*> m_qos_map_list_pool;
    QosOrch* qos_orch;

    vector<string> qos_tables = {
        CFG_TC_TO_QUEUE_MAP_TABLE_NAME,
        CFG_SCHEDULER_TABLE_NAME,
        CFG_DSCP_TO_TC_MAP_TABLE_NAME,
        CFG_QUEUE_TABLE_NAME,
        CFG_PORT_QOS_MAP_TABLE_NAME,
        CFG_WRED_PROFILE_TABLE_NAME,
        CFG_TC_TO_PRIORITY_GROUP_MAP_TABLE_NAME,
        CFG_PFC_PRIORITY_TO_PRIORITY_GROUP_MAP_TABLE_NAME,
        CFG_PFC_PRIORITY_TO_QUEUE_MAP_TABLE_NAME
    };

    QosOrchTest()
    {
        // FIXME: move out from constructor
        m_app_db = make_shared<swss::DBConnector>(APPL_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
        m_config_db = make_shared<swss::DBConnector>(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);
    }

    virtual ~QosOrchTest()
    {
        for (auto p : m_qos_map_list_pool) {
            free(p);
        }
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
        TestBase::SetUp();

        ASSERT_TRUE(0 == setenv("platform", "x86_64-barefoot_p4-r0", 1));
        gProfileMap.emplace("SAI_VS_SWITCH_TYPE", "SAI_VS_SWITCH_TYPE_BCM56850");
        gProfileMap.emplace("KV_DEVICE_MAC_ADDRESS", "20:03:04:05:06:00");

        qos_orch = new QosOrch(m_config_db.get(), qos_tables);
        sai_service_method_table_t test_services = {
            QosOrchTest::profile_get_value,
            QosOrchTest::profile_get_next_value
        };

        auto status = sai_api_initialize(0, (sai_service_method_table_t*)&test_services);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

#if WITH_SAI == LIBVS
        sai_switch_api = const_cast<sai_switch_api_t*>(&vs_switch_api);
        sai_qos_map_api = const_cast<sai_qos_map_api_t*>(&vs_qos_map_api);
        sai_port_api = const_cast<sai_port_api_t*>(&vs_port_api);
        sai_vlan_api = const_cast<sai_vlan_api_t*>(&vs_vlan_api);
        sai_bridge_api = const_cast<sai_bridge_api_t*>(&vs_bridge_api);
#endif

        sai_attribute_t swattr;

        swattr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
        swattr.value.booldata = true;

        status = sai_switch_api->create_switch(&gSwitchId, 1, &swattr);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);

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

        auto consumer = unique_ptr<Consumer>(new Consumer(
            new swss::ConsumerStateTable(m_app_db.get(), APP_PORT_TABLE_NAME, 1, 1), gPortsOrch, APP_PORT_TABLE_NAME));

        consumerAddToSync(consumer.get(), { { "PortInitDone", EMPTY_PREFIX, { { "", "" } } } });

        static_cast<Orch*>(gPortsOrch)->doTask(*consumer.get());
    }

    void TearDown() override
    {
        TestBase::TearDown();

        delete qos_orch;
        qos_orch = nullptr;
        auto status = sai_switch_api->remove_switch(gSwitchId);
        ASSERT_TRUE(status == SAI_STATUS_SUCCESS);
        gSwitchId = 0;

        delete gPortsOrch;
        gPortsOrch = nullptr;

        sai_api_uninitialize();

        sai_switch_api = nullptr;
        sai_qos_map_api = nullptr;
        sai_port_api = nullptr;
        sai_vlan_api = nullptr;
        sai_bridge_api = nullptr;
    }

    shared_ptr<MockQosOrch> createQosOrch()
    {
        return make_shared<MockQosOrch>(qos_orch, m_config_db.get());
    }

    shared_ptr<SaiAttributeList> getQosMapAttributeList(sai_object_type_t object_type, const string& qos_table_name, const vector<FieldValueTuple>& values)
    {
        vector<swss::FieldValueTuple> fields;

        if (qos_table_name == CFG_DSCP_TO_TC_MAP_TABLE_NAME) {
            fields.push_back({ "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_DSCP_TO_TC" });
            fields.push_back({ "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", convertValuesToQosMapListStr(qos_table_name, values) });
        } else if (qos_table_name == CFG_TC_TO_QUEUE_MAP_TABLE_NAME) {
            fields.push_back({ "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" });
            fields.push_back({ "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", convertValuesToQosMapListStr(qos_table_name, values) });
        }

        return shared_ptr<SaiAttributeList>(new SaiAttributeList(object_type, fields, false));
    }

    string convertValuesToQosMapListStr(const string& qos_table_name, const vector<FieldValueTuple>& values)
    {
        const sai_attr_metadata_t* meta;
        sai_attribute_t attr;
        attr.id = SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST;
        attr.value.qosmap.count = values.size();
        attr.value.qosmap.list = new sai_qos_map_t[attr.value.qosmap.count]();
        uint32_t ind = 0;

        if (qos_table_name == CFG_DSCP_TO_TC_MAP_TABLE_NAME) {
            for (auto i = values.begin(); i != values.end(); i++, ind++) {
                attr.value.qosmap.list[ind].key.dscp = (uint8_t)stoi(fvField(*i));
                attr.value.qosmap.list[ind].value.tc = (uint8_t)stoi(fvValue(*i));
            }
        } else if (qos_table_name == CFG_TC_TO_QUEUE_MAP_TABLE_NAME) {
            for (auto i = values.begin(); i != values.end(); i++, ind++) {
                attr.value.qosmap.list[ind].key.tc = (uint8_t)stoi(fvField(*i));
                attr.value.qosmap.list[ind].value.queue_index = (uint8_t)stoi(fvValue(*i));
            }
        }

        meta = sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_QOS_MAP, attr.id);

        return sai_serialize_attr_value(*meta, attr);
    }

    bool Validate(MockQosOrch* orch, const string& table_name, const vector<FieldValueTuple>& values)
    {
        const sai_object_type_t object_type = SAI_OBJECT_TYPE_QOS_MAP;

        auto qos_maps = orch->getTypeMap();
        auto qos_map = qos_maps.find(table_name);
        if (qos_map == qos_maps.end()) {
            return false;
        }

        auto obj_map = *(qos_map->second)->find(table_name);
        if (obj_map == *(qos_map->second)->end()) {
            return false;
        }

        auto exp_attr_list = getQosMapAttributeList(object_type, table_name, values);
        if (!ValidateQosMap(object_type, obj_map.second, *exp_attr_list.get())) {
            return false;
        }

        return true;
    }

    bool ValidateQosMap(sai_object_type_t object_type, sai_object_id_t object_id, SaiAttributeList& exp_attrlist)
    {
        vector<sai_attribute_t> act_attr;

        for (int i = 0; i < exp_attrlist.get_attr_count(); ++i) {
            const auto attr = exp_attrlist.get_attr_list()[i];
            auto meta = sai_metadata_get_attr_metadata(object_type, attr.id);

            if (meta == nullptr) {
                return false;
            }

            sai_attribute_t new_attr = { 0 };
            new_attr.id = attr.id;

            switch (meta->attrvaluetype) {
            case SAI_ATTR_VALUE_TYPE_INT32:
                new_attr.value.u32 = attr.value.u32;
                break;
            case SAI_ATTR_VALUE_TYPE_QOS_MAP_LIST:
                new_attr.value.qosmap.count = attr.value.qosmap.count;
                new_attr.value.qosmap.list = (sai_qos_map_t*)malloc(sizeof(sai_qos_map_t) * attr.value.qosmap.count);
                m_qos_map_list_pool.emplace_back(new_attr.value.qosmap.list);
                break;
            default:
                cout << "";
            }

            act_attr.emplace_back(new_attr);
        }

        auto status = sai_qos_map_api->get_qos_map_attribute(object_id, act_attr.size(), act_attr.data());
        if (status != SAI_STATUS_SUCCESS) {
            return false;
        }

        auto b_attr_eq = AttrListEq(object_type, act_attr, exp_attrlist);
        if (!b_attr_eq) {
            return false;
        }

        return true;
    }
};

map<string, string> QosOrchTest::gProfileMap;
map<string, string>::iterator QosOrchTest::gProfileIter = QosOrchTest::gProfileMap.begin();

TEST_F(QosOrchTest, Dscp_To_Tc_Map_Via_VS)
{
    auto orch = createQosOrch();

    vector<FieldValueTuple> dscp_to_tc_values = { { "0", "0" }, { "1", "0" }, { "2", "0" },
        { "3", "3" }, { "4", "4" }, { "5", "0" }, { "6", "0" }, { "7", "0" }, { "8", "1" },
        { "9", "0" }, { "10", "0" }, { "11", "0" }, { "12", "0" }, { "13", "0" }, { "14", "0" },
        { "15", "0" }, { "16", "0" }, { "17", "0" }, { "18", "0" }, { "19", "0" }, { "20", "0" },
        { "21", "0" }, { "22", "0" }, { "23", "0" }, { "24", "0" }, { "25", "0" }, { "26", "0" },
        { "27", "0" }, { "28", "0" }, { "29", "0" }, { "30", "0" }, { "31", "0" }, { "32", "0" },
        { "33", "0" }, { "34", "0" }, { "35", "0" }, { "36", "0" }, { "37", "0" }, { "38", "0" },
        { "39", "0" }, { "40", "0" }, { "41", "0" }, { "42", "0" }, { "43", "0" }, { "44", "0" },
        { "45", "0" }, { "46", "0" }, { "47", "0" }, { "48", "0" }, { "49", "0" }, { "50", "0" },
        { "51", "0" }, { "52", "0" }, { "53", "0" }, { "54", "0" }, { "55", "0" }, { "56", "0" },
        { "57", "0" }, { "58", "0" }, { "59", "0" }, { "60", "0" }, { "61", "0" }, { "62", "0" },
        { "63", "0" } };
    KeyOpFieldsValuesTuple dscp_to_tc_tuple(CFG_DSCP_TO_TC_MAP_TABLE_NAME, SET_COMMAND, dscp_to_tc_values);
    deque<KeyOpFieldsValuesTuple> setData = { dscp_to_tc_tuple };

    orch->doQosMapTask(setData, CFG_DSCP_TO_TC_MAP_TABLE_NAME);
    ASSERT_TRUE(Validate(orch.get(), CFG_DSCP_TO_TC_MAP_TABLE_NAME, dscp_to_tc_values));
}

TEST_F(QosOrchTest, Tc_To_Queue_Map_Via_VS)
{
    auto orch = createQosOrch();

    vector<FieldValueTuple> tc_to_queue_values = { { "0", "0" }, { "1", "1" }, { "3", "3" }, { "4", "4" } };
    KeyOpFieldsValuesTuple tc_to_queue_tuple(CFG_TC_TO_QUEUE_MAP_TABLE_NAME, SET_COMMAND, tc_to_queue_values);
    deque<KeyOpFieldsValuesTuple> setData = { tc_to_queue_tuple };

    orch->doQosMapTask(setData, CFG_TC_TO_QUEUE_MAP_TABLE_NAME);
    ASSERT_TRUE(Validate(orch.get(), CFG_TC_TO_QUEUE_MAP_TABLE_NAME, tc_to_queue_values));
}
}