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
// TODO: 
//RIME_API ... 

#endif  // RIME_H_
