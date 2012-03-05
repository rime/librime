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

#define RIME_TEXT_MAX_LENGTH 255
#define RIME_SCHEMA_MAX_LENGTH 127
#define RIME_MAX_NUM_CANDIDATES 10

typedef uintptr_t RimeSessionId;

typedef int Bool;

#ifndef False
#define False 0
#endif
#ifndef True
#define True 1
#endif

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
  char preedit[RIME_TEXT_MAX_LENGTH + 1];
} RimeComposition;

typedef struct {
  int page_size;
  int page_no;
  Bool is_last_page;
  int highlighted_candidate_index;
  int num_candidates;
  char candidates[RIME_MAX_NUM_CANDIDATES][RIME_TEXT_MAX_LENGTH + 1];
  char select_keys[RIME_MAX_NUM_CANDIDATES + 1];
} RimeMenu;

typedef struct {
  RimeComposition composition;
  RimeMenu menu;
} RimeContext;

typedef struct {
  char text[RIME_TEXT_MAX_LENGTH + 1];
} RimeCommit;

typedef struct {
  char schema_id[RIME_SCHEMA_MAX_LENGTH + 1];
  char schema_name[RIME_SCHEMA_MAX_LENGTH + 1];
  Bool is_disabled;
  Bool is_composing;
  Bool is_ascii_mode;
  Bool is_full_shape;
  Bool is_simplified;
} RimeStatus;

typedef struct {
  void *ptr;
} RimeConfig;

// library entry and exit

RIME_API void RimeInitialize(RimeTraits *traits);
RIME_API void RimeFinalize();

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

// using sessions

RIME_API Bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask);

RIME_API Bool RimeGetContext(RimeSessionId session_id, RimeContext* context);
RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit);
RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status);

// configuration

RIME_API Bool RimeConfigOpen(const char *config_id, RimeConfig* config);
RIME_API Bool RimeConfigClose(RimeConfig *config);
RIME_API Bool RimeConfigGetBool(RimeConfig *config, const char *key, Bool *value);
RIME_API Bool RimeConfigGetInt(RimeConfig *config, const char *key, int *value);
RIME_API Bool RimeConfigGetDouble(RimeConfig *config, const char *key, double *value);
RIME_API Bool RimeConfigGetString(RimeConfig *config, const char *key,
                                  char *value, size_t buffer_size);

// for testing

RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence);

#ifdef __cplusplus
}
#endif

#endif  // RIME_API_H_
