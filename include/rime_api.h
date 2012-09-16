/* vim: set sts=2 sw=2 et:
 * encoding: utf-8
 *
 * Copyleft 2011 RIME Developers
 * License: GPLv3
 *
 * 2011-08-08 GONG Chen <chen.sst@gmail.com>
 */
#ifndef RIME_API_H_
#define RIME_API_H_

#ifdef __cplusplus
extern "C" {
#endif

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

#define RIME_MAX_NUM_CANDIDATES 10

#define RIME_STRUCT_INIT(Type, var) ((var).data_size = sizeof(Type) - sizeof((var).data_size))
#define RIME_STRUCT_HAS_MEMBER(var, member) (sizeof((var).data_size) + (var).data_size > (char*)&member - (char*)&var)

typedef struct {
  const char* shared_data_dir;
  const char* user_data_dir;
  const char* distribution_name;
  const char* distribution_code_name;
  const char* distribution_version;
} RimeTraits;

typedef struct {
  int length;
  int cursor_pos;
  int sel_start;
  int sel_end;
  char* preedit;
} RimeComposition;

typedef struct {
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
  RimeCandidate candidates[RIME_MAX_NUM_CANDIDATES];
  char select_keys[RIME_MAX_NUM_CANDIDATES + 1];
} RimeMenu;

typedef struct {
  char* text;
} RimeCommit;

// should be initialized by calling RIME_STRUCT_INIT(Type, var);
typedef struct {
  int data_size;
  // v0.9.1
  RimeComposition composition;
  RimeMenu menu;
  // since v0.9.2
  char* commit_text_preview;
  // future technology...
} RimeContext;

// should be initialized by calling RIME_STRUCT_INIT(Type, var);
typedef struct {
  int data_size;
  char* schema_id;
  char* schema_name;
  Bool is_disabled;
  Bool is_composing;
  Bool is_ascii_mode;
  Bool is_full_shape;
  Bool is_simplified;
  // ...
} RimeStatus;

typedef struct {
  void* ptr;
} RimeConfig;

typedef struct {
  void* list;
  void* map;
  int index;
  const char* key;
  const char* path;
} RimeConfigIterator;

// entry and exit

// call this function before accessing any other API.
// pass a C-string constant in the format "rime.x"
// where 'x' is the name of your application.
// the prefix "rime." ensures old log files are automatically cleaned.
RIME_API void RimeSetupLogging(const char* app_name);

RIME_API void RimeInitialize(RimeTraits *traits);
RIME_API void RimeFinalize();

RIME_API Bool RimeStartMaintenance(Bool full_check);
RIME_API Bool RimeStartMaintenanceOnWorkspaceChange();
RIME_API Bool RimeIsMaintenancing();
RIME_API void RimeJoinMaintenanceThread();

// deployment

RIME_API void RimeDeployerInitialize(RimeTraits *traits);
RIME_API Bool RimePrebuildAllSchemas();
RIME_API Bool RimeDeployWorkspace();
RIME_API Bool RimeDeploySchema(const char *schema_file);
RIME_API Bool RimeDeployConfigFile(const char *file_name, const char *version_key);

// session management

RIME_API RimeSessionId RimeCreateSession();
RIME_API Bool RimeFindSession(RimeSessionId session_id);
RIME_API Bool RimeDestroySession(RimeSessionId session_id);
RIME_API void RimeCleanupStaleSessions();
RIME_API void RimeCleanupAllSessions();

// input

RIME_API Bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask);
// return True if there is unread commit text
RIME_API Bool RimeCommitComposition(RimeSessionId session_id);
RIME_API void RimeClearComposition(RimeSessionId session_id);

// output
  
RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit);
RIME_API Bool RimeFreeCommit(RimeCommit* commit);
RIME_API Bool RimeGetContext(RimeSessionId session_id, RimeContext* context);
RIME_API Bool RimeFreeContext(RimeContext* context);
RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status);
RIME_API Bool RimeFreeStatus(RimeStatus* status);

// runtime options
  
RIME_API void RimeSetOption(RimeSessionId session_id, const char* option, Bool value);
RIME_API Bool RimeGetOption(RimeSessionId session_id, const char* option);

  
// configuration

RIME_API Bool RimeConfigOpen(const char *config_id, RimeConfig* config);
RIME_API Bool RimeConfigClose(RimeConfig *config);
RIME_API Bool RimeConfigGetBool(RimeConfig *config, const char *key, Bool *value);
RIME_API Bool RimeConfigGetInt(RimeConfig *config, const char *key, int *value);
RIME_API Bool RimeConfigGetDouble(RimeConfig *config, const char *key, double *value);
RIME_API Bool RimeConfigGetString(RimeConfig *config, const char *key,
                                  char *value, size_t buffer_size);
RIME_API Bool RimeConfigUpdateSignature(RimeConfig* config, const char* signer);
RIME_API Bool RimeConfigBeginMap(RimeConfigIterator* iterator, RimeConfig* config, const char* key);
RIME_API Bool RimeConfigNext(RimeConfigIterator* iterator);
RIME_API void RimeConfigEnd(RimeConfigIterator* iterator);

// testing

RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence);

#ifdef __cplusplus
}
#endif

#endif  // RIME_API_H_
