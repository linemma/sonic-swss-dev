#pragma once

extern "C" {
    #include "sai.h"
}

#define sai_api_initialize sai_redis_sai_api_initialize
#define sai_api_query sai_redis_sai_api_query
#define sai_api_uninitialize sai_redis_sai_api_uninitialize
#define sai_log_set sai_redis_sai_log_set
#define sai_object_type_query sai_redis_sai_object_type_query
#define sai_switch_id_query sai_redis_sai_switch_id_query

extern "C" {
/**
 * @brief Adapter module initialization call
 *
 * This is NOT for SDK initialization.
 *
 * @param[in] flags Reserved for future use, must be zero
 * @param[in] services Methods table with services provided by adapter host
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_redis_sai_api_initialize(
    _In_ uint64_t flags,
    _In_ const sai_service_method_table_t* services);

/**
 * @brief Retrieve a pointer to the C-style method table for desired SAI
 * functionality as specified by the given sai_api_id.
 *
 * @param[in] api SAI API ID
 * @param[out] api_method_table Caller allocated method table. The table must
 * remain valid until the sai_api_uninitialize() is called.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_redis_sai_api_query(
    _In_ sai_api_t api,
    _Out_ void** api_method_table);

/**
 * @brief Uninitialize adapter module. SAI functionalities,
 * retrieved via sai_api_query() cannot be used after this call.
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_redis_sai_api_uninitialize(void);

/**
 * @brief Set log level for SAI API module
 *
 * The default log level is #SAI_LOG_LEVEL_WARN.
 *
 * @param[in] api SAI API ID
 * @param[in] log_level Log level
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_redis_sai_log_set(
    _In_ sai_api_t api,
    _In_ sai_log_level_t log_level);

/**
 * @brief Query SAI object type.
 *
 * @param[in] object_id Object id
 *
 * @return #SAI_OBJECT_TYPE_NULL when sai_object_id is not valid.
 * Otherwise, return a valid SAI object type SAI_OBJECT_TYPE_XXX.
 */
sai_object_type_t sai_redis_sai_object_type_query(
    _In_ sai_object_id_t object_id);

/**
 * @brief Query SAI switch id.
 *
 * @param[in] object_id Object id
 *
 * @return #SAI_NULL_OBJECT_ID when sai_object_id is not valid.
 * Otherwise, return a valid SAI_OBJECT_TYPE_SWITCH object on which
 * provided object id belongs. If valid switch id object is provided
 * as input parameter it should return itself.
 */
sai_object_id_t sai_redis_sai_switch_id_query(
    _In_ sai_object_id_t object_id);

/**
 * @brief Generate dump file. The dump file may include SAI state information and vendor SDK information.
 *
 * @param[in] dump_file_name Full path for dump file
 *
 * @return #SAI_STATUS_SUCCESS on success, failure status code on error
 */
sai_status_t sai_redis_sai_dbg_generate_dump(
    _In_ const char* dump_file_name);

//#include "sai.h"
}
