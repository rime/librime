//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Error of the most recent Rime API call, as retrieved by the client through
// RimeApi::get_last_error().
//
#ifndef RIME_LAST_ERROR_H_
#define RIME_LAST_ERROR_H_

#include <rime_api.h>

namespace rime {

//! Records `code` as the error of the most recent failed API call on the
//! calling thread. Passing RIME_ERROR_NONE forgets the recorded error.
RIME_DLL void SetLastError(RimeError code);

//! Forgets the error recorded on the calling thread, e.g. after an API call
//! known to have succeeded.
RIME_DLL void ClearLastError();

//! Error code of the most recent failed API call on the calling thread;
//! RIME_ERROR_NONE if no error has been recorded.
RIME_DLL RimeError LastErrorCode();

}  // namespace rime

#endif  // RIME_LAST_ERROR_H_
