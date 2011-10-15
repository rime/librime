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

namespace rime {

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::dynamic_pointer_cast;

void RegisterComponents();

const std::string GetLogFilePath();

}  // namespace rime

// setup ezlogger
#define EZLOGGER_OUTPUT_FILENAME rime::GetLogFilePath()
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_
#include <ezlogger/ezlogger_headers.hpp>

#endif  // RIME_COMMON_H_
