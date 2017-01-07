/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2011-08-08  GONG Chen <chen.sst@gmail.com>  v0.9
 * 2013-05-02  GONG Chen <chen.sst@gmail.com>  v1.0
 */
#ifndef RIME_API_H_
#define RIME_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(RIME_EXPORTS)
/* DLL export */
#define RIME_API __declspec(dllexport)
#elif defined(RIME_IMPORTS)
/* DLL import */
#define RIME_API __declspec(dllimport)
#else
/* static library */
#define RIME_API
#endif
#else  /* _WIN32 */
#define RIME_API
#endif  /* _WIN32 */

typedef uintptr_t RimeSessionId;

typedef int Bool;

#ifndef False
#define False 0
#endif
#ifndef True
#define True 1
#endif

//! Define the max number of candidates
/*!
 *  \deprecated There is no limit to the number of candidates in RimeMenu
 */
#define RIME_MAX_NUM_CANDIDATES 10

// Version control
#define RIME_STRUCT_INIT(Type, var) ((var).data_size = sizeof(Type) - sizeof((var).data_size))
#define RIME_STRUCT_HAS_MEMBER(var, member) ((int)(sizeof((var).data_size) + (var).data_size) > (char*)&member - (char*)&var)
#define RIME_STRUCT_CLEAR(var) memset((char*)&(var) + sizeof((var).data_size), 0, (var).data_size)

//! Define a variable of Type
#define RIME_STRUCT(Type, var)  Type var = {0}; RIME_STRUCT_INIT(Type, var);

//! Rime traits structure
/*!
 *  Should be initialized by calling RIME_STRUCT_INIT(Type, var)
 */
typedef struct rime_traits_t {
  int data_size;
  // v0.9
  const char* shared_data_dir;
  const char* user_data_dir;
  const char* distribution_name;
  const char* distribution_code_name;
  const char* distribution_version;
  // v1.0
  /*!
   * Pass a C-string constant in the format "rime.x"
   * where 'x' is the name of your application.
   * Add prefix "rime." to ensure old log files are automatically cleaned.
   */
  const char* app_name;

  //! A list of modules to load before initializing
  const char** modules;
} RimeTraits;

typedef struct {
  int length;
  int cursor_pos;
  int sel_start;
  int sel_end;
  char* preedit;
} RimeComposition;

typedef struct rime_candidate_t {
  char* text;
  char* comment;
  void* reserved;
} RimeCandidate;

typedef struct {
  int page_size;
  int page_no;
  Bool is_last_page;
  int highlighted_candidate_index;
  int num_candidates;
  RimeCandidate* candidates;
  char* select_keys;
} RimeMenu;

/*!
 *  Should be initialized by calling RIME_STRUCT_INIT(Type, var);
 */
typedef struct rime_commit_t {
  int data_size;
  // v0.9
  char* text;
} RimeCommit;

/*!
 *  Should be initialized by calling RIME_STRUCT_INIT(Type, var);
 */
typedef struct rime_context_t {
  int data_size;
  // v0.9
  RimeComposition composition;
  RimeMenu menu;
  // v0.9.2
  char* commit_text_preview;
  char** select_labels;
} RimeContext;

/*!
 *  Should be initialized by calling RIME_STRUCT_INIT(Type, var);
 */
typedef struct rime_status_t {
  int data_size;
  // v0.9
  char* schema_id;
  char* schema_name;
  Bool is_disabled;
  Bool is_composing;
  Bool is_ascii_mode;
  Bool is_full_shape;
  Bool is_simplified;
  Bool is_traditional;
  Bool is_ascii_punct;
} RimeStatus;

typedef struct rime_candidate_list_iterator_t {
  void *ptr;
  int index;
  RimeCandidate candidate;
} RimeCandidateListIterator;

typedef struct rime_config_t {
  void* ptr;
} RimeConfig;

typedef struct rime_config_iterator_t {
  void* list;
  void* map;
  int index;
  const char* key;
  const char* path;
} RimeConfigIterator;

typedef struct rime_schema_list_item_t {
  char* schema_id;
  char* name;
  void* reserved;
} RimeSchemaListItem;

typedef struct rime_schema_list_t {
  size_t size;
  RimeSchemaListItem* list;
} RimeSchemaList;

typedef void (*RimeNotificationHandler)(void* context_object,
                                        RimeSessionId session_id,
                                        const char* message_type,
                                        const char* message_value);

// Setup

/*!
 *  Call this function before accessing any other API.
 */
RIME_API void RimeSetup(RimeTraits *traits);

/*!
 *  Pass a C-string constant in the format "rime.x"
 *  where 'x' is the name of your application.
 *  Add prefix "rime." to ensure old log files are automatically cleaned.
 *  \deprecated Use RimeSetup() instead.
 */
RIME_API void RimeSetupLogging(const char* app_name);

//! Receive notifications
/*!
 * - on loading schema:
 *   + message_type="schema", message_value="luna_pinyin/Luna Pinyin"
 * - on changing mode:
 *   + message_type="option", message_value="ascii_mode"
 *   + message_type="option", message_value="!ascii_mode"
 * - on deployment:
 *   + session_id = 0, message_type="deploy", message_value="start"
 *   + session_id = 0, message_type="deploy", message_value="success"
 *   + session_id = 0, message_type="deploy", message_value="failure"
 *
 *   handler will be called with context_object as the first parameter
 *   every time an event occurs in librime, until RimeFinalize() is called.
 *   when handler is NULL, notification is disabled.
 */
RIME_API void RimeSetNotificationHandler(RimeNotificationHandler handler,
                                         void* context_object);

// Entry and exit

RIME_API void RimeInitialize(RimeTraits *traits);
RIME_API void RimeFinalize();

RIME_API Bool RimeStartMaintenance(Bool full_check);

//! \deprecated Use RimeStartMaintenance(full_check = False) instead.
RIME_API Bool RimeStartMaintenanceOnWorkspaceChange();
RIME_API Bool RimeIsMaintenancing();
RIME_API void RimeJoinMaintenanceThread();

// Deployment

RIME_API void RimeDeployerInitialize(RimeTraits *traits);
RIME_API Bool RimePrebuildAllSchemas();
RIME_API Bool RimeDeployWorkspace();
RIME_API Bool RimeDeploySchema(const char *schema_file);
RIME_API Bool RimeDeployConfigFile(const char *file_name, const char *version_key);

RIME_API Bool RimeSyncUserData();

// Session management

RIME_API RimeSessionId RimeCreateSession();
RIME_API Bool RimeFindSession(RimeSessionId session_id);
RIME_API Bool RimeDestroySession(RimeSessionId session_id);
RIME_API void RimeCleanupStaleSessions();
RIME_API void RimeCleanupAllSessions();

// Input

RIME_API Bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask);
/*!
 * return True if there is unread commit text
 */
RIME_API Bool RimeCommitComposition(RimeSessionId session_id);
RIME_API void RimeClearComposition(RimeSessionId session_id);

// Output

RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit);
RIME_API Bool RimeFreeCommit(RimeCommit* commit);
RIME_API Bool RimeGetContext(RimeSessionId session_id, RimeContext* context);
RIME_API Bool RimeFreeContext(RimeContext* context);
RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status);
RIME_API Bool RimeFreeStatus(RimeStatus* status);

// Accessing candidate list

RIME_API Bool RimeCandidateListBegin(RimeSessionId session_id, RimeCandidateListIterator* iterator);
RIME_API Bool RimeCandidateListNext(RimeCandidateListIterator* iterator);
RIME_API void RimeCandidateListEnd(RimeCandidateListIterator* iterator);

// Runtime options

RIME_API void RimeSetOption(RimeSessionId session_id, const char* option, Bool value);
RIME_API Bool RimeGetOption(RimeSessionId session_id, const char* option);

RIME_API void RimeSetProperty(RimeSessionId session_id, const char* prop, const char* value);
RIME_API Bool RimeGetProperty(RimeSessionId session_id, const char* prop, char* value, size_t buffer_size);

RIME_API Bool RimeGetSchemaList(RimeSchemaList* schema_list);
RIME_API void RimeFreeSchemaList(RimeSchemaList* schema_list);
RIME_API Bool RimeGetCurrentSchema(RimeSessionId session_id, char* schema_id, size_t buffer_size);
RIME_API Bool RimeSelectSchema(RimeSessionId session_id, const char* schema_id);

// Configuration

// <schema_id>.schema.yaml
RIME_API Bool RimeSchemaOpen(const char* schema_id, RimeConfig* config);
// <config_id>.yaml
RIME_API Bool RimeConfigOpen(const char* config_id, RimeConfig* config);
RIME_API Bool RimeConfigClose(RimeConfig* config);
RIME_API Bool RimeConfigInit(RimeConfig* config);
RIME_API Bool RimeConfigLoadString(RimeConfig* config, const char* yaml);
// Access config values
RIME_API Bool RimeConfigGetBool(RimeConfig *config, const char *key, Bool *value);
RIME_API Bool RimeConfigGetInt(RimeConfig *config, const char *key, int *value);
RIME_API Bool RimeConfigGetDouble(RimeConfig *config, const char *key, double *value);
RIME_API Bool RimeConfigGetString(RimeConfig *config, const char *key,
                                  char *value, size_t buffer_size);
RIME_API const char* RimeConfigGetCString(RimeConfig *config, const char *key);
RIME_API Bool RimeConfigSetBool(RimeConfig *config, const char *key, Bool value);
RIME_API Bool RimeConfigSetInt(RimeConfig *config, const char *key, int value);
RIME_API Bool RimeConfigSetDouble(RimeConfig *config, const char *key, double value);
RIME_API Bool RimeConfigSetString(RimeConfig *config, const char *key, const char *value);
// Manipulate complex structures
RIME_API Bool RimeConfigGetItem(RimeConfig* config, const char* key, RimeConfig* value);
RIME_API Bool RimeConfigSetItem(RimeConfig* config, const char* key, RimeConfig* value);
RIME_API Bool RimeConfigClear(RimeConfig* config, const char* key);
RIME_API Bool RimeConfigCreateList(RimeConfig* config, const char* key);
RIME_API Bool RimeConfigCreateMap(RimeConfig* config, const char* key);
RIME_API size_t RimeConfigListSize(RimeConfig* config, const char* key);
RIME_API Bool RimeConfigBeginList(RimeConfigIterator* iterator, RimeConfig* config, const char* key);
RIME_API Bool RimeConfigBeginMap(RimeConfigIterator* iterator, RimeConfig* config, const char* key);
RIME_API Bool RimeConfigNext(RimeConfigIterator* iterator);
RIME_API void RimeConfigEnd(RimeConfigIterator* iterator);
// Utilities
RIME_API Bool RimeConfigUpdateSignature(RimeConfig* config, const char* signer);

// Testing

RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence);

// Module

/*!
 *  Extend the structure to publish custom data/functions in your specific module
 */
typedef struct rime_custom_api_t {
  int data_size;
} RimeCustomApi;

typedef struct rime_module_t {
  int data_size;

  const char* module_name;
  void (*initialize)();
  void (*finalize)();
  RimeCustomApi* (*get_api)();
} RimeModule;

RIME_API Bool RimeRegisterModule(RimeModule* module);
RIME_API RimeModule* RimeFindModule(const char* module_name);

//! Run a registered task
RIME_API Bool RimeRunTask(const char* task_name);

RIME_API const char* RimeGetSharedDataDir();
RIME_API const char* RimeGetUserDataDir();
RIME_API const char* RimeGetSyncDir();
RIME_API const char* RimeGetUserId();

/*! The API structure
 *  RimeApi is for rime v1.0+
 */
typedef struct rime_api_t {
  int data_size;

  /*! setup
   *  Call this function before accessing any other API functions.
   */
  void (*setup)(RimeTraits* traits);

  /*! Set up the notification callbacks
   *  Receive notifications
   *  - on loading schema:
   *    + message_type="schema", message_value="luna_pinyin/Luna Pinyin"
   *  - on changing mode:
   *    + message_type="option", message_value="ascii_mode"
   *    + message_type="option", message_value="!ascii_mode"
   *  - on deployment:
   *    + session_id = 0, message_type="deploy", message_value="start"
   *    + session_id = 0, message_type="deploy", message_value="success"
   *    + session_id = 0, message_type="deploy", message_value="failure"
   *
   *  handler will be called with context_object as the first parameter
   *  every time an event occurs in librime, until RimeFinalize() is called.
   *  when handler is NULL, notification is disabled.
   */
  void (*set_notification_handler)(RimeNotificationHandler handler,
                                   void* context_object);

  // entry and exit

  void (*initialize)(RimeTraits *traits);
  void (*finalize)();

  Bool (*start_maintenance)(Bool full_check);
  Bool (*is_maintenance_mode)();
  void (*join_maintenance_thread)();

  // deployment

  void (*deployer_initialize)(RimeTraits *traits);
  Bool (*prebuild)();
  Bool (*deploy)();
  Bool (*deploy_schema)(const char *schema_file);
  Bool (*deploy_config_file)(const char *file_name, const char *version_key);

  Bool (*sync_user_data)();

  // session management

  RimeSessionId (*create_session)();
  Bool (*find_session)(RimeSessionId session_id);
  Bool (*destroy_session)(RimeSessionId session_id);
  void (*cleanup_stale_sessions)();
  void (*cleanup_all_sessions)();

  // input

  Bool (*process_key)(RimeSessionId session_id, int keycode, int mask);
  // return True if there is unread commit text
  Bool (*commit_composition)(RimeSessionId session_id);
  void (*clear_composition)(RimeSessionId session_id);

  // output

  Bool (*get_commit)(RimeSessionId session_id, RimeCommit* commit);
  Bool (*free_commit)(RimeCommit* commit);
  Bool (*get_context)(RimeSessionId session_id, RimeContext* context);
  Bool (*free_context)(RimeContext* ctx);
  Bool (*get_status)(RimeSessionId session_id, RimeStatus* status);
  Bool (*free_status)(RimeStatus* status);

  // runtime options

  void (*set_option)(RimeSessionId session_id, const char* option, Bool value);
  Bool (*get_option)(RimeSessionId session_id, const char* option);

  void (*set_property)(RimeSessionId session_id, const char* prop, const char* value);
  Bool (*get_property)(RimeSessionId session_id, const char* prop, char* value, size_t buffer_size);

  Bool (*get_schema_list)(RimeSchemaList* schema_list);
  void (*free_schema_list)(RimeSchemaList* schema_list);

  Bool (*get_current_schema)(RimeSessionId session_id, char* schema_id, size_t buffer_size);
  Bool (*select_schema)(RimeSessionId session_id, const char* schema_id);

  // configuration

  Bool (*schema_open)(const char *schema_id, RimeConfig* config);
  Bool (*config_open)(const char *config_id, RimeConfig* config);
  Bool (*config_close)(RimeConfig *config);
  Bool (*config_get_bool)(RimeConfig *config, const char *key, Bool *value);
  Bool (*config_get_int)(RimeConfig *config, const char *key, int *value);
  Bool (*config_get_double)(RimeConfig *config, const char *key, double *value);
  Bool (*config_get_string)(RimeConfig *config, const char *key,
                            char *value, size_t buffer_size);
  const char* (*config_get_cstring)(RimeConfig *config, const char *key);
  Bool (*config_update_signature)(RimeConfig* config, const char* signer);
  Bool (*config_begin_map)(RimeConfigIterator* iterator, RimeConfig* config, const char* key);
  Bool (*config_next)(RimeConfigIterator* iterator);
  void (*config_end)(RimeConfigIterator* iterator);

  // testing

  Bool (*simulate_key_sequence)(RimeSessionId session_id, const char *key_sequence);

  // module

  Bool (*register_module)(RimeModule* module);
  RimeModule* (*find_module)(const char* module_name);

  Bool (*run_task)(const char* task_name);
  const char* (*get_shared_data_dir)();
  const char* (*get_user_data_dir)();
  const char* (*get_sync_dir)();
  const char* (*get_user_id)();
  void (*get_user_data_sync_dir)(char* dir, size_t buffer_size);

  //! initialize an empty config object
  /*!
   * should call config_close() to free the object
   */
  Bool (*config_init)(RimeConfig* config);
  //! deserialize config from a yaml string
  Bool (*config_load_string)(RimeConfig* config, const char* yaml);

  // configuration: setters
  Bool (*config_set_bool)(RimeConfig *config, const char *key, Bool value);
  Bool (*config_set_int)(RimeConfig *config, const char *key, int value);
  Bool (*config_set_double)(RimeConfig *config, const char *key, double value);
  Bool (*config_set_string)(RimeConfig *config, const char *key, const char *value);

  // configuration: manipulating complex structures
  Bool (*config_get_item)(RimeConfig* config, const char* key, RimeConfig* value);
  Bool (*config_set_item)(RimeConfig* config, const char* key, RimeConfig* value);
  Bool (*config_clear)(RimeConfig* config, const char* key);
  Bool (*config_create_list)(RimeConfig* config, const char* key);
  Bool (*config_create_map)(RimeConfig* config, const char* key);
  size_t (*config_list_size)(RimeConfig* config, const char* key);
  Bool (*config_begin_list)(RimeConfigIterator* iterator, RimeConfig* config, const char* key);

  //! get raw input
  /*!
   *  NULL is returned if session does not exist.
   *  the returned pointer to input string will become invalid upon editing.
   */
  const char* (*get_input)(RimeSessionId session_id);

  //! caret posistion in terms of raw input
  size_t (*get_caret_pos)(RimeSessionId session_id);

  //! select a candidate at the given index in candidate list.
  Bool (*select_candidate)(RimeSessionId session_id, size_t index);

  //! get the version of librime
  const char* (*get_version)();

  //! set caret posistion in terms of raw input
  void (*set_caret_pos)(RimeSessionId session_id, size_t caret_pos);

  //! select a candidate from current page.
  Bool (*select_candidate_on_current_page)(RimeSessionId session_id, size_t index);

  // access candidate list.
  Bool (*candidate_list_begin)(RimeSessionId session_id, RimeCandidateListIterator* iterator);
  Bool (*candidate_list_next)(RimeCandidateListIterator* iterator);
  void (*candidate_list_end)(RimeCandidateListIterator* iterator);

} RimeApi;

//! API entry
/*!
 *  Acquire the version controlled RimeApi structure.
 */
RIME_API RimeApi* rime_get_api();

//! Clients should test if an api function is available in the current version before calling it.
#define RIME_API_AVAILABLE(api, func) (RIME_STRUCT_HAS_MEMBER(*(api), (api)->func) && (api)->func)

// Initializer for MSVC and GCC.
// 2010 Joe Lowe. Released into the public domain.
#if defined(__GNUC__)
#define RIME_MODULE_INITIALIZER(f) \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
#define RIME_MODULE_INITIALIZER(f) \
  static void __cdecl f(void); \
  __declspec(allocate(".CRT$XCU")) void (__cdecl*f##_)(void) = f; \
  static void __cdecl f(void)
#endif

/*!
 *  Automatically register a rime module when the library is loaded.
 *  Clients should define functions called rime_<module_name>_initialize(),
 *  and rime_<module_name>_finalize().
 *  \sa core_module.cc for an example.
 */
#define RIME_REGISTER_MODULE(name) \
void rime_require_module_##name() {} \
RIME_MODULE_INITIALIZER(rime_register_module_##name) { \
  static RimeModule module = {0}; \
  if (!module.data_size) { \
    RIME_STRUCT_INIT(RimeModule, module); \
    module.module_name = #name; \
    module.initialize = rime_##name##_initialize; \
    module.finalize = rime_##name##_finalize; \
  } \
  rime_get_api()->register_module(&module); \
}

/*!
 *  Customize the module by assigning additional functions, eg. module->get_api.
 */
#define RIME_REGISTER_CUSTOM_MODULE(name) \
void rime_require_module_##name() {} \
static void rime_customize_module_##name(RimeModule* module); \
RIME_MODULE_INITIALIZER(rime_register_module_##name) { \
  static RimeModule module = {0}; \
  if (!module.data_size) { \
    RIME_STRUCT_INIT(RimeModule, module); \
    module.module_name = #name; \
    module.initialize = rime_##name##_initialize; \
    module.finalize = rime_##name##_finalize; \
    rime_customize_module_##name(&module); \
  } \
  rime_get_api()->register_module(&module); \
} \
static void rime_customize_module_##name(RimeModule* module)

/*!
 *  Defines a constant for a list of module names.
 */
#define RIME_MODULE_LIST(var, ...) \
const char* var[] = { \
  __VA_ARGS__, NULL \
} \

/*!
 *  Register a phony module which, when loaded, will load a list of modules.
 *  \sa setup.cc for an example.
 */
#define RIME_REGISTER_MODULE_GROUP(name, ...) \
static RIME_MODULE_LIST(rime_##name##_module_group, __VA_ARGS__); \
static void rime_##name##_initialize() { \
  rime::LoadModules(rime_##name##_module_group); \
} \
static void rime_##name##_finalize() { \
} \
RIME_REGISTER_MODULE(name)

#ifdef __cplusplus
}
#endif

#endif  // RIME_API_H_
