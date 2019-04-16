#include "gtest/gtest.h"

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
extern sai_acl_api_t* sai_acl_api;
extern sai_switch_api_t* sai_switch_api;

int fake_create_acl_table(sai_object_id_t* acl_table_id,
    sai_object_id_t switch_id, uint32_t attr_count,
    const sai_attribute_t* attr_list);

TEST(foo, foo)
{
    // auto v = std::vector<swss::FieldValueTuple>({ { "SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP", "true" },
    // { "SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS", "true" } });

    // attr.id = SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST;
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE;   //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE;  //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL;  //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_IP;       //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_IP;       //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT;  //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT;  //
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS;
    // attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE;
    // attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
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

struct TestBase : public ::testing::Test {
    static sai_status_t sai_create_acl_table_(sai_object_id_t* acl_table_id,
        sai_object_id_t switch_id,
        uint32_t attr_count,
        const sai_attribute_t* attr_list)
    {
        return that->sai_create_acl_table_fn(acl_table_id, switch_id, attr_count,
            attr_list);
    }

    static TestBase* that;

    std::function<sai_status_t(sai_object_id_t*, sai_object_id_t, uint32_t,
        const sai_attribute_t*)>
        sai_create_acl_table_fn;

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
            return SAI_STATUS_FAILURE;
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

    AclTest()
    {
    }

    void SetUp() override
    {
        set_attr_count = 0;
        memset(set_attr_list, 0, sizeof(set_attr_list));
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
    return SAI_STATUS_FAILURE;
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

    ASSERT_TRUE(res->ret_val == false); // FIXME: should be true

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
