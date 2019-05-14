#define sai_api_initialize sai_redis_sai_api_initialize
#define sai_api_query sai_redis_sai_api_query
#define sai_api_uninitialize sai_redis_sai_api_uninitialize
#define sai_log_set sai_redis_sai_log_set
#define sai_object_type_query sai_redis_sai_object_type_query
#define sai_switch_id_query sai_redis_sai_switch_id_query

#define g_redisClient sai_redis_g_redisClient
#define g_asicInitViewMode sai_redis_g_asicInitViewMode

#define meta_generic_validation_post_create sai_redis_meta_generic_validation_post_create
#define meta_sai_get_oid sai_redis_meta_sai_get_oid
#define meta_sai_validate_oid sai_redis_meta_sai_validate_oid

#include "lib/src/sai_redis_acl.cpp"
#include "lib/src/sai_redis_bfd.cpp"
#include "lib/src/sai_redis_bmtor.cpp"
#include "lib/src/sai_redis_bridge.cpp"
#include "lib/src/sai_redis_buffer.cpp"
#include "lib/src/sai_redis_dtel.cpp"
#include "lib/src/sai_redis_fdb.cpp"
#include "lib/src/sai_redis_hash.cpp"
#include "lib/src/sai_redis_hostintf.cpp"
#include "lib/src/sai_redis_interfacequery.cpp"
#include "lib/src/sai_redis_ipmc.cpp"
#include "lib/src/sai_redis_ipmc_group.cpp"
#include "lib/src/sai_redis_l2mc.cpp"
#include "lib/src/sai_redis_l2mcgroup.cpp"
#include "lib/src/sai_redis_lag.cpp"
#include "lib/src/sai_redis_mcastfdb.cpp"
#include "lib/src/sai_redis_mirror.cpp"
#include "lib/src/sai_redis_mpls.cpp"
#include "lib/src/sai_redis_neighbor.cpp"
#include "lib/src/sai_redis_nexthop.cpp"
#include "lib/src/sai_redis_nexthopgroup.cpp"
#include "lib/src/sai_redis_policer.cpp"
#include "lib/src/sai_redis_port.cpp"
#include "lib/src/sai_redis_qosmaps.cpp"
#include "lib/src/sai_redis_queue.cpp"
#include "lib/src/sai_redis_route.cpp"
#include "lib/src/sai_redis_router_interface.cpp"
#include "lib/src/sai_redis_rpfgroup.cpp"
#include "lib/src/sai_redis_samplepacket.cpp"
#include "lib/src/sai_redis_scheduler.cpp"
#include "lib/src/sai_redis_schedulergroup.cpp"
#include "lib/src/sai_redis_segmentroute.cpp"
#include "lib/src/sai_redis_stp.cpp"
#include "lib/src/sai_redis_switch.cpp"
#include "lib/src/sai_redis_tam.cpp"
#include "lib/src/sai_redis_tunnel.cpp"
#include "lib/src/sai_redis_uburst.cpp"
#include "lib/src/sai_redis_udf.cpp"
#include "lib/src/sai_redis_virtual_router.cpp"
#include "lib/src/sai_redis_vlan.cpp"
#include "lib/src/sai_redis_wred.cpp"

#define redis_construct_object_id sai_redis_construct_object_id
#define redis_get_switch_id_index sai_redis_get_switch_id_index

#include "lib/src/sai_redis_generic_create.cpp"
#include "lib/src/sai_redis_generic_remove.cpp"
#include "lib/src/sai_redis_generic_set.cpp"
#include "lib/src/sai_redis_generic_get.cpp"
#include "lib/src/sai_redis_generic_stats.cpp"

#define handle_switch_state_change sai_redis_handle_switch_state_change
#define handle_fdb_event sai_redis_handle_fdb_event
#define handle_port_state_change sai_redis_handle_port_state_change
#define handle_switch_shutdown_request sai_redis_handle_switch_shutdown_request

#include "lib/src/sai_redis_notifications.cpp"
#include "lib/src/sai_redis_record.cpp"

#include "meta/sai_meta.cpp"
#include "meta/saiattributelist.cpp"
#include "meta/saiserialize.cpp"