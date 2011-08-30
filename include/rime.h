/* vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
*/
#ifndef RIME_H_
#define RIME_H_

#include <stdint.h>

#ifdef _WIN32
#ifdef rime_EXPORTS
/* DLL export */
#define RIME_API extern "C" __declspec(dllexport)
#else  /* rime_EXPORTS */
/* DLL import */
#define RIME_API extern "C" __declspec(dllimport)
#endif  /* rime_EXPORTS */
#else  /* _WIN32 */
#define RIME_API
#endif  /* _WIN32 */

typedef uintptr_t RimeSessionId;

RIME_API void RimeInitialize();
RIME_API void RimeFinalize();

// session management

RIME_API RimeSessionId RimeCreateSession();
RIME_API bool RimeDestroySession(RimeSessionId session_id);
RIME_API void RimeCleanupStaleSessions();
RIME_API void RimeCleanupAllSessions();

// using sessions

RIME_API bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask);

const int kRimeTextMaxLength = 255;
const int kRimeSchemaMaxLength = 127;
const int kRimeMaxNumCandidates = 9;

struct RimeComposition {
  bool is_composing;
  int cursor_pos;
  int sel_start;
  int sel_end;
  char preedit[kRimeTextMaxLength + 1];
};

struct RimeMenu {
  int page_size;
  int page_no;
  bool is_last_page;
  int highlighted_candidate_index;
  int num_candidates;
  char candidates[kRimeMaxNumCandidates][kRimeTextMaxLength + 1];
};

struct RimeContext {
  RimeComposition composition;
  RimeMenu menu;
};

struct RimeCommit {
  char text[kRimeTextMaxLength + 1];
};

struct RimeStatus {
  char schema_id[kRimeSchemaMaxLength + 1];
  char schema_name[kRimeSchemaMaxLength + 1];
  bool is_disabled;
  bool is_ascii_mode;
  bool is_simplified;
};

RIME_API bool RimeGetContext(RimeSessionId session_id, RimeContext* context);
RIME_API bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit);
RIME_API bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status);

// for testing

RIME_API bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence);

#endif  // RIME_H_
