/**
 * @file sysrepo.h
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief public API sysrepo header
 *
 * @copyright
 * Copyright (c) 2019 CESNET, z.s.p.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _SYSREPO_H
#define _SYSREPO_H

/**
 * @defgroup cl Client Library
 * @{
 *
 * @brief Provides the public API towards applications using sysrepo to store
 * their configuration data, or towards management agents.
 *
 * Communicates with Sysrepo Engine (@ref cm), which is running either inside
 * of dedicated sysrepo daemon, or within this library if daemon is not alive.
 *
 * Access to the sysrepo datastore is connection- and session- oriented. Before
 * calling any data access/manipulation API, one needs to connect to the datastore
 * via ::sr_connect and open a session via ::sr_session_start. One connection
 * can serve multiple sessions.
 *
 * Each data access/manipulation request call is blocking - blocks the connection
 * until the response from Sysrepo Engine comes, or until an error occurs. It is
 * safe to call multiple requests on the same session (or different session that
 * belongs to the same connection) from multiple threads at the same time,
 * however it is not effective, since each call is blocked until previous one
 * finishes. If you need fast multi-threaded access to sysrepo, use a dedicated
 * connection for each thread.
 *
 * @see
 * See @ref main_page "Sysrepo Introduction" for details about sysrepo architecture.
 * @see
 * @ref xp_page "XPath Addressing" is used for node identification in data-related calls.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#ifdef __APPLE__
    #include <sys/types.h>
#endif

#include <libyang/libyang.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
// Common typedefs and API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Sysrepo connection context used to identify a connection to sysrepo datastore.
 */
typedef struct sr_conn_ctx_s sr_conn_ctx_t;

/**
 * @brief Sysrepo session context used to identify a configuration session on a connection.
 */
typedef struct sr_session_ctx_s sr_session_ctx_t;

/**
 * @brief Possible types of a data element stored in the sysrepo datastore.
 */
typedef enum sr_type_e {
    /* special types that does not contain any data */
    SR_UNKNOWN_T,              /**< Element unknown to sysrepo (unsupported element). */
    SR_TREE_ITERATOR_T,        /**< Special type of tree node used to store all data needed for iterative tree loading. */

    SR_LIST_T,                 /**< List instance. ([RFC 6020 sec 7.8](http://tools.ietf.org/html/rfc6020#section-7.8)) */
    SR_CONTAINER_T,            /**< Non-presence container. ([RFC 6020 sec 7.5](http://tools.ietf.org/html/rfc6020#section-7.5)) */
    SR_CONTAINER_PRESENCE_T,   /**< Presence container. ([RFC 6020 sec 7.5.1](http://tools.ietf.org/html/rfc6020#section-7.5.1)) */
    SR_LEAF_EMPTY_T,           /**< A leaf that does not hold any value ([RFC 6020 sec 9.11](http://tools.ietf.org/html/rfc6020#section-9.11)) */
    SR_NOTIFICATION_T,         /**< Notification instance ([RFC 7095 sec 7.16](https://tools.ietf.org/html/rfc7950#section-7.16)) */

    /* types containing some data */
    SR_BINARY_T,       /**< Base64-encoded binary data ([RFC 6020 sec 9.8](http://tools.ietf.org/html/rfc6020#section-9.8)) */
    SR_BITS_T,         /**< A set of bits or flags ([RFC 6020 sec 9.7](http://tools.ietf.org/html/rfc6020#section-9.7)) */
    SR_BOOL_T,         /**< A boolean value ([RFC 6020 sec 9.5](http://tools.ietf.org/html/rfc6020#section-9.5)) */
    SR_DECIMAL64_T,    /**< 64-bit signed decimal number ([RFC 6020 sec 9.3](http://tools.ietf.org/html/rfc6020#section-9.3)) */
    SR_ENUM_T,         /**< A string from enumerated strings list ([RFC 6020 sec 9.6](http://tools.ietf.org/html/rfc6020#section-9.6)) */
    SR_IDENTITYREF_T,  /**< A reference to an abstract identity ([RFC 6020 sec 9.10](http://tools.ietf.org/html/rfc6020#section-9.10)) */
    SR_INSTANCEID_T,   /**< References a data tree node ([RFC 6020 sec 9.13](http://tools.ietf.org/html/rfc6020#section-9.13)) */
    SR_INT8_T,         /**< 8-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_INT16_T,        /**< 16-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_INT32_T,        /**< 32-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_INT64_T,        /**< 64-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_STRING_T,       /**< Human-readable string ([RFC 6020 sec 9.4](http://tools.ietf.org/html/rfc6020#section-9.4)) */
    SR_UINT8_T,        /**< 8-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_UINT16_T,       /**< 16-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_UINT32_T,       /**< 32-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_UINT64_T,       /**< 64-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    SR_ANYXML_T,       /**< Unknown chunk of XML ([RFC 6020 sec 7.10](https://tools.ietf.org/html/rfc6020#section-7.10)) */
    SR_ANYDATA_T,      /**< Unknown set of nodes, encoded in XML ([RFC 7950 sec 7.10](https://tools.ietf.org/html/rfc7950#section-7.10)) */
} sr_type_t;

/**
 * @brief Data of an element (if applicable), properly set according to the type.
 */
typedef union sr_data_u {
    char *binary_val;       /**< Base64-encoded binary data ([RFC 6020 sec 9.8](http://tools.ietf.org/html/rfc6020#section-9.8)) */
    char *bits_val;         /**< A set of bits or flags ([RFC 6020 sec 9.7](http://tools.ietf.org/html/rfc6020#section-9.7)) */
    bool bool_val;          /**< A boolean value ([RFC 6020 sec 9.5](http://tools.ietf.org/html/rfc6020#section-9.5)) */
    double decimal64_val;   /**< 64-bit signed decimal number ([RFC 6020 sec 9.3](http://tools.ietf.org/html/rfc6020#section-9.3)) */
    char *enum_val;         /**< A string from enumerated strings list ([RFC 6020 sec 9.6](http://tools.ietf.org/html/rfc6020#section-9.6)) */
    char *identityref_val;  /**< A reference to an abstract identity ([RFC 6020 sec 9.10](http://tools.ietf.org/html/rfc6020#section-9.10)) */
    char *instanceid_val;   /**< References a data tree node ([RFC 6020 sec 9.13](http://tools.ietf.org/html/rfc6020#section-9.13)) */
    int8_t int8_val;        /**< 8-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    int16_t int16_val;      /**< 16-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    int32_t int32_val;      /**< 32-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    int64_t int64_val;      /**< 64-bit signed integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    char *string_val;       /**< Human-readable string ([RFC 6020 sec 9.4](http://tools.ietf.org/html/rfc6020#section-9.4)) */
    uint8_t uint8_val;      /**< 8-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    uint16_t uint16_val;    /**< 16-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    uint32_t uint32_val;    /**< 32-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    uint64_t uint64_val;    /**< 64-bit unsigned integer ([RFC 6020 sec 9.2](http://tools.ietf.org/html/rfc6020#section-9.2)) */
    char *anyxml_val;       /**< Unknown chunk of XML ([RFC 6020 sec 7.10](https://tools.ietf.org/html/rfc6020#section-7.10)) */
    char *anydata_val;      /**< Unknown set of nodes, encoded in XML ([RFC 7950 sec 7.10](https://tools.ietf.org/html/rfc7950#section-7.10)) */
} sr_data_t;

/**
 * @brief Structure that contains value of an data element stored in the sysrepo datastore.
 */
typedef struct sr_val_s {

    /**
     * XPath identifier of the data element, as defined in
     * @ref xp_page "Path Addressing" documentation
     */
    char *xpath;

    /** Type of an element. */
    sr_type_t type;

    /**
     * Flag for node with default value (applicable only for leaves).
     * It is set to TRUE only if the value was *implicitly* set by the datastore as per
     * module schema. Explicitly set/modified data element (through the sysrepo API) always
     * has this flag unset regardless of the entered value.
     */
    bool dflt;

    /** Data of an element (if applicable), properly set according to the type. */
    sr_data_t data;

} sr_val_t;

/**
 * @brief Sysrepo error codes.
 */
typedef enum sr_error_e {
    SR_ERR_OK = 0,             /**< No error. */
    SR_ERR_INVAL_ARG,          /**< Invalid argument. */
    SR_ERR_LY,                 /**< Error generated by libyang. */
    SR_ERR_SYS,                /**< System function call failed. */
    SR_ERR_NOMEM,              /**< Not enough memory. */
    SR_ERR_NOT_FOUND,          /**< Item not found. */
    SR_ERR_EXISTS,             /**< Item already exists. */
    SR_ERR_INTERNAL,           /**< Other internal error. */
    SR_ERR_INIT_FAILED,        /**< Sysrepo initialization failed. */
    SR_ERR_UNSUPPORTED,        /**< Unsupported operation requested. */
    SR_ERR_UNKNOWN_MODEL,      /**< Request includes unknown schema */
    SR_ERR_BAD_ELEMENT,        /**< Unknown element in existing schema */
    SR_ERR_VALIDATION_FAILED,  /**< Validation of the changes failed. */
    SR_ERR_OPERATION_FAILED,   /**< An operation failed. */
    SR_ERR_UNAUTHORIZED,       /**< Operation not authorized. */
    SR_ERR_LOCKED,             /**< Requested resource is already locked. */
    SR_ERR_TIME_OUT,           /**< Time out has expired. */
    SR_ERR_CALLBACK_FAILED,    /**< User callback failure caused the operation to fail. */
} sr_error_t;

/**
 * @brief Detailed sysrepo error information.
 */
typedef struct sr_error_info_s {
    sr_error_t err_code; /**< Error code. */
    struct {
        char *message;   /**< Error message. */
        char *xpath;     /**< XPath to the node where the error has been discovered. */
    } *err;
    size_t err_count;    /**< Error message count. */
} sr_error_info_t;

/**
 * @brief Returns the error message corresponding to the error code.
 *
 * @param[in] err_code Error code.
 * @return Error message (statically allocated, do not free).
 */
const char *sr_strerror(int err_code);

/**
 * @brief Log levels used to determine if message of certain severity should be printed.
 */
typedef enum {
    SR_LL_NONE = 0,  /**< Do not print any messages. */
    SR_LL_ERR,       /**< Print only error messages. */
    SR_LL_WRN,       /**< Print error and warning messages. */
    SR_LL_INF,       /**< Besides errors and warnings, print some other informational messages. */
    SR_LL_DBG,       /**< Print all messages including some development debug messages. */
} sr_log_level_t;

/**
 * @brief Enables / disables / changes log level (verbosity) of logging to
 * standard error output.
 *
 * By default, logging to stderr is disabled. Setting log level to any value
 * other than SR_LL_NONE enables the logging to stderr. Setting log level
 * back to SR_LL_NONE disables the logging to stderr.
 *
 * @note Please note that this will overwrite your libyang logging settings.
 * Only libyang errors are printed, if enabled.
 *
 * @param[in] log_level Requested log level (verbosity).
 */
void sr_log_stderr(sr_log_level_t log_level);

/**
 * @brief Enables / disables / changes log level (verbosity) of logging to system log.
 *
 * By default, logging into syslog is disabled. Setting log level to any value
 * other than SR_LL_NONE enables the logging into syslog. Setting log level
 * back to SR_LL_NONE disables the logging into syslog.
 *
 * @note Please note that enabling logging into syslog will overwrite your syslog
 * connection settings (calls openlog), if you are connected to syslog already.
 *
 * @param[in] log_level Requested log level (verbosity).
 */
void sr_log_syslog(sr_log_level_t log_level);

/**
 * @brief Sets callback that will be called when a log entry would be populated.
 *
 * @param[in] level Verbosity level of the log entry.
 * @param[in] message Message of the log entry.
 */
typedef void (*sr_log_cb)(sr_log_level_t level, const char *message);

/**
 * @brief Sets callback that will be called when a log entry would be populated.
 * Callback will be called for each message with any log level.
 *
 * @param[in] log_callback Callback to be called when a log entry would populated.
 */
void sr_log_set_cb(sr_log_cb log_callback);

/**
 * @brief Get the common path prefix for all sysrepo files.
 *
 * @note If a specific path was changed, it does not use this
 * path prefix.
 *
 * @return Sysrepo repository path.
 */
const char *sr_get_repo_path(void);


////////////////////////////////////////////////////////////////////////////////
// Connection / Session Management
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Flags used to override default connection handling by ::sr_connect call.
 */
typedef enum sr_conn_flag_e {
    SR_CONN_DEFAULT = 0,          /**< No special behaviour. */
    SR_CONN_CACHE_RUNNING = 1,    /**< Always cache running datastore data which makes mainly repeated retrieval of data
                                       faster. Affects all sessions created on this connection. */
} sr_conn_flag_t;

/**
 * @brief Options overriding default connection handling by ::sr_connect call,
 * it is supposed to be bitwise OR-ed value of any ::sr_conn_flag_t flags.
 */
typedef uint32_t sr_conn_options_t;

/**
 * @brief Data stores that sysrepo supports. Their meaning should conform to RFC 8342.
 * To make changes permanent in an edited datastore ::sr_apply_changes must be issued.
 * @see @ref ds_page "Datastores & Sessions" information page.
 */
typedef enum sr_datastore_e {
    SR_DS_STARTUP = 0,     /**< Contains configuration data that will be loaded when a device starts. */
    SR_DS_RUNNING = 1,     /**< Contains current configuration data. */
    SR_DS_OPERATIONAL = 2, /**< Contains currently used configuration and state data. */
} sr_datastore_t;

/**
 * @brief Connects to the sysrepo datastore.
 *
 * @param[in] app_name Name of the application connecting to the datastore
 * (can be a static string). Used only for logging purposes.
 * @param[in] opts Options overriding default connection handling by this call.
 * @param[out] conn_ctx Connection context that can be used for subsequent API calls
 * (automatically allocated, it is supposed to be released by the caller using ::sr_disconnect).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_connect(const char *app_name, const sr_conn_options_t opts, sr_conn_ctx_t **conn_ctx);

/**
 * @brief Disconnects from the sysrepo datastore.
 *
 * Cleans up and frees connection context allocated by ::sr_connect. All sessions and subscriptions
 * started within the connection will be automatically stopped and cleaned up too.
 *
 * @param[in] conn_ctx Connection context acquired with ::sr_connect call.
 */
void sr_disconnect(sr_conn_ctx_t *conn_ctx);

/**
 * @brief Starts a new configuration session.
 *
 * @see @ref ds_page "Datastores & Sessions" for more information about datastores and sessions.
 *
 * @param[in] conn_ctx Connection context acquired with ::sr_connect call.
 * @param[in] datastore Datastore on which all sysrepo functions within this
 * session will operate. Later on, datastore can be later changed using
 * ::sr_session_switch_ds call. Functionality of some sysrepo calls does not depend on
 * datastore. If your session will contain just calls like these, you can pass
 * any valid value (e.g. SR_RUNNING).
 * @param[out] session Session context that can be used for subsequent API
 * calls (automatically allocated, can be released by calling ::sr_session_stop).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_session_start(sr_conn_ctx_t *conn_ctx, const sr_datastore_t datastore, sr_session_ctx_t **session);

/**
 * @brief Stops current session and releases resources and subscriptions tied to the session.
 *
 * @param[in] session Session context acquired with ::sr_session_start call.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_session_stop(sr_session_ctx_t *session);

/**
 * @brief Changes datastore which the session operates on. All subsequent
 * calls will be issued on the chosen datastore.
 *
 * @param[in] session Session to modify.
 * @param[in] ds New datastore that will be operated on from now on.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_session_switch_ds(sr_session_ctx_t *session, sr_datastore_t ds);

/**
 * @brief Learn the datastore a session operates on.
 *
 * @param[in] session Session to inspect.
 * @return Datastore of the session.
 */
sr_datastore_t sr_session_get_ds(sr_session_ctx_t *session);

/**
 * @brief Retrieves information about the error that has occurred
 * during the last operation executed within provided session.
 *
 * @param[in] session Session to inspect.
 * @param[out] error_info Detailed error information. Be aware that
 * returned pointer may change by the next API call executed within the provided
 * session. Do not free or modify returned values.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_error(sr_session_ctx_t *session, const sr_error_info_t **error_info);

/**
 * @brief Sets detailed error information into provided session. Used to notify
 * the client library about errors that occurred in the application code.
 *
 * @note Intended for change, RPC/action, or data-provide callbacks to be used
 * on the provided session.
 *
 * @param[in] session Session provided in a callback.
 * @param[in] message Human-readable error message.
 * @param[in] xpath XPath to the node where the error has occurred. NULL value
 * is also accepted.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_set_error(sr_session_ctx_t *session, const char *message, const char *xpath);

/**
 * @brief Returns the assigned session ID of the sysrepo session.
 *
 * @param [in] session Session to inspect.
 * @return sysrepo SID or 0 in case of error.
 */
uint32_t sr_session_get_id(sr_session_ctx_t *session);

/**
 * @brief Sets a NETCONF session ID for a sysrepo session. Any application
 * callbacks handling operations initiated by this session will be able to
 * read this ID from the session provided.
 *
 * @param[in] session Session to change.
 * @param[in] nc_sid NETCONF session ID of a NETCONF session running on top of this session.
 */
void sr_session_set_nc_id(sr_session_ctx_t *session, uint32_t nc_sid);

/**
 * @brief Learns NETCONF session ID from a sysrepo session. Either reads back
 * the value set by ::sr_session_set_nc_id or of the initiating session when used
 * in an application callback.
 *
 * @param[in] session Session to inspect.
 * @return Session NETCONF SID.
 */
uint32_t sr_session_get_nc_id(sr_session_ctx_t *session);

/**
 * @brief Set the effective user of a session to a different one that the process owner.
 *
 * Required ROOT access.
 *
 * @param[in] session Session to change.
 * @param[in] user System user.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_session_set_user(sr_session_ctx_t *session, const char *user);

/**
 * @brief Get the effective user of a session.
 *
 * Required ROOT access.
 *
 * @param[in] session Session to inspect.
 * @return Session user.
 */
const char *sr_session_get_user(sr_session_ctx_t *session);

/**
 * @brief Get the connection the session was created on.
 *
 * @param[in] session Session to inspect.
 * @return Sysrepo connection.
 */
sr_conn_ctx_t *sr_session_get_connection(sr_session_ctx_t *session);


////////////////////////////////////////////////////////////////////////////////
// Data Retrieval API (get / get-config functionality)
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Retrieves a single data element stored under provided XPath. If multiple
 * nodes matches the xpath ::SR_ERR_INVAL_ARG is returned.
 *
 * If the xpath identifies an empty leaf, a list or a container, the value
 * has no data filled in and its type is set properly
 * (::SR_LEAF_EMPTY_T / ::SR_LIST_T / ::SR_CONTAINER_T / ::SR_CONTAINER_PRESENCE_T).
 *
 * Required READ access.
 *
 * @see @ref xp_page "Path Addressing" documentation, or
 * https://tools.ietf.org/html/draft-ietf-netmod-yang-json#section-6.11
 * for XPath syntax used for identification of yang nodes in sysrepo calls.
 *
 * @see Use ::sr_get_items for retrieving larger chunks
 * of data from the datastore. Since it retrieves the data from datastore in
 * larger chunks, it can work much more efficiently than multiple ::sr_get_item calls.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be retrieved.
 * @param[out] value Structure containing information about requested element
 * (allocated by the function, it is supposed to be freed by the caller using ::sr_free_val).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_item(sr_session_ctx_t *session, const char *xpath, sr_val_t **value);

/**
 * @brief Retrieves an array of data elements matching provided XPath
 *
 * All data elements are transferred within one message from the datastore,
 * which is much more efficient that calling multiple ::sr_get_item calls.
 *
 * Required READ access.
 *
 * @see @ref xp_page "Path Addressing" documentation
 * for Path syntax used for identification of yang nodes in sysrepo calls.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be retrieved.
 * @param[out] values Array of structures containing information about requested data elements
 * (allocated by the function, it is supposed to be freed by the caller using ::sr_free_values).
 * @param[out] value_cnt Number of returned elements in the values array.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_items(sr_session_ctx_t *session, const char *xpath, sr_val_t **values, size_t *value_cnt);

/**
 * @brief Retrieves a single subtree whose root node is stored under the provided XPath.
 * If multiple nodes matches the xpath ::SR_ERR_INVAL_ARG is returned.
 *
 * The functions returns values and all associated information stored under the root node and
 * all its descendants. While the same data can be obtained using ::sr_get_items in combination
 * with the expressive power of XPath addressing, the recursive nature of the output data type
 * also preserves the hierarchical relationships between data elements.
 *
 * Required READ access.
 *
 * @see @ref xp_page "Path Addressing" documentation
 * for XPath syntax used for identification of yang nodes in sysrepo calls.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier referencing the root node of the subtree to be retrieved.
 * @param[out] subtree Nested subtree containing all data of the requested subtree
 * (allocated by the function, it is supposed to be freed by the caller).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_subtree(sr_session_ctx_t *session, const char *xpath, struct lyd_node **subtree);

/**
 * @brief Retrieves an array of subtrees whose root nodes match the provided XPath.
 *
 * Subtrees that match the provided XPath are not merged even if they overlap. This significantly
 * simplifies the implementation and decreases the cost of this operation. The downside is that
 * the user must choose the XPath carefully. If the subtree selection process results in too many
 * node overlaps, the cost of the operation may easily outshine the benefits. As an example,
 * a common XPath expression "//." is normally used to select all nodes in a data tree, but for this
 * operation it would result in an excessive duplication of transfered data elements.
 * Since you get all the descendants of each matched node implicitly, you probably should not need
 * to use XPath wildcards deeper than on the top-level.
 * (i.e. "/." is preferred alternative to "//." for get-subtrees operation).
 *
 * Required READ access.
 *
 * @see @ref xp_page "Path Addressing" documentation, or
 * https://tools.ietf.org/html/draft-ietf-netmod-yang-json#section-6.11
 * for XPath syntax used for identification of yang nodes in sysrepo calls.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier referencing root nodes of subtrees to be retrieved.
 * @param[out] subtrees Set of nested structures storing all data of the requested subtrees
 * (allocated by the function, it is supposed to be freed by the caller).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_subtrees(sr_session_ctx_t *session, const char *xpath, struct ly_set **subtrees);


////////////////////////////////////////////////////////////////////////////////
// Data Manipulation API (edit-config functionality)
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Flags used to override default behavior of data manipulation calls.
 */
typedef enum sr_edit_flag_e {
    SR_EDIT_DEFAULT = 0,        /**< Default behavior - non-strict. */
    SR_EDIT_NON_RECURSIVE = 1,  /**< Non-recursive behavior:
                                     by ::sr_set_item, all preceding nodes (parents) of the identified element must exist. */
    SR_EDIT_STRICT = 2          /**< Strict behavior:
                                     by ::sr_set_item the identified element must not exist (similar to NETCONF create operation),
                                     by ::sr_delete_item the identified element must exist (similar to NETCONF delete operation). */
} sr_edit_flag_t;

/**
 * @brief Options overriding default behavior of data manipulation calls,
 * it is supposed to be bitwise OR-ed value of any ::sr_edit_flag_t flags.
 */
typedef uint32_t sr_edit_options_t;

/**
 * @brief Options for specifying move direction of ::sr_move_item call.
 */
typedef enum sr_move_position_e {
    SR_MOVE_BEFORE = 0,    /**< Move the specified item before the selected sibling. */
    SR_MOVE_AFTER = 1,     /**< Move the specified item after the selected. */
    SR_MOVE_FIRST = 2,     /**< Move the specified item to the position of the first child. */
    SR_MOVE_LAST = 3,      /**< Move the specified item to the position of the last child. */
} sr_move_position_t;

/**
 * @brief Sets the value of the leaf, leaf-list, list or presence container.
 *
 * With default options it recursively creates all missing nodes (containers and
 * lists including their key leaves) in the xpath to the specified node (can be
 * turned off with SR_EDIT_NON_RECURSIVE option). If SR_EDIT_STRICT flag is set,
 * the node must not exist (otherwise an error is returned).
 *
 * To create a list use xpath with key values included and pass NULL as value argument.
 *
 * Setting of a leaf-list value appends the value at the end of the leaf-list.
 * A value of leaf-list can be specified either by predicate in xpath or by value argument.
 * If both are present, value argument is ignored and xpath predicate is used.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be set.
 * @param[in] value Value to be set on specified xpath. xpath member of the
 * ::sr_val_t structure can be NULL. Value will be copied - can be allocated on stack.
 * @param[in] opts Options overriding default behavior of this call.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_set_item(sr_session_ctx_t *session, const char *xpath, const sr_val_t *value, const sr_edit_options_t opts);

/**
 * @brief Functions is similar to ::sr_set_item with the difference that the value to be set
 * is provided as string.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be set.
 * @param[in] value String representation of the value to be set.
 * @param[in] opts Same as for ::sr_set_item.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_set_item_str(sr_session_ctx_t *session, const char *xpath, const char *value, const sr_edit_options_t opts);

/**
 * @brief Deletes the nodes matching the specified xpath. The accepted values are the same as for
 * ::sr_set_item_str.
 *
 * If SR_EDIT_STRICT flag is set the specified node must must exist in the datastore.
 * If the xpath includes the list keys, the specified list instance is deleted.
 * If the xpath to list does not include keys, all instances of the list are deleted.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be deleted.
 * @param[in] opts Options overriding default behavior of this call.
 * @return Error code (::SR_ERR_OK on success).
 **/
int sr_delete_item(sr_session_ctx_t *session, const char *xpath, const sr_edit_options_t opts);

/**
 * @brief Move the instance of an user-ordered list or leaf-list to the specified position.
 *
 * Item can be move to the first or last position or positioned relatively to its sibling.
 * @note To determine current order, you can issue a ::sr_get_items call
 * (without specifying keys of the list in question).
 *
 * @param[in] session Session to use
 * @param[in] xpath @ref xp_page "Data Path" identifier of the data element to be moved.
 * @param[in] position Requested move direction.
 * @param[in] list_keys Predicate identifying the relative list instance (example input "[key1='val1'][key2='val2']...").
 * @param[in] leaflist_value Value of the relative leaf-list instance (example input "val1").
 * to determine relative position, used only if position argument is SR_MOVE_BEFORE or SR_MOVE_AFTER.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_move_item(sr_session_ctx_t *session, const char *xpath, const sr_move_position_t position, const char *list_keys,
        const char *leaflist_value);

/**
 * @brief Provide a prepared edit data tree to be applied. Similar semantics to
 * NETCONF \<edit-config\> content.
 *
 * @param[in] session Session to use.
 * @param[in] edit Edit content.
 * @param[in] default_operation Default operation for nodes without operation on themselves or any parent.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_edit_batch(sr_session_ctx_t *session, const struct lyd_node *edit, const char *default_operation);

/**
 * @brief Perform the validation of changes made in current session, but do not
 * commit nor discard them.
 *
 * Provides only YANG validation, apply-changes subscribers won't be notified in this case.
 *
 * @see Use ::sr_get_error to retrieve error information if the operation returned an error.
 *
 * @param[in] session Session to use.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_validate(sr_session_ctx_t *session);

/**
 * @brief Apply changes made in current session.
 *
 * @note Note that in case that you are changing the running datastore, you also
 * need to copy the config to startup to make changes permanent after restart.
 *
 * @see Use ::sr_get_error to retrieve error information if the operation returned an error.
 *
 * Required WRITE access.
 *
 * @param[in] session Session context acquired with ::sr_session_start call.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_apply_changes(sr_session_ctx_t *session);

/**
 * @brief Discard non-committed changes made in current session.
 *
 * @param[in] session Session context acquired with ::sr_session_start call.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_discard_changes(sr_session_ctx_t *session);

/**
 * @brief Replaces a configuration datastore with the contents of
 * another configuration datastore. If the module is specified, limits
 * the operation only to the specified module. If it is not specified,
 * the operation is performed on all modules.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Optional module name that limits the copy operation only to this module.
 * @param[in] src_datastore Source datastore.
 * @param[in] dst_datastore Destination datastore.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_copy_config(sr_session_ctx_t *session, const char *module_name, sr_datastore_t src_datastore,
        sr_datastore_t dst_datastore);

/**
 * @brief Replace a configuration datastore with the contents of
 * a data tree. If the module is specified, limits the operation only to the specified module. If
 * it is not specified, the operation is performed on all modules.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name If specified, limits the replace operation only to this module.
 * @param[in] src_config Source configuration to replace the datastore one. Is ALWAYS spent
 * and cannot be used by the application!
 * @param[in] dst_datastore Destination datastore.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_replace_config(sr_session_ctx_t *session, const char *module_name, struct lyd_node *src_config,
        sr_datastore_t dst_datastore);


////////////////////////////////////////////////////////////////////////////////
// Locking API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Locks the data of the specified module or whole datastore.
 *
 * Required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Optional name of the module to be locked.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_lock(sr_session_ctx_t *session, const char *module_name);

/**
 * @brief Unlocks the data of the specified module or whole datastore.
 *
 * Required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Optional name of the module to be unlocked.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_unlock(sr_session_ctx_t *session, const char *module_name);


////////////////////////////////////////////////////////////////////////////////
// Change Notifications API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Flags used to override default handling of subscriptions.
 */
typedef enum sr_subscr_flag_e {
    /**
     * @brief Default behavior of the subscription. In case of ::sr_module_change_subscribe call it means that:
     *
     * - the subscriber is the "owner" of the subscribed data tree and it will appear in the operational
     *   datastore while this subscription is alive (if not already, can be changed using ::SR_SUBSCR_PASSIVE flag),
     * - the callback will be called twice, once with ::SR_EV_CHANGE event and once with ::SR_EV_DONE / ::SR_EV_ABORT
     *   event passed in (can be changed with ::SR_SUBSCR_DONE_ONLY flag).
     */
    SR_SUBSCR_DEFAULT = 0,

    /**
     * @brief This option enables the application to re-use an already existing subscription context previously returned
     * from any sr_*_subscribe call instead of requesting the creation of a new one. In that case a single
     * ::sr_unsubscribe call unsubscribes from all subscriptions filed within the context.
     */
    SR_SUBSCR_CTX_REUSE = 1,

    /**
     * @brief The subscriber is not the "owner" of the subscribed data tree, just a passive watcher for changes.
     * When this option is passed in to ::sr_module_change_subscribe, the subscription will have no effect on
     * the presence of the subtree in the operational datastore.
     */
    SR_SUBSCR_PASSIVE = 2,

    /**
     * @brief The subscriber does not support verification of the changes and wants to be notified only after
     * the changes has been applied in the datastore, without the possibility to deny them
     * (it will receive only ::SR_EV_DONE events).
     */
    SR_SUBSCR_DONE_ONLY = 4,

    /**
     * @brief The subscriber wants to be notified about the current configuration at the moment of subscribing.
     */
    SR_SUBSCR_ENABLED = 8,

    /**
     * @brief The subscriber will be called before any other subscribers for the particular module
     * and is allowed to modify the new module data.
     */
    SR_SUBSCR_UPDATE = 16,

} sr_subscr_flag_t;

/**
 * @brief Type of the notification event that has occurred (passed to application callbacks).
 *
 * @note Each change is normally announced twice: first as ::SR_EV_CHANGE event and then as ::SR_EV_DONE or ::SR_EV_ABORT
 * event. If the subscriber does not support verification, it can subscribe only to ::SR_EV_DONE event by providing
 * ::SR_SUBSCR_DONE_ONLY subscription flag.
 */
typedef enum sr_notif_event_e {
    SR_EV_UPDATE,  /**< Occurs before any other events and the subscriber can update the apply-changes diff. */
    SR_EV_CHANGE,  /**< Occurs just before the changes are committed to the datastore,
                        the subscriber is supposed to verify that the changes are valid and can be applied
                        and prepare all resources required for the changes. The subscriber can still deny the changes
                        in this phase by returning an error from the callback. */
    SR_EV_DONE,    /**< Occurs just after the changes have been successfully committed to the datastore,
                        the subscriber can apply the changes now, but it cannot deny the changes in this
                        phase anymore (any returned errors are just logged and ignored). */
    SR_EV_ABORT,   /**< Occurs in case that the commit transaction has failed (possibly because one of the verifiers
                        has denied the change / returned an error). The subscriber is supposed to return the managed
                        application to the state before the commit. Any returned errors are just logged and ignored. */
} sr_notif_event_t;

/**
 * @brief Type of the operation made on an item, used by changeset retrieval in ::sr_get_change_next.
 */
typedef enum sr_change_oper_e {
    SR_OP_CREATED,   /**< The item has been created by the change. */
    SR_OP_MODIFIED,  /**< The value of the item has been modified by the change. */
    SR_OP_DELETED,   /**< The item has been deleted by the change. */
    SR_OP_MOVED,     /**< The item has been moved in the subtree by the change (applicable for leaf-lists and user-ordered lists). */
} sr_change_oper_t;

/**
 * @brief Sysrepo subscription context returned from sr_*_subscribe calls,
 * it is supposed to be released by the caller using ::sr_unsubscribe call.
 */
typedef struct sr_subscription_ctx_s sr_subscription_ctx_t;

/**
 * @brief Iterator used for retrieval of a changeset using ::sr_get_changes_iter call.
 */
typedef struct sr_change_iter_s sr_change_iter_t;

/**
 * @brief Options overriding default behavior of subscriptions,
 * it is supposed to be a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 */
typedef uint32_t sr_subscr_options_t;

/**
 * @brief Callback to be called by the event of changing any running datastore
 * content within the specified module. Subscribe to it by ::sr_module_change_subscribe call.
 *
 * @param[in] session Automatically-created session that can be used for obtaining changed data
 * (by ::sr_get_changes_iter call) and learn about initiator session IDs. Do not stop this session.
 * @param[in] module_name Name of the module where the change has occurred.
 * @param[in] xpath XPath used when subscribing, NULL if the whole module was subscribed to.
 * @param[in] event Type of the notification event that has occurred.
 * @param[in] private_data Private context opaque to sysrepo, as passed to ::sr_module_change_subscribe call.
 * @return Error code (::SR_ERR_OK on success).
 */
typedef int (*sr_module_change_cb)(sr_session_ctx_t *session, const char *module_name, const char *xpath,
        sr_notif_event_t event, void *private_data);

/**
 * @brief Subscribes for changes made in the specified module.
 *
 * Required WRITE access. If ::SR_SUBSCR_PASSIVE is set, required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Name of the module of interest for change notifications.
 * @param[in] xpath Optionally further filter the changes that will be handled by this subscription.
 * @param[in] callback Callback to be called when the change in the datastore occurs.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] priority Specifies the order in which the callbacks will be called (callbacks with higher
 * priority will be called sooner, callbacks with the priority of 0 will be called at the end).
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @note An existing context may be passed in in case that ::SR_SUBSCR_CTX_REUSE option is specified.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_module_change_subscribe(sr_session_ctx_t *session, const char *module_name, const char *xpath,
        sr_module_change_cb callback, void *private_data, uint32_t priority, sr_subscr_options_t opts,
        sr_subscription_ctx_t **subscription);

/**
 * @brief Unsubscribes from a subscription acquired by any of sr_*_subscribe
 * calls and releases all subscription-related data.
 *
 * @note In case that the same subscription context was used to subscribe for
 * multiple subscriptions, unsubscribes from all of them.
 *
 * @param[in] subscription Subscription context acquired by any of sr_*_subscribe calls.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_unsubscribe(sr_subscription_ctx_t *subscription);

/**
 * @brief Creates an iterator for retrieving of the changeset (list of newly
 * added / removed / modified nodes) in module-change callbacks.
 *
 * @see ::sr_get_change_next for iterating over the changeset using this iterator.
 *
 * @param[in] session Session provided in the callbacks (::sr_module_change_cb). Will not work with other sessions.
 * @param[in] xpath @ref xp_page "Data Path" identifier of the subtree from which the changeset
 * should be obtained.
 * @param[out] iter Iterator context that can be used to retrieve individual changes using
 * ::sr_get_change_next calls. Allocated by the function, should be freed with ::sr_free_change_iter.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_changes_iter(sr_session_ctx_t *session, const char *xpath, sr_change_iter_t **iter);

/**
 * @brief Returns the next change from the changeset of provided iterator created
 * by ::sr_get_changes_iter call. If there is no item left, ::SR_ERR_NOT_FOUND is returned.
 *
 * @note If the operation is ::SR_OP_MOVED the meaning of new_value and old value argument is
 * as follows - the value pointed by new_value was moved after the old_value. If the
 * old value is NULL it was moved to the first position.
 *
 * @param[in] session Session provided in the callbacks (::sr_module_change_cb). Will not work with other sessions.
 * @param[in,out] iter Iterator acquired with ::sr_get_changes_iter call.
 * @param[out] operation Type of the operation made on the returned item.
 * @param[out] old_value Old value of the item (the value before the change).
 * NULL in case that the item has been just created (operation ::SR_OP_CREATED).
 * @param[out] new_value New (modified) value of the the item. NULL in case that
 * the item has been just deleted (operation ::SR_OP_DELETED).
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_change_next(sr_session_ctx_t *session, sr_change_iter_t *iter, sr_change_oper_t *operation,
        sr_val_t **old_value, sr_val_t **new_value);


////////////////////////////////////////////////////////////////////////////////
// RPC (Remote Procedure Calls) and Action API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback to be called for RPC or action specified by xpath.
 * Subscribe to it by ::sr_rpc_subscribe call.
 *
 * @param[in] xpath @ref xp_page "Data Path" identifying the RPC/action.
 * @param[in] input Array of input parameters.
 * @param[in] input_cnt Number of input parameters.
 * @param[out] output Array of output parameters. Should be allocated on heap,
 * will be freed by sysrepo after sending of the RPC response.
 * @param[out] output_cnt Number of output parameters.
 * @param[in] private_data Private context opaque to sysrepo, as passed to ::sr_rpc_subscribe call.
 * @return Error code (::SR_ERR_OK on success).
 */
typedef int (*sr_rpc_cb)(sr_session_ctx_t *session, const char *xpath, const sr_val_t *input, const size_t input_cnt,
        sr_val_t **output, size_t *output_cnt, void *private_data);

/**
 * @brief Callback to be called for RPC or action specified by xpath.
 * This operates with libyang trees rather than with sysrepo values,
 * use it with ::sr_rpc_subscribe_tree and ::sr_rpc_send_tree.
 *
 * @param[in] xpath @ref xp_page "Data Path" identifying the RPC/action.
 * @param[in] input Data tree of input parameters.
 * @param[out] output Data tree of output parameters. Should be allocated on heap,
 * will be freed by sysrepo after sending of the RPC response.
 * @param[in] private_data Private context opaque to sysrepo, as passed to ::sr_rpc_subscribe_tree call.
 * @return Error code (::SR_ERR_OK on success).
 */
typedef int (*sr_rpc_tree_cb)(sr_session_ctx_t *session, const char *xpath, const struct lyd_node *input,
        struct lyd_node *output, void *private_data);

/**
 * @brief Subscribes for delivery of RPC or action specified by xpath.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Schema Path" identifying the RPC/action.
 * @param[in] callback Callback to be called.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @note An existing context may be passed in case that ::SR_SUBSCR_CTX_REUSE option is specified.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_rpc_subscribe(sr_session_ctx_t *session, const char *xpath, sr_rpc_cb callback, void *private_data,
        sr_subscr_options_t opts, sr_subscription_ctx_t **subscription);

/**
 * @brief Subscribes for delivery of RPC or action specified by xpath. Unlike ::sr_rpc_subscribe, this
 * function expects callback of type ::sr_rpc_tree_cb, therefore use this version if you prefer
 * to manipulate with RPC/action input and output data organized in a tree rather than as a flat
 * enumeration of all values.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Schema Path" identifying the RPC/action.
 * @param[in] callback Callback to be called.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @note An existing context may be passed in case that ::SR_SUBSCR_CTX_REUSE option is specified.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_rpc_subscribe_tree(sr_session_ctx_t *session, const char *xpath, sr_rpc_tree_cb callback,
        void *private_data, sr_subscr_options_t opts, sr_subscription_ctx_t **subscription);

/**
 * @brief Sends an RPC or action specified by xpath and waits for the result.
 *
 * Required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifying the RPC/action.
 * @param[in] input Array of input parameters (array of all nodes that hold some
 * data in RPC/action input subtree - same as ::sr_get_items would return).
 * @param[in] input_cnt Number of input parameters.
 * @param[out] output Array of output parameters (all nodes that hold some data
 * in RPC/action output subtree). Will be allocated by sysrepo and should be freed by
 * caller using ::sr_free_values.
 * @param[out] output_cnt Number of output parameters.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_rpc_send(sr_session_ctx_t *session, const char *xpath, const sr_val_t *input, const size_t input_cnt,
        sr_val_t **output, size_t *output_cnt);

/**
 * @brief Sends an RPC or action specified by xpath and waits for the result. Input and output data
 * are represented as subtrees reflecting the scheme of RPC/action arguments.
 *
 * Required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] input Input data tree.
 * @param[out] output Output data tree. Will be allocated by sysrepo and should be freed by the caller.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_rpc_send_tree(sr_session_ctx_t *session, struct lyd_node *input, struct lyd_node **output);


////////////////////////////////////////////////////////////////////////////////
// Event Notifications API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Type of the notification passed to the ::sr_event_notif_cb and ::sr_event_notif_tree_cb callbacks.
 */
typedef enum sr_ev_notif_type_e {
    SR_EV_NOTIF_REALTIME,         /**< Real-time notification. */
    SR_EV_NOTIF_REPLAY,           /**< Replayed notification. */
    SR_EV_NOTIF_REPLAY_COMPLETE,  /**< Not a real notification, just a signal that the notification replay has completed
                                       (all the stored notifications from the given time interval have been delivered). */
    SR_EV_NOTIF_STOP,             /**< Not a real notification, just a signal that replay stop time has been reached
                                       (delivered only if stop_time was specified when subscribing). */
} sr_ev_notif_type_t;

/**
 * @brief Callback to be called for event notification specified by xpath.
 * Subscribe to it by ::sr_event_notif_subscribe call.
 *
 * @param[in] session Automatically-created session that can be used for learning about initiator session IDs.
 * Do not stop this session.
 * @param[in] notif_type Type of the notification.
 * @param[in] xpath @ref xp_page "Data Path" identifying the event notification.
 * @param[in] values Array of all nodes that hold some data in event notification subtree.
 * @param[in] values_cnt Number of items inside the values array.
 * @param[in] timestamp Time when the notification was generated
 * @param[in] private_data Private context opaque to sysrepo,
 * as passed to ::sr_event_notif_subscribe call.
 */
typedef void (*sr_event_notif_cb)(sr_session_ctx_t *session, const sr_ev_notif_type_t notif_type, const char *xpath,
        const sr_val_t *values, const size_t values_cnt, time_t timestamp, void *private_data);

/**
 * @brief Callback to be called for event notification specified by xpath.
 * This callback variant operates with libyang trees rather than with sysrepo values,
 * use it with ::sr_event_notif_subscribe_tree and ::sr_event_notif_send_tree.
 *
 * @param[in] session Automatically-created session that can be used for learning about initiator session IDs.
 * Do not stop this session.
 * @param[in] notif_type Type of the notification.
 * @param[in] xpath @ref xp_page "Data Path" identifying the event notification.
 * @param[in] notif Notification data tree.
 * @param[in] timestamp Time when the notification was generated
 * @param[in] private_data Private context opaque to sysrepo, as passed to ::sr_event_notif_subscribe_tree call.
 */
typedef void (*sr_event_notif_tree_cb)(sr_session_ctx_t *session, const sr_ev_notif_type_t notif_type,
        const struct lyd_node *notif, time_t timestamp, void *private_data);

/**
 * @brief Subscribes for delivery of an event notification specified by xpath.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Name of the module whose notifications to subscribe to.
 * @param[in] xpath @ref xp_page Optional "Schema Path" identifying a single event notification.
 * @param[in] start_time Optional start time of the subscription. Used for replaying stored notifications.
 * @param[in] stop_time Optional stop time ending the notification subscription.
 * @param[in] callback Callback to be called when the event notification is delivered.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @note An existing context may be passed in case that ::SR_SUBSCR_CTX_REUSE option is specified.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_event_notif_subscribe(sr_session_ctx_t *session, const char *module_name, const char *xpath, time_t start_time,
        time_t stop_time, sr_event_notif_cb callback, void *private_data, sr_subscr_options_t opts,
        sr_subscription_ctx_t **subscription);

/**
 * @brief Subscribes for delivery of event notification specified by xpath.
 * Unlike ::sr_event_notif_subscribe, this function expects callback of type ::sr_event_notif_tree_cb,
 * therefore use this version if you prefer to manipulate with event notification data organized
 * in trees rather than as a flat enumeration of all values.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Name of the module whose notifications to subscribe to.
 * @param[in] xpath @ref xp_page Optional "Schema Path" identifying a single event notification.
 * @param[in] start_time Optional start time of the subscription. Used for replaying stored notifications.
 * @param[in] stop_time Optional stop time ending the notification subscription.
 * @param[in] callback Callback to be called when the event notification is delivered.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @note An existing context may be passed in case that ::SR_SUBSCR_CTX_REUSE option is specified.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_event_notif_subscribe_tree(sr_session_ctx_t *session, const char *module_name, const char *xpath,
        time_t start_time, time_t stop_time, sr_event_notif_tree_cb callback, void *private_data,
        sr_subscr_options_t opts, sr_subscription_ctx_t **subscription);

/**
 * @brief Sends an event notification specified by xpath.
 *
 * Required WRITE access. If the module does not support replay, required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] xpath @ref xp_page "Data Path" identifying the event notification.
 * @param[in] values Array of all nodes that hold some data in event notification subtree
 * (same as ::sr_get_items would return).
 * @param[in] values_cnt Number of items inside the values array.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_event_notif_send(sr_session_ctx_t *session, const char *xpath, const sr_val_t *values, const size_t values_cnt);

/**
 * @brief Sends an event notification specified by xpath.
 * The notification data are represented as libyang data trees reflecting the scheme
 * of the event notification.
 *
 * Required WRITE access. If the module does not support replay, required READ access.
 *
 * @param[in] session Session to use.
 * @param[in] notif Notification data tree to send.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_event_notif_send_tree(sr_session_ctx_t *session, struct lyd_node *notif);


////////////////////////////////////////////////////////////////////////////////
// Operational Data API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback to be called when operational data at the selected xpath are requested.
 * Subscribe to it by ::sr_dp_get_items_subscribe call.
 *
 * Callback handler can provide any data matching the xpath but in case there are other nested subscriptions,
 * they will be called after this one.
 *
 * The xpath argument passed to callback can only be the xpath that was used for the subscription.
 *
 * @param[in] session Automatically-created session that can be used for learning about initiator session IDs.
 * Do not stop this session.
 * @param[in] module_name Name of the affected module.
 * @param[in] xpath @ref xp_page "Data Path" identifying the requested nodes.
 * @param[in,out] parent Pointer to an existing parent of the requested nodes. Is NULL for top-level nodes.
 * Called is supposed to append the requested nodes to this data subtree.
 * @param[in] private_data Private context opaque to sysrepo, as passed to ::sr_dp_get_items_subscribe call.
 * @return Error code (::SR_ERR_OK on success).
 */
typedef int (*sr_dp_get_items_cb)(sr_session_ctx_t *session, const char *module_name, const char *xpath,
        struct lyd_node **parent, void *private_data);

/**
 * @brief Register for providing operational data at the given xpath.
 *
 * Required WRITE access.
 *
 * @param[in] session Session to use.
 * @param[in] module_name Name of the affected module.
 * @param[in] xpath @ref xp_page "Data Path" identifying the subtree which the provider is able to provide.
 * @param[in] callback Callback to be called when the operational data for the given xpath are requested.
 * @param[in] private_data Private context passed to the callback function, opaque to sysrepo.
 * @param[in] opts Options overriding default behavior of the subscription, it is supposed to be
 * a bitwise OR-ed value of any ::sr_subscr_flag_t flags.
 * @param[in,out] subscription Subscription context that is supposed to be released by ::sr_unsubscribe.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_dp_get_items_subscribe(sr_session_ctx_t *session, const char *module_name, const char *xpath,
        sr_dp_get_items_cb callback, void *private_data, sr_subscr_options_t opts, sr_subscription_ctx_t **subscription);


////////////////////////////////////////////////////////////////////////////////
// Schema Manipulation API
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get the libyang context used by a connection. Can be used in an application for working with data
 * and schemas. Do NOT change this context!
 *
 * @param[in] conn Connection to use.
 * @return Const libyang context.
 */
const struct ly_ctx *sr_get_context(sr_conn_ctx_t *conn);

/**
 * @brief Install a new module into sysrepo.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_path Path to a new module. Can have either YANG or YIN extension/format.
 * @param[in] search_dir Optional search dir for import modules.
 * @param[in] features Array of enabled features.
 * @param[in] feat_count Number of enabled features.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_install_module(sr_conn_ctx_t *conn, const char *module_path, const char *search_dir, const char **features,
        int feat_count);

/**
 * @brief Remove an installed module from sysrepo. Deferred until new main SHM creation!
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the mdoule to remove.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_remove_module(sr_conn_ctx_t *conn, const char *module_name);

/**
 * @brief Update an installed module with a new revision. Deferred until new main SHM creation!
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_path Path to the updated module. Can have either YANG or YIN extension/format.
 * @param[in] search_dir Optional search dir for import modules.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_update_module(sr_conn_ctx_t *conn, const char *module_path, const char *search_dir);

/**
 * @brief Cancel scheduled update of a module.
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module whose update to cancel.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_cancel_update_module(sr_conn_ctx_t *conn, const char *module_name);

/**
 * @brief Change module replay support.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module to change.
 * @param[in] replay_support 0 to disabled, non-zero to enable.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_set_module_replay_support(sr_conn_ctx_t *conn, const char *module_name, int replay_support);

/**
 * @brief Change module filesystem permissions.
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module to change.
 * @param[in] owner If set, new owner of the module.
 * @param[in] group If set, new group of the module.
 * @param[in] perm If set, new permissions of the module.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_set_module_access(sr_conn_ctx_t *conn, const char *module_name, const char *owner, const char *group, mode_t perm);

/**
 * @brief Learn about module filesystem permissions.
 *
 * Required READ access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module to inspect.
 * @param[in,out] owner If set, read the owner of the module.
 * @param[in,out] group If set, read the group of the module.
 * @param[in,out] perm If set, read the permissions of the module.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_module_access(sr_conn_ctx_t *conn, const char *module_name, char **owner, char **group, mode_t *perm);

/**
 * @brief Enable a module feature. Deferred until new main SHM creation!
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module to change.
 * @param[in] feature_name Name of the feature to enable.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_enable_module_feature(sr_conn_ctx_t *conn, const char *module_name, const char *feature_name);

/**
 * @brief Disable a module feature. Deferred until new main SHM creation!
 *
 * Required WRITE access.
 *
 * @param[in] conn Connection to use.
 * @param[in] module_name Name of the module to change.
 * @param[in] feature_name Name of the feature to disable.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_disable_module_feature(sr_conn_ctx_t *conn, const char *module_name, const char *feature_name);

/**
 * @brief Get internal sysrepo data tree, which holds information about installed modules.
 *
 * @param[in] conn Connection to use.
 * @param[out] sysrepo_data Sysrepo internal data tree.
 * @return Error code (::SR_ERR_OK on success).
 */
int sr_get_module_info(sr_conn_ctx_t *conn, struct lyd_node **sysrepo_data);


////////////////////////////////////////////////////////////////////////////////
// Cleanup Routines
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Frees ::sr_val_t structure and all memory allocated within it.
 *
 * @param[in] value Value to be freed.
 */
void sr_free_val(sr_val_t *value);

/**
 * @brief Frees array of ::sr_val_t structures (and all memory allocated
 * within of each array element).
 *
 * @param[in] values Array of values to be freed.
 * @param[in] count Number of elements stored in the array.
 */
void sr_free_values(sr_val_t *values, size_t count);

/**
 * @brief Frees ::sr_change_iter_t iterator and all memory allocated within it.
 *
 * @param[in] iter Iterator to be freed.
 */
void sr_free_change_iter(sr_change_iter_t *iter);

/**@} cl */

#ifdef __cplusplus
}
#endif

#endif /* _SYSREPO_H */
