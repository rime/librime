//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/last_error.h>

namespace rime {

namespace {

// The error is kept per thread, so that a client can query the error of the
// API call it just made from the same thread. Some API calls are made by
// librime's own threads (e.g. the maintenance thread), whose errors are of no
// interest to the client and should not overwrite the reported one.
thread_local RimeError g_error_code = RIME_ERROR_NONE;

}  // namespace

void ClearLastError() {
  g_error_code = RIME_ERROR_NONE;
}

void SetLastError(RimeError code) {
  if (code == RIME_ERROR_NONE) {
    ClearLastError();
    return;
  }
  g_error_code = code;
}

RimeError LastErrorCode() {
  return g_error_code;
}

}  // namespace rime
