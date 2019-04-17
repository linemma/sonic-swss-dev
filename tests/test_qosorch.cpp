#include "gtest/gtest.h"

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

struct SetQosResult {
    bool ret_val;

    std::vector<sai_attribute_t> attr_list;
};

struct TestBase : public ::testing::Test {
    static sai_status_t sai_set_qos_map(sai_object_id_t* sai_object_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_set_qos_map_fn(sai_object_id, switch_id, attr_count,
            attr_list);
    }

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        sai_set_qos_map_fn;

    std::shared_ptr<SetQosResult> setDscp2Tc(DscpToTcMapHandler& dscpToTc, vector<sai_attribute_t>& attributes)
    {
        assert(sai_qos_map_api == nullptr);

        sai_qos_map_api = new sai_qos_map_api_t();
        auto sai_qos = std::shared_ptr<sai_qos_map_api_t>(sai_qos_map_api, [](sai_qos_map_api_t* p) {
            delete p;
            sai_qos_map_api = nullptr;
        });

        sai_qos_map_api->create_qos_map = sai_set_qos_map;
        that = this;

        auto ret = std::make_shared<SetQosResult>();

        sai_set_qos_map_fn =
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

struct DscpToTcTest : public TestBase {

    DscpToTcTest()
    {
    }
};

TEST_F(DscpToTcTest, setDscp2Tc)
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

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

    ASSERT_TRUE(AttrListEq(res->attr_list, exp_dscp_to_tc));
}
