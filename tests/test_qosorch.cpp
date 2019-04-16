#include "gtest/gtest.h"

#include "hiredis.h"
#include "orchdaemon.h"
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

uint32_t dscp_to_tc_attr_count;
uint32_t tc_to_queue_attr_count;
sai_attribute_t dscp_to_tc_attr_list[10];

extern sai_qos_map_api_t* sai_qos_map_api;

struct DscpToTcTest : public ::testing::Test {

    DscpToTcTest()
    {
        dscp_to_tc_attr_count = 0;
        memset(dscp_to_tc_attr_list, 0, sizeof(dscp_to_tc_attr_list));
    }
    ~DscpToTcTest()
    {
    }
};

int fake_create_dscp_to_tc_map(sai_object_id_t* sai_object,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list)
{
    dscp_to_tc_attr_count = attr_count;
    memcpy(dscp_to_tc_attr_list, attr_list, sizeof(sai_attribute_t) * attr_count);
    return SAI_STATUS_FAILURE;
}

bool equal(uint8_t value1, uint8_t value2)
{
    return value1 == value2 ? true : false;
}

bool check_dscp_to_tc_value(vector<sai_attribute_t>& expected_attr, sai_attribute_t* verify_attr, uint32_t count)
{
    if (expected_attr.size() != count) {
        return false;
    }

    for (auto it : expected_attr) {
        for (int i = 0; i < count; i++) {
            sai_attribute_t attr_obj = verify_attr[i];
            if (it.id == attr_obj.id) {
                switch (it.id) {
                case SAI_QOS_MAP_ATTR_TYPE:
                    if (it.value.u32 != attr_obj.value.u32) {
                        return false;
                    }
                    break;
                case SAI_QOS_MAP_ATTR_MAP_TO_VALUE_LIST:
                    if (it.value.qosmap.count == attr_obj.value.qosmap.count) {
                        for (int j = 0; j < it.value.qosmap.count; j++) {
                            if (!equal(it.value.qosmap.list[j].key.dscp, attr_obj.value.qosmap.list[j].key.dscp)) {
                                return false;
                            }
                            if (!equal(it.value.qosmap.list[j].value.tc, attr_obj.value.qosmap.list[j].value.tc)) {
                                return false;
                            }
                        }
                    } else {
                        return false;
                    }
                    break;
                default:
                    return false;
                }
            }
        }
    }
    return true;
}

TEST_F(DscpToTcTest, check_dscp_to_tc_attrs)
{
    sai_qos_map_api = new sai_qos_map_api_t();

    sai_qos_map_api->create_qos_map = fake_create_dscp_to_tc_map;

    // set attribute tuple
    KeyOpFieldsValuesTuple dscp_to_tc_tuple("dscpToTc", "addDscoToTc", { { "1", "0" }, { "2", "0" }, { "3", "3" } });
    vector<sai_attribute_t> dscp_to_tc_attributes;
    DscpToTcMapHandler dscpToTcMapHandler;

    // call dscp to tc functions
    dscpToTcMapHandler.convertFieldValuesToAttributes(dscp_to_tc_tuple, dscp_to_tc_attributes);
    dscpToTcMapHandler.addQosItem(dscp_to_tc_attributes);

    // set expected data
    vector<sai_attribute_t> expected_attr_list;
    sai_attribute_t attr;

    memset(&attr, 0, sizeof(attr));
    attr.id = SAI_QOS_MAP_ATTR_TYPE;
    attr.value.u32 = SAI_QOS_MAP_TYPE_DSCP_TO_TC;
    expected_attr_list.push_back(attr);

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
    expected_attr_list.push_back(attr);

    // validate
    ASSERT_EQ(check_dscp_to_tc_value(expected_attr_list, dscp_to_tc_attr_list, dscp_to_tc_attr_count), true);

    // teardown
    sai_qos_map_api->create_qos_map = NULL;
    delete sai_qos_map_api;
}
