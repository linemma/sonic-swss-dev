#include <limits.h>
#include <unordered_map>
#include <algorithm>
#include "aclorch.h"
#include "logger.h"
#include "schema.h"
#include "ipprefix.h"
#include "converter.h"
#include "tokenize.h"
#include "timer.h"
#include "crmorch.h"
#include "bufferorch.h"
#include "directory.h"
#include "vrforch.h"

//MacAddress gMacAddress;

sai_acl_api_t*    sai_acl_api;
sai_port_api_t*   sai_port_api;
sai_switch_api_t* sai_switch_api;
sai_object_id_t   gSwitchId;


/*
sai_vlan_api_t *sai_vlan_api;
sai_bridge_api_t *sai_bridge_api;
sai_lag_api_t *sai_lag_api;
sai_hostif_api_t* sai_hostif_api;
sai_queue_api_t *sai_queue_api;
IntfsOrch *gIntfsOrch;
NeighOrch *gNeighOrch;
BufferOrch *gBufferOrch;
RouteOrch *gRouteOrch;
sai_neighbor_api_t*         sai_neighbor_api;
sai_next_hop_api_t*         sai_next_hop_api;
sai_object_id_t gVirtualRouterId;
sai_next_hop_group_api_t*    sai_next_hop_group_api;
sai_route_api_t*             sai_route_api;
sai_mirror_api_t *sai_mirror_api;
sai_dtel_api_t* sai_dtel_api;
sai_buffer_api_t *sai_buffer_api;
sai_router_interface_api_t*  sai_router_intfs_api;
sai_fdb_api_t    *sai_fdb_api;
sai_virtual_router_api_t* sai_virtual_router_api;
sai_tunnel_api_t *sai_tunnel_api;
sai_object_id_t  gUnderlayIfId;

int gBatchSize;

bool gSwssRecord;
ofstream gRecordOfs;
bool gLogRotate;
string gRecordFile;

Directory<Orch*> gDirectory;
*/