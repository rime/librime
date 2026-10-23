//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_LOGGING_H_
#define RIME_LOGGING_H_

#include <cstring>
#include <functional>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>

#ifndef _WIN32
#include <strings.h>
#endif

#include "rime_api.h"

namespace rime {

enum class LogSeverity : int {
  kInfo = RIME_LOG_INFO,
  kWarning = RIME_LOG_WARNING,
  kError = RIME_LOG_ERROR,
  kFatal = RIME_LOG_FATAL,
};

namespace logging_internal {

//! Returns true when a record with the given severity should be emitted.
RIME_DLL bool log_is_enabled(LogSeverity severity);

//! Emits a record to the installed handler. FATAL records always abort.
RIME_DLL void write_log(const char* file,
                        int line,
                        const char* function,
                        LogSeverity severity,
                        std::string message);

//! Stream used to discard log arguments when no handler is installed.
RIME_DLL std::ostream& null_stream();

[[noreturn]] RIME_DLL void check_failed(const char* file,
                                        int line,
                                        const char* expression);
[[noreturn]] RIME_DLL void check_op_failed(const char* file,
                                           int line,
                                           const char* expression,
                                           const std::string& value1,
                                           const std::string& value2);

template <class T>
std::string stringify(const T& value) {
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

inline bool string_equal(const char* lhs, const char* rhs) {
  return std::strcmp(lhs, rhs) == 0;
}

inline bool string_not_equal(const char* lhs, const char* rhs) {
  return !string_equal(lhs, rhs);
}

inline bool string_case_equal(const char* lhs, const char* rhs) {
#ifdef _WIN32
  return _stricmp(lhs, rhs) == 0;
#else
  return strcasecmp(lhs, rhs) == 0;
#endif
}

inline bool string_case_not_equal(const char* lhs, const char* rhs) {
  return !string_case_equal(lhs, rhs);
}

template <class T>
T check_notnull(const char* file, int line, const char* expression, T value) {
  if (value == nullptr) {
    check_failed(file, line, expression);
  }
  return value;
}

class CheckMessage {
 public:
  CheckMessage(const char* file,
               int line,
               const char* function,
               bool condition,
               const char* expression)
      : file_(file),
        line_(line),
        function_(function),
        condition_(condition),
        expression_(expression) {}

  ~CheckMessage() {
    if (condition_) {
      return;
    }
    std::string message = "Check failed: ";
    message += expression_;
    std::string streamed = stream_.str();
    if (!streamed.empty()) {
      message += " ";
      message += streamed;
    }
    write_log(file_, line_, function_, LogSeverity::kFatal, message);
  }

  std::ostream& stream() {
    return condition_ ? null_stream() : static_cast<std::ostream&>(stream_);
  }

 private:
  const char* file_;
  int line_;
  const char* function_;
  bool condition_;
  const char* expression_;
  std::ostringstream stream_;
};

class LogMessage {
 public:
  LogMessage(const char* file,
             int line,
             const char* function,
             LogSeverity severity,
             bool condition = true)
      : file_(file),
        line_(line),
        function_(function),
        severity_(severity),
        active_(condition && log_is_enabled(severity)) {}

  ~LogMessage() {
    if (active_) {
      write_log(file_, line_, function_, severity_, stream_.str());
    }
  }

  std::ostream& stream() {
    return active_ ? static_cast<std::ostream&>(stream_) : null_stream();
  }

 private:
  const char* file_;
  int line_;
  const char* function_;
  LogSeverity severity_;
  bool active_;
  std::ostringstream stream_;
};

//! A stream-like sink that discards everything written to it.
class NullLogger {
 public:
  NullLogger& stream() { return *this; }

  template <class T>
  NullLogger& operator<<(const T&) {
    return *this;
  }
};

class Voidify {
 public:
  void operator&(NullLogger&) {}
};

}  // namespace logging_internal

}  // namespace rime

// The condition is constant so the right-hand side is never evaluated at
// runtime, but it still type-checks `LOG_EVERY_N(...) << value`.
#define RIME_LOG_NOOP                          \
  true ? (void)0                               \
       : ::rime::logging_internal::Voidify() & \
             ::rime::logging_internal::NullLogger().stream()

// Severity tokens are pasted into these level macros so that Windows macros
// such as ERROR from wingdi.h are not expanded in LOG(ERROR).
#define RIME_LOG_LEVEL_INFO RIME_LOG_INFO
#define RIME_LOG_LEVEL_WARNING RIME_LOG_WARNING
#define RIME_LOG_LEVEL_ERROR RIME_LOG_ERROR
#define RIME_LOG_LEVEL_FATAL RIME_LOG_FATAL

#define RIME_LOG_STREAM(level, condition)                                    \
  ::rime::logging_internal::LogMessage(                                      \
      __FILE__, __LINE__, __func__, static_cast<::rime::LogSeverity>(level), \
      (condition))                                                           \
      .stream()

#define LOG(severity) RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, true)
#define LOG_IF(severity, condition) \
  RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, (condition))
#define LOG_EVERY_N(severity, n) RIME_LOG_NOOP
#define LOG_IF_EVERY_N(severity, condition, n) RIME_LOG_NOOP
#define LOG_ASSERT(condition) RIME_LOG_NOOP
#define VLOG(verboselevel) RIME_LOG_NOOP

#define CHECK(condition)                                               \
  ::rime::logging_internal::CheckMessage(__FILE__, __LINE__, __func__, \
                                         (condition), #condition)      \
      .stream()

#define RIME_CHECK_OP(op, val1, val2)                           \
  do {                                                          \
    const auto& rime_check_lhs = (val1);                        \
    const auto& rime_check_rhs = (val2);                        \
    if (!(rime_check_lhs op rime_check_rhs)) {                  \
      ::rime::logging_internal::check_op_failed(                \
          __FILE__, __LINE__, #val1 " " #op " " #val2,          \
          ::rime::logging_internal::stringify(rime_check_lhs),  \
          ::rime::logging_internal::stringify(rime_check_rhs)); \
    }                                                           \
  } while (0)

#define CHECK_EQ(val1, val2) RIME_CHECK_OP(==, val1, val2)
#define CHECK_NE(val1, val2) RIME_CHECK_OP(!=, val1, val2)
#define CHECK_LE(val1, val2) RIME_CHECK_OP(<=, val1, val2)
#define CHECK_LT(val1, val2) RIME_CHECK_OP(<, val1, val2)
#define CHECK_GE(val1, val2) RIME_CHECK_OP(>=, val1, val2)
#define CHECK_GT(val1, val2) RIME_CHECK_OP(>, val1, val2)

#define CHECK_NOTNULL(val) \
  ::rime::logging_internal::check_notnull(__FILE__, __LINE__, #val, (val))

#define RIME_CHECK_STR(predicate, val1, val2)                   \
  do {                                                          \
    const char* rime_check_lhs = (val1);                        \
    const char* rime_check_rhs = (val2);                        \
    if (!(predicate(rime_check_lhs, rime_check_rhs))) {         \
      ::rime::logging_internal::check_op_failed(                \
          __FILE__, __LINE__, #val1 " / " #val2,                \
          ::rime::logging_internal::stringify(rime_check_lhs),  \
          ::rime::logging_internal::stringify(rime_check_rhs)); \
    }                                                           \
  } while (0)

#define CHECK_STREQ(val1, val2) \
  RIME_CHECK_STR(::rime::logging_internal::string_equal, val1, val2)
#define CHECK_STRNE(val1, val2) \
  RIME_CHECK_STR(::rime::logging_internal::string_not_equal, val1, val2)
#define CHECK_STRCASEEQ(val1, val2) \
  RIME_CHECK_STR(::rime::logging_internal::string_case_equal, val1, val2)
#define CHECK_STRCASENE(val1, val2) \
  RIME_CHECK_STR(::rime::logging_internal::string_case_not_equal, val1, val2)

#ifndef NDEBUG
#define DLOG(severity) RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, true)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition) \
  RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, (condition))
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) \
  LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(val1, val2) CHECK_STREQ(val1, val2)
#define DCHECK_STRNE(val1, val2) CHECK_STRNE(val1, val2)
#define DCHECK_STRCASEEQ(val1, val2) CHECK_STRCASEEQ(val1, val2)
#define DCHECK_STRCASENE(val1, val2) CHECK_STRCASENE(val1, val2)
#else
#define DLOG(severity) RIME_LOG_NOOP
#define DVLOG(verboselevel) RIME_LOG_NOOP
#define DLOG_IF(severity, condition) RIME_LOG_NOOP
#define DLOG_EVERY_N(severity, n) RIME_LOG_NOOP
#define DLOG_IF_EVERY_N(severity, condition, n) RIME_LOG_NOOP
#define DLOG_ASSERT(condition) RIME_LOG_NOOP
#define DCHECK(condition) RIME_LOG_NOOP
#define DCHECK_EQ(val1, val2) RIME_LOG_NOOP
#define DCHECK_NE(val1, val2) RIME_LOG_NOOP
#define DCHECK_LE(val1, val2) RIME_LOG_NOOP
#define DCHECK_LT(val1, val2) RIME_LOG_NOOP
#define DCHECK_GE(val1, val2) RIME_LOG_NOOP
#define DCHECK_GT(val1, val2) RIME_LOG_NOOP
#define DCHECK_NOTNULL(val) (val)
#define DCHECK_STREQ(val1, val2) RIME_LOG_NOOP
#define DCHECK_STRNE(val1, val2) RIME_LOG_NOOP
#define DCHECK_STRCASEEQ(val1, val2) RIME_LOG_NOOP
#define DCHECK_STRCASENE(val1, val2) RIME_LOG_NOOP
#endif  // NDEBUG

#endif  // RIME_LOGGING_H_
