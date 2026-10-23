//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <gtest/gtest.h>

// Windows headers (wingdi.h) define ERROR as 0. Define it here as well so the
// logging macros are compiled with a conflicting severity token on every
// platform.
#ifndef ERROR
#define ERROR 0
#endif

#include <rime/common.h>

TEST(LoggingErrorMacroTest, AcceptsWindowsErrorMacro) {
  LOG(ERROR) << "error";
  LOG_IF(ERROR, false) << "conditional error";
  DLOG(ERROR) << "debug error";
  DLOG_IF(ERROR, false) << "conditional debug error";
  SUCCEED();
}
