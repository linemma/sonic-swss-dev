#include "gtest/gtest.h"

#include "hiredis.h"
#include "orchdaemon.h"
#include "saihelper.h"
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

extern sai_qos_map_api_t *sai_qos_map_api;

struct CreateQosResult
{
    bool ret_val;

    std::vector<sai_attribute_t> attr_list;
};

struct TestBase : public ::testing::Test
{
    static sai_status_t sai_create_qos_map(sai_object_id_t *sai_object_id,
                                           sai_object_id_t switch_id,
                                           uint32_t attr_count,
                                           const sai_attribute_t *attr_list)
    {
        return that->sai_create_qos_map_fn(sai_object_id, switch_id, attr_count,
                                           attr_list);
    }

    static TestBase *that;

    std::function<sai_status_t(sai_object_id_t *, sai_object_id_t, uint32_t,
                               const sai_attribute_t *)>
        sai_create_qos_map_fn;

    std::shared_ptr<CreateQosResult> createDscpToTcMap(DscpToTcMapHandler &dscpToTc)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t *p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        sai_qos_map_api->create_qos_map = sai_create_qos_map;
        that = this;

        auto ret = std::make_shared<CreateQosResult>();

        sai_create_qos_map_fn =
            [&](sai_object_id_t *sai_object_id, sai_object_id_t switch_id,
                uint32_t attr_count,
                const sai_attribute_t *attr_list) -> sai_status_t {
            for (auto i = 0; i < attr_count; ++i)
            {
                ret->attr_list.emplace_back(attr_list[i]);
            }
            return SAI_STATUS_FAILURE;
        };

        // set attribute tuple
        KeyOpFieldsValuesTuple dscp_to_tc_tuple("dscpToTc", "addDscoToTc", {{"1", "0"}, {"2", "0"}, {"3", "3"}});
        vector<sai_attribute_t> dscp_to_tc_attributes;

        dscpToTc.convertFieldValuesToAttributes(dscp_to_tc_tuple, dscp_to_tc_attributes);
        ret->ret_val = dscpToTc.addQosItem(dscp_to_tc_attributes);
        return ret;
    }

    bool AttrListEq(const std::vector<sai_attribute_t> &act_attr_list, vector<sai_attribute_t> &exp_attr_list)
    {
        if (act_attr_list.size() != exp_attr_list.size())
        {
            return false;
        }

        auto l = exp_attr_list;
        for (int i = 0; i < exp_attr_list.size(); i++)
        {
            auto found = std::find_if(act_attr_list.begin(), act_attr_list.end(), [&](const sai_attribute_t &attr) {
                if (attr.id != l[i].id)
                {
                    return false;
                }

                switch (attr.id)
                {
                case SAI_QOS_MAP_ATTR_TYPE:
                    if (attr.value.u32 != l[i].value.u32)
                    {
                        return false;
                    }
                    break;
                case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST:
                    if (attr.value.qosmap.count == l[i].value.qosmap.count)
                    {
                        for (int j = 0; j < attr.value.qosmap.count; j++)
                        {
                            if (attr.value.qosmap.list[j].key.dscp != l[i].value.qosmap.list[j].key.dscp)
                            {
                                return false;
                            }
                            if (attr.value.qosmap.list[j].value.tc != l[i].value.qosmap.list[j].value.tc)
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        return false;
                    }
                    break;
                }
                return true;
            });

            if (found == act_attr_list.end())
            {
                return false;
            }
        }
        return true;
    }
};

TestBase *TestBase::that = nullptr;

struct DscpToTcTest : public TestBase
{

    DscpToTcTest()
    {
    }
};

TEST_F(DscpToTcTest, check_dscp_to_tc_attrs)
{
    DscpToTcMapHandler dscpToTcMapHandler;

    auto res = createDscpToTcMap(dscpToTcMapHandler);

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    // set expected data
    vector<sai_attribute_t> attr_list;
    sai_attribute_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_QOS_MAP_ATTR_TYPE;
    attr.value.u32 = SAI_QOS_MAP_TYPE_DSCP_TO_TC;
    attr_list.push_back(attr);

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST;
    attr.value.qosmap.count = 3;
    attr.value.qosmap.list = new sai_qos_map_t[attr.value.qosmap.count]();
    attr.value.qosmap.list[0].key.dscp = 1;
    attr.value.qosmap.list[0].value.tc = 0;
    attr.value.qosmap.list[1].key.dscp = 2;
    attr.value.qosmap.list[1].value.tc = 0;
    attr.value.qosmap.list[2].key.dscp = 3;
    attr.value.qosmap.list[2].value.tc = 3;
    attr_list.push_back(attr);

    ASSERT_TRUE(AttrListEq(res->attr_list, attr_list));
}
