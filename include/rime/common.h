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
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace rime {

using boost::scoped_ptr;
using boost::shared_ptr;
using boost::weak_ptr;

template <class A, class B>
shared_ptr<A> As(const B &ptr) {
  return boost::dynamic_pointer_cast<A>(ptr);
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
