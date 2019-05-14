#pragma once

#include "syncd.h"

/**
 * @brief Global mutex for thread synchronization
 *
 * Purpose of this mutex is to synchronize multiple threads like main thread,
 * counters and notifications as well as all operations which require multiple
 * Redis DB access.
 *
 * For example: query DB for next VID id number, and then put map RID and VID
 * to Redis. From syncd point of view this entire operation should be atomic
 * and no other thread should access DB or make assumption on previous
 * information until entire operation will finish.
 */
extern std::mutex syncd_g_mutex;

extern std::shared_ptr<swss::RedisClient> syncd_g_redisClient;
//extern std::shared_ptr<swss::ProducerTable> syncd_getResponse;
extern std::shared_ptr<swss::NotificationProducer> syncd_notifications;

/*
 * TODO: Those are hard coded values for mlnx integration for v1.0.1 they need
 * to be updated.
 *
 * Also DEVICE_MAC_ADDRESS is not present in saiswitch.h
 */
extern std::map<std::string, std::string> syncd_gProfileMap;

/**
 * @brief Contains map of all created switches.
 *
 * This syncd implementation supports only one switch but it's written in
 * a way that could be extended to use multiple switches in the future, some
 * refactoring needs to be made in marked places.
 *
 * To support multiple switches VIDTORID and RIDTOVID db entries needs to be
 * made per switch like HIDDEN and LANES. Best way is to wrap vid/rid map to
 * functions that will return right key.
 *
 * Key is switch VID.
 */
extern std::map<sai_object_id_t, std::shared_ptr<SaiSwitch>> syncd_switches;

/**
 * @brief set of objects removed by user when we are in init view mode. Those
 * could be vlan members, bridge ports etc.
 *
 * We need this list to later on not put them back to temp view mode when doing
 * populate existing objects in apply view mode.
 *
 * Object ids here a VIDs.
 */
//extern std::set<sai_object_id_t> syncd_initViewRemovedVidSet;

/*
 * When set to true will enable DB vs ASIC consistency check after comparison
 * logic.
 */
extern bool syncd_g_enableConsistencyCheck;

/*
 * By default we are in APPLY mode.
 */
extern volatile bool syncd_g_asicInitViewMode;

/*
 * SAI switch global needed for RPC server and for remove_switch
 */
extern sai_object_id_t syncd_gSwitchId;

#define g_mutex syncd_g_mutex

#define g_redisClient syncd_g_redisClient
#define getResponse syncd_getResponse
#define notifications syncd_notifications

#define gProfileMap syncd_gProfileMap

#define switches syncd_switches

#define initViewRemovedVidSet syncd_initViewRemovedVidSet

#define g_enableConsistencyCheck syncd_g_enableConsistencyCheck

#define g_asicInitViewMode syncd_g_asicInitViewMode

#define gSwitchId syncd_gSwitchId

#define test_services syncd_test_services

