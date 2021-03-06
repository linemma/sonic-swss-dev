#define sai_api_initialize sai_vs_sai_api_initialize
#define sai_api_query sai_vs_sai_api_query
#define sai_api_uninitialize sai_vs_sai_api_uninitialize
#define sai_log_set sai_vs_sai_log_set
#define sai_object_type_query sai_vs_sai_object_type_query
#define sai_switch_id_query sai_vs_sai_switch_id_query

#define g_redisClient sai_vs_g_redisClient
#define g_asicInitViewMode sai_vs_g_asicInitViewMode

// sai_vs_interfacequery.cpp
#define g_api_initialized sai_vs_g_api_initialized
#define g_vs_hostif_use_tap_device sai_vs_g_vs_hostif_use_tap_device
#define g_vs_switch_type sai_vs_g_vs_switch_type
#define g_recursive_mutex sai_vs_g_recursive_mutex
#define g_unittestChannelRun sai_vs_g_unittestChannelRun
#define g_unittestChannelThreadEvent sai_vs_g_unittestChannelThreadEvent
#define g_unittestChannelThread sai_vs_g_unittestChannelThread
#define g_unittestChannelNotificationConsumer sai_vs_g_unittestChannelNotificationConsumer
#define g_dbNtf sai_vs_g_dbNtf
#define g_fdbAgingThreadRun sai_vs_g_fdbAgingThreadRun
#define g_fdbAgingThread sai_vs_g_fdbAgingThread
#define g_vs_boot_type sai_vs_g_vs_boot_type
#define g_lane_to_ifname sai_vs_g_lane_to_ifname
#define g_ifname_to_lanes sai_vs_g_ifname_to_lanes
#define g_lane_order sai_vs_g_lane_order
#define g_laneMap sai_vs_g_laneMap
#define g_boot_type sai_vs_g_boot_type
#define g_warm_boot_read_file sai_vs_g_warm_boot_read_file
#define g_warm_boot_write_file sai_vs_g_warm_boot_write_file
#define g_interface_lane_map_file sai_vs_g_interface_lane_map_file 

#define clear_local_state sai_vs_clear_local_state

#define meta_generic_validation_post_create sai_vs_meta_generic_validation_post_create
#define meta_sai_get_oid sai_vs_meta_sai_get_oid
#define meta_sai_validate_oid sai_vs_meta_sai_validate_oid

#define meta_sai_create_oid sai_vs_meta_sai_create_oid
#define meta_sai_remove_oid sai_vs_meta_sai_remove_oid
#define meta_sai_set_oid sai_vs_meta_sai_set_oid
#define meta_sai_on_fdb_event sai_vs_meta_sai_on_fdb_event
#define meta_sai_flush_fdb_entries sai_vs_meta_sai_flush_fdb_entries
#define meta_sai_create_fdb_entry sai_vs_meta_sai_create_fdb_entry
#define meta_sai_remove_fdb_entry sai_vs_meta_sai_remove_fdb_entry
#define meta_sai_set_fdb_entry sai_vs_meta_sai_set_fdb_entry
#define meta_sai_get_fdb_entry sai_vs_meta_sai_get_fdb_entry
#define meta_unittests_enable sai_vs_meta_unittests_enable
#define meta_unittests_allow_readonly_set_once sai_vs_meta_unittests_allow_readonly_set_once
#define meta_init_db sai_vs_meta_init_db
#define meta_sai_create_ipmc_entry sai_vs_meta_sai_create_ipmc_entry
#define meta_sai_remove_ipmc_entry sai_vs_meta_sai_remove_ipmc_entry
#define meta_sai_set_ipmc_entry sai_vs_meta_sai_set_ipmc_entry
#define meta_sai_get_ipmc_entry sai_vs_meta_sai_get_ipmc_entry
#define meta_sai_create_l2mc_entry sai_vs_meta_sai_create_l2mc_entry
#define meta_sai_remove_l2mc_entry sai_vs_meta_sai_remove_l2mc_entry
#define meta_sai_set_l2mc_entry sai_vs_meta_sai_set_l2mc_entry
#define meta_sai_get_l2mc_entry sai_vs_meta_sai_get_l2mc_entry
#define meta_sai_create_mcast_fdb_entry sai_vs_meta_sai_create_mcast_fdb_entry
#define meta_sai_remove_mcast_fdb_entry sai_vs_meta_sai_remove_mcast_fdb_entry
#define meta_sai_set_mcast_fdb_entry sai_vs_meta_sai_set_mcast_fdb_entry
#define meta_sai_get_mcast_fdb_entry sai_vs_meta_sai_get_mcast_fdb_entry
#define meta_sai_create_inseg_entry sai_vs_meta_sai_create_inseg_entry
#define meta_sai_remove_inseg_entry sai_vs_meta_sai_remove_inseg_entry
#define meta_sai_set_inseg_entry sai_vs_meta_sai_set_inseg_entry
#define meta_sai_get_inseg_entry sai_vs_meta_sai_get_inseg_entry
#define meta_sai_create_neighbor_entry sai_vs_meta_sai_create_neighbor_entry
#define meta_sai_remove_neighbor_entry sai_vs_meta_sai_remove_neighbor_entry
#define meta_sai_set_neighbor_entry sai_vs_meta_sai_set_neighbor_entry
#define meta_sai_get_neighbor_entry sai_vs_meta_sai_get_neighbor_entry
#define meta_sai_create_route_entry sai_vs_meta_sai_create_route_entry
#define meta_sai_remove_route_entry sai_vs_meta_sai_remove_route_entry
#define meta_sai_set_route_entry sai_vs_meta_sai_set_route_entry
#define meta_sai_get_route_entry sai_vs_meta_sai_get_route_entry
#define meta_generic_validation_post_set sai_vs_meta_generic_validation_post_set
#define meta_warm_boot_notify sai_vs_meta_warm_boot_notify
#define meta_unittests_enabled sai_vs_meta_unittests_enabled
#define is_ipv6_mask_valid sai_vs_is_ipv6_mask_valid
#define get_attributes_metadata sai_vs_get_attributes_metadata
#define dump_object_reference sai_vs_dump_object_reference
#define object_reference_exists sai_vs_object_reference_exists
#define object_reference_inc sai_vs_object_reference_inc
#define object_reference_dec sai_vs_object_reference_dec
#define object_reference_insert sai_vs_object_reference_insert
#define object_reference_count sai_vs_object_reference_count
#define object_reference_remove sai_vs_object_reference_remove
#define object_exists sai_vs_object_exists
#define get_object_previous_attr sai_vs_get_object_previous_attr
#define set_object sai_vs_set_object
#define get_object_attributes sai_vs_get_object_attributes
#define remove_object sai_vs_remove_object
#define create_object sai_vs_create_object
#define meta_generic_validation_objlist sai_vs_meta_generic_validation_objlist
#define meta_genetic_validation_list sai_vs_meta_genetic_validation_list
#define meta_extract_switch_id sai_vs_meta_extract_switch_id
#define meta_generic_validate_non_object_on_create sai_vs_meta_generic_validate_non_object_on_create
#define meta_generic_validation_create sai_vs_meta_generic_validation_create
#define meta_generic_validation_remove sai_vs_meta_generic_validation_remove
#define meta_generic_validation_set sai_vs_meta_generic_validation_set
#define meta_generic_validation_get sai_vs_meta_generic_validation_get
#define meta_generic_validation_post_remove sai_vs_meta_generic_validation_post_remove
#define meta_generic_validation_post_get_objlist sai_vs_meta_generic_validation_post_get_objlist
#define meta_generic_validation_post_get sai_vs_meta_generic_validation_post_get
#define meta_sai_validate_fdb_entry sai_vs_meta_sai_validate_fdb_entry
#define meta_sai_validate_mcast_fdb_entry sai_vs_meta_sai_validate_mcast_fdb_entry
#define meta_sai_validate_neighbor_entry sai_vs_meta_sai_validate_neighbor_entry
#define meta_sai_validate_route_entry sai_vs_meta_sai_validate_route_entry
#define meta_sai_validate_l2mc_entry sai_vs_meta_sai_validate_l2mc_entry
#define meta_sai_validate_ipmc_entry sai_vs_meta_sai_validate_ipmc_entry
#define meta_sai_validate_inseg_entry sai_vs_meta_sai_validate_inseg_entry
#define meta_generic_validation_get_stats sai_vs_meta_generic_validation_get_stats
#define meta_sai_get_stats_oid sai_vs_meta_sai_get_stats_oid
#define meta_sai_on_fdb_flush_event_consolidated sai_vs_meta_sai_on_fdb_flush_event_consolidated
#define meta_fdb_event_snoop_oid sai_vs_meta_fdb_event_snoop_oid
#define meta_sai_on_fdb_event_single sai_vs_meta_sai_on_fdb_event_single

#include "vslib/src/sai_vs_acl.cpp"
#include "vslib/src/sai_vs_bfd.cpp"
#include "vslib/src/sai_vs_bmtor.cpp"
#include "vslib/src/sai_vs_bridge.cpp"
#include "vslib/src/sai_vs_buffer.cpp"
#include "vslib/src/sai_vs_dtel.cpp"
#include "vslib/src/sai_vs_fdb.cpp"
#include "vslib/src/sai_vs_hash.cpp"
#include "vslib/src/sai_vs_hostintf.cpp"
#include "vslib/src/sai_vs_interfacequery.cpp"
#include "vslib/src/sai_vs_ipmc.cpp"
#include "vslib/src/sai_vs_ipmc_group.cpp"
#include "vslib/src/sai_vs_l2mc.cpp"
#include "vslib/src/sai_vs_l2mcgroup.cpp"
#include "vslib/src/sai_vs_lag.cpp"
#include "vslib/src/sai_vs_mcastfdb.cpp"
#include "vslib/src/sai_vs_mirror.cpp"
#include "vslib/src/sai_vs_mpls.cpp"
#include "vslib/src/sai_vs_neighbor.cpp"
#include "vslib/src/sai_vs_nexthop.cpp"
#include "vslib/src/sai_vs_nexthopgroup.cpp"
#include "vslib/src/sai_vs_policer.cpp"
#include "vslib/src/sai_vs_port.cpp"
#include "vslib/src/sai_vs_qosmaps.cpp"
#include "vslib/src/sai_vs_queue.cpp"
#include "vslib/src/sai_vs_route.cpp"
#include "vslib/src/sai_vs_router_interface.cpp"
#include "vslib/src/sai_vs_rpfgroup.cpp"
#include "vslib/src/sai_vs_samplepacket.cpp"
#include "vslib/src/sai_vs_scheduler.cpp"
#include "vslib/src/sai_vs_schedulergroup.cpp"
#include "vslib/src/sai_vs_segmentroute.cpp"
#include "vslib/src/sai_vs_stp.cpp"
#include "vslib/src/sai_vs_switch.cpp"
#include "vslib/src/sai_vs_tam.cpp"
#include "vslib/src/sai_vs_tunnel.cpp"
#include "vslib/src/sai_vs_udf.cpp"
#include "vslib/src/sai_vs_virtual_router.cpp"
#include "vslib/src/sai_vs_vlan.cpp"
#include "vslib/src/sai_vs_wred.cpp"

//#define redis_construct_object_id sai_vs_construct_object_id
//#define redis_get_switch_id_index sai_vs_get_switch_id_index

// sai_vs_generic_create.cpp
// local variables
#define real_ids sai_vs_real_ids
#define switch_ids sai_vs_switch_ids

#include "vslib/src/sai_vs_generic_create.cpp"

#undef real_ids
#undef switch_ids

#include "vslib/src/sai_vs_generic_get.cpp"
#include "vslib/src/sai_vs_generic_remove.cpp"
#include "vslib/src/sai_vs_generic_set.cpp"
#include "vslib/src/sai_vs_generic_stats.cpp"
#include "vslib/src/sai_vs.cpp"

#define ss sai_vs_switch_BCM56850_ss
#define port_list sai_vs_switch_BCM56850_port_list
#define bridge_port_list_port_based sai_vs_switch_BCM56850_bridge_port_list_port_based
#define default_vlan_id sai_vs_switch_BCM56850_default_vlan_id

#define set_switch_mac_address sai_vs_switch_BCM56850_set_switch_mac_address
#define create_cpu_port sai_vs_switch_BCM56850_create_cpu_port
#define create_default_vlan sai_vs_switch_BCM56850_create_default_vlan
#define create_default_virtual_router sai_vs_switch_BCM56850_create_default_virtual_router
#define create_default_stp_instance sai_vs_switch_BCM56850_create_default_stp_instance
#define create_default_1q_bridge sai_vs_switch_BCM56850_create_default_1q_bridge
#define create_default_trap_group sai_vs_switch_BCM56850_create_default_trap_group
#define create_ports sai_vs_switch_BCM56850_create_ports
#define create_port_list sai_vs_switch_BCM56850_create_port_list
#define create_bridge_ports sai_vs_switch_BCM56850_create_bridge_ports
#define create_vlan_members sai_vs_switch_BCM56850_create_vlan_members
#define create_acl_entry_min_prio sai_vs_switch_BCM56850_create_acl_entry_min_prio
#define create_ingress_priority_groups sai_vs_switch_BCM56850_create_ingress_priority_groups
#define create_qos_queues sai_vs_switch_BCM56850_create_qos_queues
#define set_maximum_number_of_childs_per_scheduler_group sai_vs_switch_BCM56850_set_maximum_number_of_childs_per_scheduler_group
#define set_switch_default_attributes sai_vs_switch_BCM56850_set_switch_default_attributes
#define create_scheduler_groups sai_vs_switch_BCM56850_create_scheduler_groups
#define create_scheduler_group_tree sai_vs_switch_BCM56850_create_scheduler_group_tree
#define initialize_default_objects sai_vs_switch_BCM56850_initialize_default_objects
#define warm_boot_initialize_objects sai_vs_switch_BCM56850_warm_boot_initialize_objects
#define refresh_bridge_port_list sai_vs_switch_BCM56850_refresh_bridge_port_list
#define refresh_vlan_member_list sai_vs_switch_BCM56850_refresh_vlan_member_list
#define refresh_ingress_priority_group sai_vs_switch_BCM56850_refresh_ingress_priority_group
#define refresh_qos_queues sai_vs_switch_BCM56850_refresh_qos_queues
#define refresh_scheduler_groups sai_vs_switch_BCM56850_refresh_scheduler_groups

#include "vslib/src/sai_vs_switch_BCM56850.cpp"

#undef ss
#undef port_list
#undef bridge_port_list_port_based
#undef default_vlan_id

#undef set_switch_mac_address
#undef create_cpu_port
#undef create_default_vlan
#undef create_default_virtual_router
#undef create_default_stp_instance
#undef create_default_1q_bridge
#undef create_default_trap_group
#undef create_ports
#undef create_port_list
#undef create_bridge_ports
#undef create_vlan_members
#undef create_acl_entry_min_prio
#undef create_ingress_priority_groups
#undef create_qos_queues
#undef set_maximum_number_of_childs_per_scheduler_group
#undef set_switch_default_attributes
#undef create_scheduler_groups
#undef create_scheduler_group_tree
#undef initialize_default_objects
#undef warm_boot_initialize_objects
#undef refresh_bridge_port_list
#undef refresh_vlan_member_list
#undef refresh_ingress_priority_group
#undef refresh_qos_queues
#undef refresh_scheduler_groups

#define ss sai_vs_switch_MLNX2700_ss
#define port_list sai_vs_switch_MLNX2700_port_list
#define bridge_port_list_port_based sai_vs_switch_MLNX2700_bridge_port_list_port_based
#define default_vlan_id sai_vs_switch_MLNX2700_default_vlan_id
#define default_bridge_port_1q_router sai_vs_switch_MLNX2700_default_bridge_port_1q_router
#define cpu_port_id sai_vs_switch_MLNX2700_cpu_port_id

#define set_switch_mac_address sai_vs_switch_MLNX2700_set_switch_mac_address
#define create_cpu_port sai_vs_switch_MLNX2700_create_cpu_port
#define create_default_vlan sai_vs_switch_MLNX2700_create_default_vlan
#define create_default_virtual_router sai_vs_switch_MLNX2700_create_default_virtual_router
#define create_default_stp_instance sai_vs_switch_MLNX2700_create_default_stp_instance
#define create_default_1q_bridge sai_vs_switch_MLNX2700_create_default_1q_bridge
#define create_default_trap_group sai_vs_switch_MLNX2700_create_default_trap_group
#define create_ports sai_vs_switch_MLNX2700_create_ports
#define create_port_list sai_vs_switch_MLNX2700_create_port_list
#define create_bridge_ports sai_vs_switch_MLNX2700_create_bridge_ports
#define create_vlan_members sai_vs_switch_MLNX2700_create_vlan_members
#define create_acl_entry_min_prio sai_vs_switch_MLNX2700_create_acl_entry_min_prio
#define create_ingress_priority_groups sai_vs_switch_MLNX2700_create_ingress_priority_groups
#define create_qos_queues sai_vs_switch_MLNX2700_create_qos_queues
#define set_maximum_number_of_childs_per_scheduler_group sai_vs_switch_MLNX2700_set_maximum_number_of_childs_per_scheduler_group
#define set_switch_default_attributes sai_vs_switch_MLNX2700_set_switch_default_attributes
#define create_scheduler_groups sai_vs_switch_MLNX2700_create_scheduler_groups
#define create_scheduler_group_tree sai_vs_switch_MLNX2700_create_scheduler_group_tree
#define initialize_default_objects sai_vs_switch_MLNX2700_initialize_default_objects
#define warm_boot_initialize_objects sai_vs_switch_MLNX2700_warm_boot_initialize_objects
#define refresh_bridge_port_list sai_vs_switch_MLNX2700_refresh_bridge_port_list
#define refresh_vlan_member_list sai_vs_switch_MLNX2700_refresh_vlan_member_list
#define refresh_ingress_priority_group sai_vs_switch_MLNX2700_refresh_ingress_priority_group
#define refresh_qos_queues sai_vs_switch_MLNX2700_refresh_qos_queues
#define refresh_scheduler_groups sai_vs_switch_MLNX2700_refresh_scheduler_groups

#include "vslib/src/sai_vs_switch_MLNX2700.cpp"

#undef ss
#undef port_list
#undef bridge_port_list_port_based
#undef default_vlan_id
#undef default_bridge_port_1q_router
#undef cpu_port_id

#undef set_switch_mac_address
#undef create_cpu_port
#undef create_default_vlan
#undef create_default_virtual_router
#undef create_default_stp_instance
#undef create_default_1q_bridge
#undef create_default_trap_group
#undef create_ports
#undef create_port_list
#undef create_bridge_ports
#undef create_vlan_members
#undef create_acl_entry_min_prio
#undef create_ingress_priority_groups
#undef create_qos_queues
#undef set_maximum_number_of_childs_per_scheduler_group
#undef set_switch_default_attributes
#undef create_scheduler_groups
#undef create_scheduler_group_tree
#undef initialize_default_objects
#undef warm_boot_initialize_objects
#undef refresh_bridge_port_list
#undef refresh_vlan_member_list
#undef refresh_ingress_priority_group
#undef refresh_qos_queues
#undef refresh_scheduler_groups


// sai_meta.cpp
// local variables
#define ObjectAttrHash sai_vs_ObjectAttrHash
#define warmBoot sai_vs_warmBoot

// local functions
#define get_attr_info sai_vs_get_attr_info
#define construct_key sai_vs_construct_key

#include "meta/sai_meta.cpp"

#undef ObjectAttrHash
#undef warmBoot
#undef get_attr_info
#undef construct_key

//#include "meta/saiattributelist.cpp"
//#include "meta/saiserialize.cpp"
