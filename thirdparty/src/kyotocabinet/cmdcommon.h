/*************************************************************************************************
 * Common symbols for command line utilities
 *                                                               Copyright (C) 2009-2012 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _CMDCOMMON_H                     // duplication check
#define _CMDCOMMON_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>
#include <kcfile.h>
#include <kccompress.h>
#include <kccompare.h>
#include <kcmap.h>
#include <kcregex.h>
#include <kcdb.h>

#if !defined(_KC_PREFIX)
#define _KC_PREFIX       "*"
#endif
#if !defined(_KC_INCLUDEDIR)
#define _KC_INCLUDEDIR   "*"
#endif
#if !defined(_KC_LIBDIR)
#define _KC_LIBDIR       "*"
#endif
#if !defined(_KC_BINDIR)
#define _KC_BINDIR       "*"
#endif
#if !defined(_KC_LIBEXECDIR)
#define _KC_LIBEXECDIR   "*"
#endif
#if !defined(_KC_APPINC)
#define _KC_APPINC       "*"
#endif
#if !defined(_KC_APPLIBS)
#define _KC_APPLIBS      "*"
#endif

namespace kc = kyotocabinet;


// constants
const int32_t THREADMAX = 64;            // maximum number of threads
const size_t RECBUFSIZ = 64;             // buffer size for a record
const size_t RECBUFSIZL = 1024;          // buffer size for a long record


// global variables
uint64_t g_rnd_x = 123456789;
uint64_t g_rnd_y = 362436069;
uint64_t g_rnd_z = 521288629;
uint64_t g_rnd_w = 88675123;


// function prototypes
void mysrand(int64_t seed);
int64_t myrand(int64_t range);
int64_t memusage();
void oprintf(const char* format, ...);
void oputchar(char c);
void eprintf(const char* format, ...);
void printversion();
void printdata(const char* buf, int32_t size, bool px);
bool mygetline(std::istream* is, std::string* str);
std::string unitnumstr(int64_t num);
std::string unitnumstrbyte(int64_t num);
kc::BasicDB::ProgressChecker* stdchecker(const char* prefix, std::ostream* strm);
kc::BasicDB::Logger* stdlogger(const char* progname, std::ostream* strm);
void printdb(kc::BasicDB* db, bool px = false);


// checker to show progress by printing dots
class DotChecker : public kc::BasicDB::ProgressChecker {
 public:
  explicit DotChecker(std::ostream* strm, int64_t freq) : strm_(strm), freq_(freq), cnt_(0) {}
  int64_t count() {
    return cnt_;
  }
 private:
  bool check(const char* name, const char* message, int64_t curcnt, int64_t allcnt) {
    if (std::strcmp(message, "processing") || freq_ == 0) return true;
    if (freq_ < 0) {
      cnt_++;
      if (cnt_ % -freq_ == 0) {
        oputchar('.');
        if (cnt_ % (-freq_ * 50) == 0) oprintf(" (%lld)\n", (long long)cnt_);
      }
    } else {
      if (curcnt > cnt_) {
        cnt_ = curcnt;
        if (cnt_ % freq_ == 0) {
          oputchar('.');
          if (cnt_ % (freq_ * 50) == 0) oprintf(" (%lld)\n", (long long)cnt_);
        }
      }
    }
    return true;
  }
  std::ostream* strm_;
  int64_t freq_;
  int64_t cnt_;
};


// get the random seed
inline void mysrand(int64_t seed) {
  g_rnd_x = seed;
  for (int32_t i = 0; i < 16; i++) {
    myrand(1);
  }
}


// get a random number
inline int64_t myrand(int64_t range) {
  uint64_t t = g_rnd_x ^ (g_rnd_x << 11);
  g_rnd_x = g_rnd_y;
  g_rnd_y = g_rnd_z;
  g_rnd_z = g_rnd_w;
  g_rnd_w = (g_rnd_w ^ (g_rnd_w >> 19)) ^ (t ^ (t >> 8));
  return (g_rnd_w & kc::INT64MAX) % range;
}


// get the current memory usage
inline int64_t memusage() {
  std::map<std::string, std::string> info;
  kc::getsysinfo(&info);
  return kc::atoi(info["mem_rss"].c_str());
}


// print formatted information string and flush the buffer
inline void oprintf(const char* format, ...) {
  std::string msg;
  va_list ap;
  va_start(ap, format);
  kc::vstrprintf(&msg, format, ap);
  va_end(ap);
  std::cout << msg;
  std::cout.flush();
}


// print a character and flush the buffer
inline void oputchar(char c) {
  std::cout << c;
  std::cout.flush();
}


// print formatted error string and flush the buffer
inline void eprintf(const char* format, ...) {
  std::string msg;
  va_list ap;
  va_start(ap, format);
  kc::vstrprintf(&msg, format, ap);
  va_end(ap);
  std::cerr << msg;
  std::cerr.flush();
}


// print the versin information
inline void printversion() {
  oprintf("Kyoto Cabinet %s (%d.%d:%d) on %s\n",
          kc::VERSION, kc::LIBVER, kc::LIBREV, kc::FMTVER, kc::OSNAME);
}


// print record data
inline void printdata(const char* buf, int32_t size, bool px) {
  size_t cnt = 0;
  char numbuf[kc::NUMBUFSIZ];
  while (size-- > 0) {
    if (px) {
      if (cnt++ > 0) putchar(' ');
      std::sprintf(numbuf, "%02X", *(unsigned char*)buf);
      std::cout << numbuf;
    } else {
      std::cout << *buf;
    }
    buf++;
  }
}


// read a line from a file descriptor
inline bool mygetline(std::istream* is, std::string* str) {
  str->clear();
  bool hit = false;
  char c;
  while (is->get(c)) {
    hit = true;
    if (c == '\0' || c == '\r') continue;
    if (c == '\n') break;
    str->append(1, c);
  }
  return hit;
}


// convert a number into the string with the decimal unit
inline std::string unitnumstr(int64_t num) {
  if (num >= std::pow(1000.0, 6)) {
    return kc::strprintf("%.3Lf quintillion", (long double)num / std::pow(1000.0, 6));
  } else if (num >= std::pow(1000.0, 5)) {
    return kc::strprintf("%.3Lf quadrillion", (long double)num / std::pow(1000.0, 5));
  } else if (num >= std::pow(1000.0, 4)) {
    return kc::strprintf("%.3Lf trillion", (long double)num / std::pow(1000.0, 4));
  } else if (num >= std::pow(1000.0, 3)) {
    return kc::strprintf("%.3Lf billion", (long double)num / std::pow(1000.0, 3));
  } else if (num >= std::pow(1000.0, 2)) {
    return kc::strprintf("%.3Lf million", (long double)num / std::pow(1000.0, 2));
  } else if (num >= std::pow(1000.0, 1)) {
    return kc::strprintf("%.3Lf thousand", (long double)num / std::pow(1000.0, 1));
  }
  return kc::strprintf("%lld", (long long)num);
}


// convert a number into the string with the byte unit
inline std::string unitnumstrbyte(int64_t num) {
  if ((unsigned long long)num >= 1ULL << 60) {
    return kc::strprintf("%.3Lf EiB", (long double)num / (1ULL << 60));
  } else if ((unsigned long long)num >= 1ULL << 50) {
    return kc::strprintf("%.3Lf PiB", (long double)num / (1ULL << 50));
  } else if ((unsigned long long)num >= 1ULL << 40) {
    return kc::strprintf("%.3Lf TiB", (long double)num / (1ULL << 40));
  } else if ((unsigned long long)num >= 1ULL << 30) {
    return kc::strprintf("%.3Lf GiB", (long double)num / (1ULL << 30));
  } else if ((unsigned long long)num >= 1ULL << 20) {
    return kc::strprintf("%.3Lf MiB", (long double)num / (1ULL << 20));
  } else if ((unsigned long long)num >= 1ULL << 10) {
    return kc::strprintf("%.3Lf KiB", (long double)num / (1ULL << 10));
  }
  return kc::strprintf("%lld B", (long long)num);
}


// get the progress checker to print the parameters
inline kc::BasicDB::ProgressChecker* stdchecker(const char* prefix, std::ostream* strm) {
  class CheckerImpl : public kc::BasicDB::ProgressChecker {
   public:
    explicit CheckerImpl(std::ostream* strm, const char* prefix) :
        strm_(strm), prefix_(prefix) {}
    bool check(const char* name, const char* message, int64_t curcnt, int64_t allcnt) {
      *strm_ << prefix_ << ": " << name << ": " << message << ": " <<
          curcnt << "/" << allcnt << std::endl;
      return true;
    }
   private:
    std::ostream* strm_;
    const char* prefix_;
  };
  static CheckerImpl checker(strm, prefix);
  return &checker;
}


// get the logger into the standard stream
inline kc::BasicDB::Logger* stdlogger(const char* prefix, std::ostream* strm) {
  class LoggerImpl : public kc::BasicDB::Logger {
   public:
    explicit LoggerImpl(std::ostream* strm, const char* prefix) :
        strm_(strm), prefix_(prefix) {}
    void log(const char* file, int32_t line, const char* func, Kind kind,
             const char* message) {
      const char* kstr = "MISC";
      switch (kind) {
        case kc::BasicDB::Logger::DEBUG: kstr = "DEBUG"; break;
        case kc::BasicDB::Logger::INFO: kstr = "INFO"; break;
        case kc::BasicDB::Logger::WARN: kstr = "WARN"; break;
        case kc::BasicDB::Logger::ERROR: kstr = "ERROR"; break;
      }
      *strm_ << prefix_ << ": [" << kstr << "]: " <<
          file << ": " << line << ": " << func << ": " << message << std::endl;
    }
   private:
    std::ostream* strm_;
    const char* prefix_;
  };
  static LoggerImpl logger(strm, prefix);
  return &logger;
}


// print all record of a database
inline void printdb(kc::BasicDB* db, bool px) {
  class Printer : public kc::DB::Visitor {
   public:
    explicit Printer(bool px) : px_(px) {}
   private:
    const char* visit_full(const char* kbuf, size_t ksiz,
                           const char* vbuf, size_t vsiz, size_t* sp) {
      printdata(kbuf, ksiz, px_);
      oputchar('\t');
      printdata(vbuf, vsiz, px_);
      oputchar('\n');
      return NOP;
    }
    bool px_;
  } printer(px);
  db->iterate(&printer, false);
}


#endif                                   // duplication check

// END OF FILE
