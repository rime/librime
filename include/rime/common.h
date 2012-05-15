// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <cstdlib>
#include <string>
#include <memory>

namespace rime {

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

template <class A, class B>
shared_ptr<A> As(const B &ptr) {
  return std::dynamic_pointer_cast<A>(ptr);
}

void RegisterComponents();

const std::string GetLogFilePath();

}  // namespace rime

// setup ezlogger
#ifndef EZLOGGER_OUTPUT_FILENAME
#define EZLOGGER_OUTPUT_FILENAME rime::GetLogFilePath()
#endif
#ifndef EZLOGGER_REPLACE_EXISTING_LOGFILE_
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_
#endif
// DEBUG
//#define EZLOGGER_IMPLEMENT_DEBUGLOGGING
#include <ezlogger/ezlogger_headers.hpp>

#endif  // RIME_COMMON_H_
