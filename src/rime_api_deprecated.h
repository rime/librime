#ifndef RIME_API_DEPRECATED_H_
#define RIME_API_DEPRECATED_H_

#include "rime_api.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Define the max number of candidates
/*!
 *  \deprecated There is no limit to the number of candidates in RimeMenu
 */
#define RIME_MAX_NUM_CANDIDATES 10

// Setup

/*!
 *  Call this function before accessing any other API.
 */
RIME_DEPRECATED void RimeSetup(RimeTraits* traits);

/*!
 *  Pass a C-string constant in the format "rime.x"
 *  where 'x' is the name of your application.
 *  Add prefix "rime." to ensure old log files are automatically cleaned.
 *  \deprecated Use RimeSetup() instead.
 */
RIME_DEPRECATED void RimeSetupLogging(const char* app_name);

//! Receive notifications
RIME_DEPRECATED void RimeSetNotificationHandler(RimeNotificationHandler handler,
                                                void* context_object);

// Entry and exit

RIME_DEPRECATED void RimeInitialize(RimeTraits* traits);
RIME_DEPRECATED void RimeFinalize(void);

RIME_DEPRECATED Bool RimeStartMaintenance(Bool full_check);

//! \deprecated Use RimeStartMaintenance(full_check = False) instead.
RIME_DEPRECATED Bool RimeStartMaintenanceOnWorkspaceChange(void);
RIME_DEPRECATED Bool RimeIsMaintenancing(void);
RIME_DEPRECATED void RimeJoinMaintenanceThread(void);

// Deployment

RIME_DEPRECATED void RimeDeployerInitialize(RimeTraits* traits);
RIME_DEPRECATED Bool RimePrebuildAllSchemas(void);
RIME_DEPRECATED Bool RimeDeployWorkspace(void);
RIME_DEPRECATED Bool RimeDeploySchema(const char* schema_file);
RIME_DEPRECATED Bool RimeDeployConfigFile(const char* file_name,
                                          const char* version_key);

RIME_DEPRECATED Bool RimeSyncUserData(void);

// Session management

RIME_DEPRECATED RimeSessionId RimeCreateSession(void);
RIME_DEPRECATED Bool RimeFindSession(RimeSessionId session_id);
RIME_DEPRECATED Bool RimeDestroySession(RimeSessionId session_id);
RIME_DEPRECATED void RimeCleanupStaleSessions(void);
RIME_DEPRECATED void RimeCleanupAllSessions(void);

// Input

RIME_DEPRECATED Bool RimeProcessKey(RimeSessionId session_id,
                                    int keycode,
                                    int mask);
/*!
 * return True if there is unread commit text
 */
RIME_DEPRECATED Bool RimeCommitComposition(RimeSessionId session_id);
RIME_DEPRECATED void RimeClearComposition(RimeSessionId session_id);

// Output

RIME_DEPRECATED Bool RimeGetCommit(RimeSessionId session_id,
                                   RimeCommit* commit);
RIME_DEPRECATED Bool RimeFreeCommit(RimeCommit* commit);
RIME_DEPRECATED Bool RimeGetContext(RimeSessionId session_id,
                                    RIME_FLAVORED(RimeContext) * context);
RIME_DEPRECATED Bool RimeFreeContext(RIME_FLAVORED(RimeContext) * context);
RIME_DEPRECATED Bool RimeGetStatus(RimeSessionId session_id,
                                   RIME_FLAVORED(RimeStatus) * status);
RIME_DEPRECATED Bool RimeFreeStatus(RIME_FLAVORED(RimeStatus) * status);

// Accessing candidate list
RIME_DEPRECATED Bool
RimeCandidateListBegin(RimeSessionId session_id,
                       RimeCandidateListIterator* iterator);
RIME_DEPRECATED Bool RimeCandidateListNext(RimeCandidateListIterator* iterator);
RIME_DEPRECATED void RimeCandidateListEnd(RimeCandidateListIterator* iterator);
RIME_DEPRECATED Bool
RimeCandidateListFromIndex(RimeSessionId session_id,
                           RimeCandidateListIterator* iterator,
                           int index);
RIME_DEPRECATED Bool RimeSelectCandidate(RimeSessionId session_id,
                                         size_t index);
RIME_DEPRECATED Bool RimeSelectCandidateOnCurrentPage(RimeSessionId session_id,
                                                      size_t index);
RIME_DEPRECATED Bool RimeDeleteCandidate(RimeSessionId session_id,
                                         size_t index);
RIME_DEPRECATED Bool RimeDeleteCandidateOnCurrentPage(RimeSessionId session_id,
                                                      size_t index);

// Runtime options

RIME_DEPRECATED void RimeSetOption(RimeSessionId session_id,
                                   const char* option,
                                   Bool value);
RIME_DEPRECATED Bool RimeGetOption(RimeSessionId session_id,
                                   const char* option);

RIME_DEPRECATED void RimeSetProperty(RimeSessionId session_id,
                                     const char* prop,
                                     const char* value);
RIME_DEPRECATED Bool RimeGetProperty(RimeSessionId session_id,
                                     const char* prop,
                                     char* value,
                                     size_t buffer_size);

RIME_DEPRECATED Bool RimeGetSchemaList(RimeSchemaList* schema_list);
RIME_DEPRECATED void RimeFreeSchemaList(RimeSchemaList* schema_list);
RIME_DEPRECATED Bool RimeGetCurrentSchema(RimeSessionId session_id,
                                          char* schema_id,
                                          size_t buffer_size);
RIME_DEPRECATED Bool RimeSelectSchema(RimeSessionId session_id,
                                      const char* schema_id);

// Configuration

// <schema_id>.schema.yaml
RIME_DEPRECATED Bool RimeSchemaOpen(const char* schema_id, RimeConfig* config);
// <config_id>.yaml
RIME_DEPRECATED Bool RimeConfigOpen(const char* config_id, RimeConfig* config);
// access config files in user data directory, eg. user.yaml and
// installation.yaml
RIME_DEPRECATED Bool RimeUserConfigOpen(const char* config_id,
                                        RimeConfig* config);
RIME_DEPRECATED Bool RimeConfigClose(RimeConfig* config);
RIME_DEPRECATED Bool RimeConfigInit(RimeConfig* config);
RIME_DEPRECATED Bool RimeConfigLoadString(RimeConfig* config, const char* yaml);
// Access config values
RIME_DEPRECATED Bool RimeConfigGetBool(RimeConfig* config,
                                       const char* key,
                                       Bool* value);
RIME_DEPRECATED Bool RimeConfigGetInt(RimeConfig* config,
                                      const char* key,
                                      int* value);
RIME_DEPRECATED Bool RimeConfigGetDouble(RimeConfig* config,
                                         const char* key,
                                         double* value);
RIME_DEPRECATED Bool RimeConfigGetString(RimeConfig* config,
                                         const char* key,
                                         char* value,
                                         size_t buffer_size);
RIME_DEPRECATED const char* RimeConfigGetCString(RimeConfig* config,
                                                 const char* key);
RIME_DEPRECATED Bool RimeConfigSetBool(RimeConfig* config,
                                       const char* key,
                                       Bool value);
RIME_DEPRECATED Bool RimeConfigSetInt(RimeConfig* config,
                                      const char* key,
                                      int value);
RIME_DEPRECATED Bool RimeConfigSetDouble(RimeConfig* config,
                                         const char* key,
                                         double value);
RIME_DEPRECATED Bool RimeConfigSetString(RimeConfig* config,
                                         const char* key,
                                         const char* value);
// Manipulate complex structures
RIME_DEPRECATED Bool RimeConfigGetItem(RimeConfig* config,
                                       const char* key,
                                       RimeConfig* value);
RIME_DEPRECATED Bool RimeConfigSetItem(RimeConfig* config,
                                       const char* key,
                                       RimeConfig* value);
RIME_DEPRECATED Bool RimeConfigClear(RimeConfig* config, const char* key);
RIME_DEPRECATED Bool RimeConfigCreateList(RimeConfig* config, const char* key);
RIME_DEPRECATED Bool RimeConfigCreateMap(RimeConfig* config, const char* key);
RIME_DEPRECATED size_t RimeConfigListSize(RimeConfig* config, const char* key);
RIME_DEPRECATED Bool RimeConfigBeginList(RimeConfigIterator* iterator,
                                         RimeConfig* config,
                                         const char* key);
RIME_DEPRECATED Bool RimeConfigBeginMap(RimeConfigIterator* iterator,
                                        RimeConfig* config,
                                        const char* key);
RIME_DEPRECATED Bool RimeConfigNext(RimeConfigIterator* iterator);
RIME_DEPRECATED void RimeConfigEnd(RimeConfigIterator* iterator);
// Utilities
RIME_DEPRECATED Bool RimeConfigUpdateSignature(RimeConfig* config,
                                               const char* signer);

// Testing

RIME_DEPRECATED Bool RimeSimulateKeySequence(RimeSessionId session_id,
                                             const char* key_sequence);

RIME_DEPRECATED Bool RimeSetInput(RimeSessionId session_id, const char* input);

//! Run a registered task
RIME_DEPRECATED Bool RimeRunTask(const char* task_name);

//! \deprecated use RimeApi::get_shared_data_dir_s instead.
RIME_DEPRECATED const char* RimeGetSharedDataDir(void);
//! \deprecated use RimeApi::get_user_data_dir_s instead.
RIME_DEPRECATED const char* RimeGetUserDataDir(void);
//! \deprecated use RimeApi::get_sync_dir_s instead.
RIME_DEPRECATED const char* RimeGetSyncDir(void);

RIME_DEPRECATED const char* RimeGetUserId(void);

#ifdef __cplusplus
}
#endif

#endif  // RIME_API_DEPRECATED_H_
