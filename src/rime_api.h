/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 *
 * 2011-08-08  GONG Chen <chen.sst@gmail.com>  v0.9
 * 2013-05-02  GONG Chen <chen.sst@gmail.com>  v1.0
 */
#ifndef RIME_API_H_
#define RIME_API_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#if defined(RIME_EXPORTS)
/* DLL export */
#define RIME_DLL __declspec(dllexport)
#define RIME_API extern "C" RIME_DLL
#elif defined(RIME_IMPORTS)
/* DLL import */
#define RIME_DLL __declspec(dllimport)
#define RIME_API extern "C" RIME_DLL
#else
/* static library */
#define RIME_DLL
#define RIME_API
#endif
#else /* _WIN32 */
#define RIME_DLL
#define RIME_API
#endif /* _WIN32 */

#ifndef RIME_DEPRECATED
#define RIME_DEPRECATED RIME_API
#endif

#ifndef RIME_FLAVORED
#define RIME_FLAVORED(name) name
#endif

#ifndef Bool
#define Bool int
#endif

#ifndef False
#define False 0
#endif

#ifndef True
#define True 1
#endif

typedef uintptr_t RimeSessionId;

// Version control
#define RIME_STRUCT_INIT(Type, var) \
  ((var).data_size = sizeof(Type) - sizeof((var).data_size))
#define RIME_STRUCT_HAS_MEMBER(var, member)           \
  ((int)(sizeof((var).data_size) + (var).data_size) > \
   (char*)&member - (char*)&var)
#define RIME_STRUCT_CLEAR(var) \
  memset((char*)&(var) + sizeof((var).data_size), 0, (var).data_size)

//! Define a variable of Type
#define RIME_STRUCT(Type, var) \
  Type var = {0};              \
  RIME_STRUCT_INIT(Type, var);

// tests that member is a non-null pointer in self-versioned struct *p.
#define RIME_PROVIDED(p, member) \
  ((p) && RIME_STRUCT_HAS_MEMBER(*(p), (p)->member) && (p)->member)

//! For passing pointer to capnproto builder as opaque pointer through C API.
#define RIME_PROTO_BUILDER void

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
  // v1.6
  /*! Minimal level of logged messages.
   *  Value is passed to Glog library using FLAGS_minloglevel variable.
   *  0 = INFO (default), 1 = WARNING, 2 = ERROR, 3 = FATAL
   */
  int min_log_level;
  /*! Directory of log files.
   *  Value is passed to Glog library using FLAGS_log_dir variable.
   *  NULL means temporary directory, and "" means only writing to stderr.
   */
  const char* log_dir;
  //! prebuilt data directory. defaults to ${shared_data_dir}/build
  const char* prebuilt_data_dir;
  //! staging directory. defaults to ${user_data_dir}/build
  const char* staging_dir;
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
} RIME_FLAVORED(RimeMenu);

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
typedef struct RIME_FLAVORED(rime_context_t) {
  int data_size;
  // v0.9
  RimeComposition composition;
  RIME_FLAVORED(RimeMenu) menu;
  // v0.9.2
  char* commit_text_preview;
  char** select_labels;
} RIME_FLAVORED(RimeContext);

/*!
 *  Should be initialized by calling RIME_STRUCT_INIT(Type, var);
 */
typedef struct RIME_FLAVORED(rime_status_t) {
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
  Bool is_predicting;
} RIME_FLAVORED(RimeStatus);

typedef struct rime_candidate_list_iterator_t {
  void* ptr;
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

typedef struct rime_string_slice_t {
  const char* str;
  size_t length;
} RimeStringSlice;

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
typedef void (*RimeNotificationHandler)(void* context_object,
                                        RimeSessionId session_id,
                                        const char* message_type,
                                        const char* message_value);

/*!
 *  Extend the structure to publish custom data/functions in your specific
 * module
 */
typedef struct rime_custom_api_t {
  int data_size;
} RimeCustomApi;

typedef struct rime_module_t {
  int data_size;

  const char* module_name;
  void (*initialize)(void);
  void (*finalize)(void);
  RimeCustomApi* (*get_api)(void);
} RimeModule;

RIME_API Bool RimeRegisterModule(RimeModule* module);
RIME_API RimeModule* RimeFindModule(const char* module_name);

/*! The API structure
 *  RimeApi is for rime v1.0+
 */
typedef struct RIME_FLAVORED(rime_api_t) {
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

  void (*initialize)(RimeTraits* traits);
  void (*finalize)(void);

  Bool (*start_maintenance)(Bool full_check);
  Bool (*is_maintenance_mode)(void);
  void (*join_maintenance_thread)(void);

  // deployment

  void (*deployer_initialize)(RimeTraits* traits);
  Bool (*prebuild)(void);
  Bool (*deploy)(void);
  Bool (*deploy_schema)(const char* schema_file);
  Bool (*deploy_config_file)(const char* file_name, const char* version_key);

  Bool (*sync_user_data)(void);

  // session management

  RimeSessionId (*create_session)(void);
  Bool (*find_session)(RimeSessionId session_id);
  Bool (*destroy_session)(RimeSessionId session_id);
  void (*cleanup_stale_sessions)(void);
  void (*cleanup_all_sessions)(void);

  // input

  Bool (*process_key)(RimeSessionId session_id, int keycode, int mask);
  // return True if there is unread commit text
  Bool (*commit_composition)(RimeSessionId session_id);
  void (*clear_composition)(RimeSessionId session_id);

  // output

  Bool (*get_commit)(RimeSessionId session_id, RimeCommit* commit);
  Bool (*free_commit)(RimeCommit* commit);
  Bool (*get_context)(RimeSessionId session_id,
                      RIME_FLAVORED(RimeContext) * context);
  Bool (*free_context)(RIME_FLAVORED(RimeContext) * ctx);
  Bool (*get_status)(RimeSessionId session_id,
                     RIME_FLAVORED(RimeStatus) * status);
  Bool (*free_status)(RIME_FLAVORED(RimeStatus) * status);

  // runtime options

  void (*set_option)(RimeSessionId session_id, const char* option, Bool value);
  Bool (*get_option)(RimeSessionId session_id, const char* option);

  void (*set_property)(RimeSessionId session_id,
                       const char* prop,
                       const char* value);
  Bool (*get_property)(RimeSessionId session_id,
                       const char* prop,
                       char* value,
                       size_t buffer_size);

  Bool (*get_schema_list)(RimeSchemaList* schema_list);
  void (*free_schema_list)(RimeSchemaList* schema_list);

  Bool (*get_current_schema)(RimeSessionId session_id,
                             char* schema_id,
                             size_t buffer_size);
  Bool (*select_schema)(RimeSessionId session_id, const char* schema_id);

  // configuration

  Bool (*schema_open)(const char* schema_id, RimeConfig* config);
  Bool (*config_open)(const char* config_id, RimeConfig* config);
  Bool (*config_close)(RimeConfig* config);
  Bool (*config_get_bool)(RimeConfig* config, const char* key, Bool* value);
  Bool (*config_get_int)(RimeConfig* config, const char* key, int* value);
  Bool (*config_get_double)(RimeConfig* config, const char* key, double* value);
  Bool (*config_get_string)(RimeConfig* config,
                            const char* key,
                            char* value,
                            size_t buffer_size);
  const char* (*config_get_cstring)(RimeConfig* config, const char* key);
  Bool (*config_update_signature)(RimeConfig* config, const char* signer);
  Bool (*config_begin_map)(RimeConfigIterator* iterator,
                           RimeConfig* config,
                           const char* key);
  Bool (*config_next)(RimeConfigIterator* iterator);
  void (*config_end)(RimeConfigIterator* iterator);

  // testing

  Bool (*simulate_key_sequence)(RimeSessionId session_id,
                                const char* key_sequence);

  // module

  Bool (*register_module)(RimeModule* module);
  RimeModule* (*find_module)(const char* module_name);

  Bool (*run_task)(const char* task_name);

  //! \deprecated use get_shared_data_dir_s instead.
  const char* (*get_shared_data_dir)(void);
  //! \deprecated use get_user_data_dir_s instead.
  const char* (*get_user_data_dir)(void);
  //! \deprecated use get_sync_dir_s instead.
  const char* (*get_sync_dir)(void);

  const char* (*get_user_id)(void);
  void (*get_user_data_sync_dir)(char* dir, size_t buffer_size);

  //! initialize an empty config object
  /*!
   * should call config_close() to free the object
   */
  Bool (*config_init)(RimeConfig* config);
  //! deserialize config from a yaml string
  Bool (*config_load_string)(RimeConfig* config, const char* yaml);

  // configuration: setters
  Bool (*config_set_bool)(RimeConfig* config, const char* key, Bool value);
  Bool (*config_set_int)(RimeConfig* config, const char* key, int value);
  Bool (*config_set_double)(RimeConfig* config, const char* key, double value);
  Bool (*config_set_string)(RimeConfig* config,
                            const char* key,
                            const char* value);

  // configuration: manipulating complex structures
  Bool (*config_get_item)(RimeConfig* config,
                          const char* key,
                          RimeConfig* value);
  Bool (*config_set_item)(RimeConfig* config,
                          const char* key,
                          RimeConfig* value);
  Bool (*config_clear)(RimeConfig* config, const char* key);
  Bool (*config_create_list)(RimeConfig* config, const char* key);
  Bool (*config_create_map)(RimeConfig* config, const char* key);
  size_t (*config_list_size)(RimeConfig* config, const char* key);
  Bool (*config_begin_list)(RimeConfigIterator* iterator,
                            RimeConfig* config,
                            const char* key);

  //! get raw input
  /*!
   *  NULL is returned if session does not exist.
   *  the returned pointer to input string will become invalid upon editing.
   */
  const char* (*get_input)(RimeSessionId session_id);

  //! caret position in terms of raw input
  size_t (*get_caret_pos)(RimeSessionId session_id);

  //! select a candidate at the given index in candidate list.
  Bool (*select_candidate)(RimeSessionId session_id, size_t index);

  //! get the version of librime
  const char* (*get_version)(void);

  //! set caret position in terms of raw input
  void (*set_caret_pos)(RimeSessionId session_id, size_t caret_pos);

  //! select a candidate from current page.
  Bool (*select_candidate_on_current_page)(RimeSessionId session_id,
                                           size_t index);

  //! access candidate list.
  Bool (*candidate_list_begin)(RimeSessionId session_id,
                               RimeCandidateListIterator* iterator);
  Bool (*candidate_list_next)(RimeCandidateListIterator* iterator);
  void (*candidate_list_end)(RimeCandidateListIterator* iterator);

  //! access config files in user data directory, eg. user.yaml and
  //! installation.yaml
  Bool (*user_config_open)(const char* config_id, RimeConfig* config);

  Bool (*candidate_list_from_index)(RimeSessionId session_id,
                                    RimeCandidateListIterator* iterator,
                                    int index);

  //! prebuilt data directory.
  //! \deprecated use get_prebuilt_data_dir_s instead.
  const char* (*get_prebuilt_data_dir)(void);
  //! staging directory, stores data files deployed to a Rime client.
  //! \deprecated use get_staging_dir_s instead.
  const char* (*get_staging_dir)(void);

  //! \deprecated for capnproto API, use "proto" module from librime-proto
  //! plugin.
  void (*commit_proto)(RimeSessionId session_id,
                       RIME_PROTO_BUILDER* commit_builder);
  void (*context_proto)(RimeSessionId session_id,
                        RIME_PROTO_BUILDER* context_builder);
  void (*status_proto)(RimeSessionId session_id,
                       RIME_PROTO_BUILDER* status_builder);

  const char* (*get_state_label)(RimeSessionId session_id,
                                 const char* option_name,
                                 Bool state);

  //! delete a candidate at the given index in candidate list.
  Bool (*delete_candidate)(RimeSessionId session_id, size_t index);
  //! delete a candidate from current page.
  Bool (*delete_candidate_on_current_page)(RimeSessionId session_id,
                                           size_t index);

  RimeStringSlice (*get_state_label_abbreviated)(RimeSessionId session_id,
                                                 const char* option_name,
                                                 Bool state,
                                                 Bool abbreviated);

  Bool (*set_input)(RimeSessionId session_id, const char* input);

  void (*get_shared_data_dir_s)(char* dir, size_t buffer_size);
  void (*get_user_data_dir_s)(char* dir, size_t buffer_size);
  void (*get_prebuilt_data_dir_s)(char* dir, size_t buffer_size);
  void (*get_staging_dir_s)(char* dir, size_t buffer_size);
  void (*get_sync_dir_s)(char* dir, size_t buffer_size);

  //! highlight a selection without committing
  Bool (*highlight_candidate)(RimeSessionId session_id, size_t index);
  //! highlight a selection without committing
  Bool (*highlight_candidate_on_current_page)(RimeSessionId session_id,
                                              size_t index);

  Bool (*change_page)(RimeSessionId session_id, Bool backward);
} RIME_FLAVORED(RimeApi);

//! API entry
/*!
 *  Acquire the version controlled RimeApi structure.
 */
RIME_API RIME_FLAVORED(RimeApi) * RIME_FLAVORED(rime_get_api)(void);

//! Clients should test if an api function is available in the current version
//! before calling it.
#define RIME_API_AVAILABLE(api, func) \
  (RIME_STRUCT_HAS_MEMBER(*(api), (api)->func) && (api)->func)

// Initializer for MSVC and GCC.
// 2010 Joe Lowe. Released into the public domain.
#if defined(__GNUC__)
#define RIME_MODULE_INITIALIZER(f)                  \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define RIME_MODULE_INITIALIZER(f)                                 \
  static void __cdecl f(void);                                     \
  __declspec(allocate(".CRT$XCU")) void(__cdecl * f##_)(void) = f; \
  static void __cdecl f(void)
#endif

/*!
 *  Automatically register a rime module when the library is loaded.
 *  Clients should define functions called rime_<module_name>_initialize(),
 *  and rime_<module_name>_finalize().
 *  \sa core_module.cc for an example.
 */
#define RIME_REGISTER_MODULE(name)                       \
  void rime_require_module_##name() {}                   \
  RIME_MODULE_INITIALIZER(rime_register_module_##name) { \
    static RimeModule module = {0};                      \
    if (!module.data_size) {                             \
      RIME_STRUCT_INIT(RimeModule, module);              \
      module.module_name = #name;                        \
      module.initialize = rime_##name##_initialize;      \
      module.finalize = rime_##name##_finalize;          \
    }                                                    \
    RimeRegisterModule(&module);                         \
  }

/*!
 *  Customize the module by assigning additional functions, eg. module->get_api.
 */
#define RIME_REGISTER_CUSTOM_MODULE(name)                       \
  void rime_require_module_##name() {}                          \
  static void rime_customize_module_##name(RimeModule* module); \
  RIME_MODULE_INITIALIZER(rime_register_module_##name) {        \
    static RimeModule module = {0};                             \
    if (!module.data_size) {                                    \
      RIME_STRUCT_INIT(RimeModule, module);                     \
      module.module_name = #name;                               \
      module.initialize = rime_##name##_initialize;             \
      module.finalize = rime_##name##_finalize;                 \
      rime_customize_module_##name(&module);                    \
    }                                                           \
    RimeRegisterModule(&module);                                \
  }                                                             \
  static void rime_customize_module_##name(RimeModule* module)

/*!
 *  Defines a constant for a list of module names.
 */
#define RIME_MODULE_LIST(var, ...) const char* var[] = {__VA_ARGS__, NULL}

/*!
 *  Register a phony module which, when loaded, will load a list of modules.
 *  \sa setup.cc for an example.
 */
#define RIME_REGISTER_MODULE_GROUP(name, ...)                       \
  static RIME_MODULE_LIST(rime_##name##_module_group, __VA_ARGS__); \
  static void rime_##name##_initialize() {                          \
    rime::LoadModules(rime_##name##_module_group);                  \
  }                                                                 \
  static void rime_##name##_finalize() {}                           \
  RIME_REGISTER_MODULE(name)

#ifdef __cplusplus
}
#endif

#endif  // RIME_API_H_
