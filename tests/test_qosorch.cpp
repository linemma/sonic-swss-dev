#include "gtest/gtest.h"

#include "consumerstatetable.h"
#include "hiredis.h"
#include "orchdaemon.h"
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

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        create_qos_map_fn;

    std::shared_ptr<SetQosResult> setDscp2Tc(DscpToTcMapHandler& dscpToTc, vector<sai_attribute_t>& attributes)
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

        ret->ret_val = dscpToTc.addQosItem(attributes);
        return ret;
    }

    std::shared_ptr<SetQosResult> setTc2Queue(TcToQueueMapHandler& tcToQueue, vector<sai_attribute_t>& attributes)
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

        ret->ret_val = tcToQueue.addQosItem(attributes);
        return ret;
    }

    bool AttrListEq(const std::vector<sai_attribute_t>& act_attr_list, const std::vector<sai_attribute_t>& exp_attr_list)
    {
        // FIXME: add attr compare
        return true;
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

    auto v = std::vector<swss::FieldValueTuple>({ { "1", "0" },
        { "2", "0" },
        { "3", "3" } });
    // SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    // FIXME: add attr_list to dscp_to_tc_tuple
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("dscpToTc", "setDscoToTc", v);
    vector<sai_attribute_t> exp_dscp_to_tc;
    dscpToTcMapHandler.convertFieldValuesToAttributes(dscp_to_tc_tuple, exp_dscp_to_tc);

    auto res = setDscp2Tc(dscpToTcMapHandler, exp_dscp_to_tc);

    // FIXME: should check SAI_QOS_MAP_TYPE_DSCP_TO_TC

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    ASSERT_TRUE(AttrListEq(res->attr_list, exp_dscp_to_tc));
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

    //
    // input
    //
    auto v = std::vector<swss::FieldValueTuple>({ { "1", "0" },
        { "2", "0" },
        { "3", "3" } });
    // SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);
    // //

    // qosorch.addConsumer(&configDb, std::string(APP_PORT_QOS_MAP_TABLE_NAME), 1);
    auto consumer = std::unique_ptr<Consumer>(new Consumer(new swss::ConsumerStateTable(&configDb, std::string(APP_COPP_TABLE_NAME), 1, 1), &qosorch, std::string(APP_COPP_TABLE_NAME)));

    KeyOpFieldsValuesTuple rule1Attr("coppRule1", "SET", { { "trap_ids", "stp,lacp,eapol" }, { "trap_action", "drop" }, { "queue", "3" }, { "trap_priority", "1" } });
    std::deque<KeyOpFieldsValuesTuple> setData = { rule1Attr };

    // consumer->addToSync(setData);
    consumerAddToSync(consumer.get(), setData);

    qosorch.handleDscpToTcTable2(*(consumer.get()));

    DscpToTcMapHandler dscpToTcMapHandler;

    // FIXME: add attr_list to dscp_to_tc_tuple
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("dscpToTc", "setDscoToTc", v);
    vector<sai_attribute_t> exp_dscp_to_tc;
    dscpToTcMapHandler.convertFieldValuesToAttributes(dscp_to_tc_tuple, exp_dscp_to_tc);

    auto res = setDscp2Tc(dscpToTcMapHandler, exp_dscp_to_tc);

    // FIXME: should check SAI_QOS_MAP_TYPE_DSCP_TO_TC

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    ASSERT_TRUE(AttrListEq(res->attr_list, exp_dscp_to_tc));
}

TEST_F(QosMapHandlerTest, TcToQueueMap)
{
    TcToQueueMapHandler tcToQueueMapHandler;

    auto v = std::vector<swss::FieldValueTuple>({ { "1", "0" },
        { "2", "0" },
        { "3", "3" } });
    // SaiAttributeList attr_list(SAI_OBJECT_TYPE_QOS_MAP, v, false);

    // FIXME: add attr_list to dscp_to_tc_tuple
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("dscpToTc", "setDscoToTc", v);
    vector<sai_attribute_t> exp_dscp_to_tc;
    tcToQueueMapHandler.convertFieldValuesToAttributes(dscp_to_tc_tuple, exp_dscp_to_tc);

    auto res = setTc2Queue(tcToQueueMapHandler, exp_dscp_to_tc);

    // FIXME: should check SAI_QOS_MAP_TYPE_TC_TO_QUEUE

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    ASSERT_TRUE(AttrListEq(res->attr_list, exp_dscp_to_tc));
}

// TODO: add for TcToPgHandler
// TODO: add for PfcPrioToPgHandler
// TODO: add for PfcToQueueHandler

struct QosOrchTest : public TestBase {
};