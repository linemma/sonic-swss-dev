#include "gtest/gtest.h"

#include "consumerstatetable.h"
#include "hiredis.h"
#include "orchdaemon.h"
#include "sai_vs.h"
#include "saiattributelist.h"
#include "saihelper.h"

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

extern sai_qos_map_api_t* sai_qos_map_api;
extern sai_wred_api_t* sai_wred_api;

struct QosOrchMock : public QosOrch {
    QosOrchMock(swss::DBConnector* db, vector<string>& tableNames)
        : QosOrch(db, tableNames)
    {
    }

    task_process_status handleDscpToTcTable2(Consumer& consumer)
    {
        // SWSS_LOG_ENTER();
        DscpToTcMapHandler dscp_tc_handler;
        return dscp_tc_handler.processWorkItem(consumer);
    }
};

size_t consumerAddToSync(Consumer* consumer, std::deque<KeyOpFieldsValuesTuple>& entries)
{
    // SWSS_LOG_ENTER();

    /* Nothing popped */
    if (entries.empty()) {
        return 0;
    }

    for (auto& entry : entries) {
        string key = kfvKey(entry);
        string op = kfvOp(entry);

        // /* Record incoming tasks */
        // if (gSwssRecord)
        // {
        //     Orch::recordTuple(*this, entry);
        // }

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

struct SetQosResult {
    bool ret_val;

    std::vector<sai_attribute_t> attr_list;
};

struct TestBase : public ::testing::Test {
    static sai_status_t create_qos_map(sai_object_id_t* sai_object_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->create_qos_map_fn(sai_object_id, switch_id, attr_count,
            attr_list);
    }

    static sai_status_t create_wred(sai_object_id_t* sai_object_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->create_wred_fn(sai_object_id, switch_id, attr_count,
            attr_list);
    }

    static sai_status_t set_wred_attribute(sai_object_id_t* sai_object_id,
        const sai_attribute_t* attr_list)
    {
        return that->set_wred_attribute_fn(sai_object_id, attr_list);
    }

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        create_qos_map_fn;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        create_wred_fn;

    std::function<sai_status_t(sai_object_id_t*, const sai_attribute_t*)>
        set_wred_attribute_fn;

    std::shared_ptr<SetQosResult> setDscp2Tc(DscpToTcMapHandler& dscpToTc, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        dscpToTc.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = dscpToTc.addQosItem(attrs);
        return ret;
    }

    std::shared_ptr<SetQosResult> setDscp2Tc2(QosOrchMock& qosorch, Consumer& consumer)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        ret->ret_val = qosorch.handleDscpToTcTable2(consumer);
        return ret;
    }

    std::shared_ptr<SetQosResult> setTc2Queue(TcToQueueMapHandler& tcToQueue, KeyOpFieldsValuesTuple& tuple)
    {
        // assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        tcToQueue.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = tcToQueue.addQosItem(attrs);
        return ret;
    }

    std::shared_ptr<SetQosResult> setTc2Pg(TcToPgHandler& tcToPg, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        tcToPg.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = tcToPg.addQosItem(attrs);
        return ret;
    }

    std::shared_ptr<SetQosResult> setPfcPrio2Pg(PfcPrioToPgHandler& pfcPrioToPg, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        pfcPrioToPg.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = pfcPrioToPg.addQosItem(attrs);
        return ret;
    }

    std::shared_ptr<SetQosResult> setPfc2Queue(PfcToQueueHandler& pfcToQueue, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_qos_map_api->create_qos_map = create_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_qos_map_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        pfcToQueue.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = pfcToQueue.addQosItem(attrs);
        return ret;
    }

    std::shared_ptr<SetQosResult> addWredfile(WredMapHandler& wredMap, KeyOpFieldsValuesTuple& tuple)
    {
        assert(sai_wred_api == nullptr);

        sai_wred_api = new sai_wred_api_t();
        auto sai_qos = std::shared_ptr<sai_wred_api_t>(sai_wred_api, [](sai_wred_api_t* p) {
            delete p;
            sai_wred_api = nullptr;
        });

        // FIXME: add new function to setup spy function
        sai_wred_api->create_wred = create_wred;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        create_wred_fn =
            [&](sai_object_id_t* sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t* attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i) {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        vector<sai_attribute_t> attrs;
        wredMap.convertFieldValuesToAttributes(tuple, attrs);
        ret->ret_val = wredMap.addQosItem(attrs);
        return ret;
    }

    bool AttrListEq(const std::vector<sai_attribute_t>& act_attr_list, SaiAttributeList& exp_attr_list)
    {
        if (act_attr_list.size() != exp_attr_list.get_attr_count()) {
            return false;
        }

        auto l = exp_attr_list.get_attr_list();
        for (int i = 0; i < exp_attr_list.get_attr_count(); ++i) {
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
                return false;
            }
        }
        return true;
    }
};

TestBase* TestBase::that = nullptr;

struct QosMapHandlerTest : public TestBase {
    void SetUp() override
    {
        // ...
        ASSERT_TRUE(0 == setenv("platform", "x86_64-barefoot_p4-r0", 1));
    }
};

TEST_F(QosMapHandlerTest, DscpToTcMap)
{
    DscpToTcMapHandler dscpToTcMapHandler;
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("DSCP_TO_TC_MAP", "SET", { { "1", "0" }, { "2", "0" }, { "3", "3" } });

    auto res = setDscp2Tc(dscpToTcMapHandler, dscp_to_tc_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_DSCP_TO_TC" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":1,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":2,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":3,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, DscpToTcMap2)
{
    auto configDb = swss::DBConnector(CONFIG_DB, swss::DBConnector::DEFAULT_UNIXSOCKET, 0);

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
    auto qosorch = QosOrchMock(&configDb, qos_tables);

    auto consumer = std::unique_ptr<Consumer>(new Consumer(new swss::ConsumerStateTable(&configDb, std::string(CFG_DSCP_TO_TC_MAP_TABLE_NAME), 1, 1), &qosorch, std::string(CFG_DSCP_TO_TC_MAP_TABLE_NAME)));

    KeyOpFieldsValuesTuple dscp_to_tc_tuple("DSCP_TO_TC_MAP", "SET", { { "1", "0" }, { "2", "0" }, { "3", "3" } });
    std::deque<KeyOpFieldsValuesTuple> setData = { dscp_to_tc_tuple };

    consumerAddToSync(consumer.get(), setData);
    auto res = setDscp2Tc2(qosorch, *consumer);

    ASSERT_TRUE(res->ret_val == true);

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_DSCP_TO_TC" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":1,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":2,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":3,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, DscpToTcMap3)
{
    DscpToTcMapHandler dscpToTcMapHandler;

    auto v = std::vector<swss::FieldValueTuple>({ { "1", "0" },
        { "2", "0" },
        { "3", "3" } });
    // SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    // FIXME: add attr_list to dscp_to_tc_tuple
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("DSCP_TO_TC_MAP", "SET", v);
    vector<sai_attribute_t> exp_dscp_to_tc;
    dscpToTcMapHandler.convertFieldValuesToAttributes(dscp_to_tc_tuple, exp_dscp_to_tc);

    // auto res = setDscp2Tc(dscpToTcMapHandler, exp_dscp_to_tc);
    sai_qos_map_api = const_cast<sai_qos_map_api_t*>(&vs_qos_map_api);
    bool ret_val = dscpToTcMapHandler.addQosItem(exp_dscp_to_tc);

    // FIXME: should check SAI_QOS_MAP_TYPE_DSCP_TO_TC

    // ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    // ASSERT_TRUE(AttrListEq(res->attr_list, exp_dscp_to_tc));
}

TEST_F(QosMapHandlerTest, TcToQueueMap)
{
    TcToQueueMapHandler tcToQueueMapHandler;
    KeyOpFieldsValuesTuple tc_to_queue_tuple("TC_TO_QUEUE_MAP", "SET", { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setTc2Queue(tcToQueueMapHandler, tc_to_queue_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":1},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":1,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, TcToPgMap)
{
    TcToPgHandler tcToPgHandler;
    KeyOpFieldsValuesTuple tc_to_pg_tuple("TC_TO_PRIORITY_GROUP_MAP", "SET", { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setTc2Pg(tcToPgHandler, tc_to_pg_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_TC_TO_PRIORITY_GROUP" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":1},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":1,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":3},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":3,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, PfcPrioToPgMap)
{
    PfcPrioToPgHandler pfcPrioToPgHandler;
    KeyOpFieldsValuesTuple pfc_prio_to_pg_tuple("PFC_PRIORITY_TO_PRIORITY_GROUP_MAP", "SET", { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setPfcPrio2Pg(pfcPrioToPgHandler, pfc_prio_to_pg_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":1,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":1,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":3,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":3,\"prio\":0,\"qidx\":0,\"tc\":0}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, PfcToQueueMap)
{
    PfcToQueueHandler pfcToQueueHandler;
    KeyOpFieldsValuesTuple pfc_to_queue_tuple("MAP_PFC_PRIORITY_TO_QUEUE", "SET", { { "0", "0" }, { "1", "1" }, { "3", "3" } });

    auto res = setPfc2Queue(pfcToQueueHandler, pfc_to_queue_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_QOS_MAP_ATTR_TYPE", "SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_QUEUE" },
        { "SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST", "{\"count\":3,\"list\":[{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":0,\"tc\":0}},{\
         \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":1,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":1,\"tc\":0}},{\
        \"key\":{\"color\":\"SAI_PACKET_COLOR_RED\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":3,\"qidx\":0,\"tc\":0},\
        \"value\":{\"color\":\"SAI_PACKET_COLOR_GREEN\",\"dot1p\":0,\"dscp\":0,\"pg\":0,\"prio\":0,\"qidx\":3,\"tc\":0}}]}" } });
    SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

TEST_F(QosMapHandlerTest, addWredProfile)
{
    WredMapHandler wredMapHandler;
    KeyOpFieldsValuesTuple wred_profile_tuple("WRED_PROFILE", "SET",
        { { "wred_yellow_enable", "true" },
            { "wred_red_enable", "true" },
            { "ecn", "ecn_all" },
            { "red_max_threshold", "312000" },
            { "red_min_threshold", "104000" },
            { "yellow_max_threshold", "312000" },
            { "yellow_min_threshold", "104000" },
            { "green_max_threshold", "312000" },
            { "green_min_threshold", "104000" } });

    auto res = addWredfile(wredMapHandler, wred_profile_tuple);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    auto v = std::vector<swss::FieldValueTuple>({ { "SAI_WRED_ATTR_WEIGHT", "0" },
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

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}

struct QosOrchTest : public TestBase {
};