/*************************************************************************************************
 * Utility functions
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


#include "kcutil.h"
#include "myconf.h"

namespace kyotocabinet {                 // common namespace


/** The package version. */
const char* const VERSION = _KC_VERSION;


/** The library version. */
const int32_t LIBVER = _KC_LIBVER;


/** The library revision. */
const int32_t LIBREV = _KC_LIBREV;


/** The database format version. */
const int32_t FMTVER = _KC_FMTVER;


/** The system name. */
const char* const OSNAME = _KC_OSNAME;


/** The flag for big endian environments. */
const bool BIGEND = _KC_BIGEND ? true : false;


/** The clock tick of interruption. */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
const int32_t CLOCKTICK = 100;
#else
const int32_t CLOCKTICK = sysconf(_SC_CLK_TCK);
#endif


/** The size of a page. */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static int32_t win_getpagesize() {
  ::SYSTEM_INFO ibuf;
  ::GetSystemInfo(&ibuf);
  return ibuf.dwPageSize;
}
const int32_t PAGESIZ = win_getpagesize();
#else
const int32_t PAGESIZ = sysconf(_SC_PAGESIZE);
#endif


/** The extra feature list. */
const char* const FEATURES = ""
#if _KC_GCCATOMIC
    "(atomic)"
#endif
#if _KC_ZLIB
    "(zlib)"
#endif
#if _KC_LZO
    "(lzo)"
#endif
#if _KC_LZMA
    "(lzma)"
#endif
    ;


// get the levenshtein distance of two arrays
template<class CHARTYPE, class CNTTYPE>
static size_t levdist(const CHARTYPE* abuf, size_t asiz, const CHARTYPE* bbuf, size_t bsiz) {
  size_t dsiz = bsiz + 1;
  size_t tsiz = (asiz + 1) * dsiz;
  CNTTYPE tblstack[2048/sizeof(CNTTYPE)];
  CNTTYPE* tbl = tsiz > sizeof(tblstack) / sizeof(*tblstack) ? new CNTTYPE[tsiz] : tblstack;
  tbl[0] = 0;
  for (size_t i = 1; i <= asiz; i++) {
    tbl[i*dsiz] = i;
  }
  for (size_t i = 1; i <= bsiz; i++) {
    tbl[i] = i;
  }
  abuf--;
  bbuf--;
  for (size_t i = 1; i <= asiz; i++) {
    for (size_t j = 1; j <= bsiz; j++) {
      uint32_t ac = tbl[(i-1)*dsiz+j] + 1;
      uint32_t bc = tbl[i*dsiz+j-1] + 1;
      uint32_t cc = tbl[(i-1)*dsiz+j-1] + (abuf[i] != bbuf[j]);
      ac = ac < bc ? ac : bc;
      tbl[i*dsiz+j] = ac < cc ? ac : cc;
    }
  }
  size_t ed = tbl[asiz*dsiz+bsiz];
  if (tbl != tblstack) delete[] tbl;
  return ed;
}


/**
 * Calculate the levenshtein distance of two regions in bytes.
 */
size_t memdist(const void* abuf, size_t asiz, const void* bbuf, size_t bsiz) {
  _assert_(abuf && asiz <= MEMMAXSIZ && bbuf && bsiz <= MEMMAXSIZ);
  return asiz > UINT8MAX || bsiz > UINT8MAX ?
    levdist<const char, uint32_t>((const char*)abuf, asiz, (const char*)bbuf, bsiz) :
    levdist<const char, uint8_t>((const char*)abuf, asiz, (const char*)bbuf, bsiz);
}


/**
 * Calculate the levenshtein distance of two UTF-8 strings.
 */
size_t strutfdist(const char* astr, const char* bstr) {
  _assert_(astr && bstr);
  size_t anum = strutflen(astr);
  uint32_t astack[128];
  uint32_t* aary = anum > sizeof(astack) / sizeof(*astack) ? new uint32_t[anum] : astack;
  strutftoucs(astr, aary, &anum);
  size_t bnum = strutflen(bstr);
  uint32_t bstack[128];
  uint32_t* bary = bnum > sizeof(bstack) / sizeof(*bstack) ? new uint32_t[bnum] : bstack;
  strutftoucs(bstr, bary, &bnum);
  size_t dist = strucsdist(aary, anum, bary, bnum);
  if (bary != bstack) delete[] bary;
  if (aary != astack) delete[] aary;
  return dist;
}


/**
 * Calculate the levenshtein distance of two UCS-4 arrays.
 */
size_t strucsdist(const uint32_t* aary, size_t anum, const uint32_t* bary, size_t bnum) {
  _assert_(aary && anum <= MEMMAXSIZ && bary && bnum <= MEMMAXSIZ);
  return anum > UINT8MAX || bnum > UINT8MAX ?
    levdist<const uint32_t, uint32_t>(aary, anum, bary, bnum) :
    levdist<const uint32_t, uint8_t>(aary, anum, bary, bnum);
}


/**
 * Allocate a nullified region on memory.
 */
void* mapalloc(size_t size) {
#if defined(_SYS_LINUX_)
  _assert_(size > 0 && size <= MEMMAXSIZ);
  void* ptr = ::mmap(0, sizeof(size) + size,
                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) throw std::bad_alloc();
  *(size_t*)ptr = size;
  return (char*)ptr + sizeof(size);
#else
  _assert_(size > 0 && size <= MEMMAXSIZ);
  void* ptr = std::calloc(size, 1);
  if (!ptr) throw std::bad_alloc();
  return ptr;
#endif
}


/**
 * Free a region on memory.
 */
void mapfree(void* ptr) {
#if defined(_SYS_LINUX_)
  _assert_(ptr);
  size_t size = *((size_t*)ptr - 1);
  ::munmap((char*)ptr - sizeof(size), sizeof(size) + size);
#else
  _assert_(ptr);
  std::free(ptr);
#endif
}


/**
 * Get the time of day in seconds.
 * @return the time of day in seconds.  The accuracy is in microseconds.
 */
double time() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  ::LARGE_INTEGER li;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  return li.QuadPart / 10000000.0;
#else
  _assert_(true);
  struct ::timeval tv;
  if (::gettimeofday(&tv, NULL) != 0) return 0.0;
  return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}


/**
 * Get the process ID.
 */
int64_t getpid() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  return ::GetCurrentProcessId();
#else
  _assert_(true);
  return ::getpid();
#endif
}


/**
 * Get the value of an environment variable.
 */
const char* getenv(const char* name) {
  _assert_(name);
  return ::getenv(name);
}


/**
 * Get system information of the environment.
 */
void getsysinfo(std::map<std::string, std::string>* strmap) {
#if defined(_SYS_LINUX_)
  _assert_(strmap);
  struct ::rusage rbuf;
  std::memset(&rbuf, 0, sizeof(rbuf));
  if (::getrusage(RUSAGE_SELF, &rbuf) == 0) {
    (*strmap)["ru_utime"] = strprintf("%0.6f",
                                      rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    (*strmap)["ru_stime"] = strprintf("%0.6f",
                                      rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
    if (rbuf.ru_maxrss > 0) {
      int64_t size = rbuf.ru_maxrss * 1024LL;
      (*strmap)["mem_peak"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_size"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_rss"] = strprintf("%lld", (long long)size);
    }
  }
  std::ifstream ifs;
  ifs.open("/proc/self/status", std::ios_base::in | std::ios_base::binary);
  if (ifs) {
    std::string line;
    while (getline(ifs, line)) {
      size_t idx = line.find(':');
      if (idx != std::string::npos) {
        const std::string& name = line.substr(0, idx);
        idx++;
        while (idx < line.size() && line[idx] >= '\0' && line[idx] <= ' ') {
          idx++;
        }
        const std::string& value = line.substr(idx);
        if (name == "VmPeak") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_peak"] = strprintf("%lld", (long long)size);
        } else if (name == "VmSize") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_size"] = strprintf("%lld", (long long)size);
        } else if (name == "VmRSS") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_rss"] = strprintf("%lld", (long long)size);
        }
      }
    }
    ifs.close();
  }
  ifs.open("/proc/meminfo", std::ios_base::in | std::ios_base::binary);
  if (ifs) {
    std::string line;
    while (getline(ifs, line)) {
      size_t idx = line.find(':');
      if (idx != std::string::npos) {
        const std::string& name = line.substr(0, idx);
        idx++;
        while (idx < line.size() && line[idx] >= '\0' && line[idx] <= ' ') {
          idx++;
        }
        const std::string& value = line.substr(idx);
        if (name == "MemTotal") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_total"] = strprintf("%lld", (long long)size);
        } else if (name == "MemFree") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_free"] = strprintf("%lld", (long long)size);
        } else if (name == "Cached") {
          int64_t size = atoix(value.c_str());
          if (size > 0) (*strmap)["mem_cached"] = strprintf("%lld", (long long)size);
        }
      }
    }
    ifs.close();
  }
#elif defined(_SYS_MACOSX_)
  _assert_(strmap);
  struct ::rusage rbuf;
  std::memset(&rbuf, 0, sizeof(rbuf));
  if (::getrusage(RUSAGE_SELF, &rbuf) == 0) {
    (*strmap)["ru_utime"] = strprintf("%0.6f",
                                      rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    (*strmap)["ru_stime"] = strprintf("%0.6f",
                                      rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
    if (rbuf.ru_maxrss > 0) {
      int64_t size = rbuf.ru_maxrss;
      (*strmap)["mem_peak"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_size"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_rss"] = strprintf("%lld", (long long)size);
    }
  }
#elif defined(_SYS_FREEBSD_) || defined(_SYS_SUNOS_)
  _assert_(strmap);
  struct ::rusage rbuf;
  std::memset(&rbuf, 0, sizeof(rbuf));
  if (::getrusage(RUSAGE_SELF, &rbuf) == 0) {
    (*strmap)["ru_utime"] = strprintf("%0.6f",
                                      rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    (*strmap)["ru_stime"] = strprintf("%0.6f",
                                      rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
    if (rbuf.ru_maxrss > 0) {
      int64_t size = rbuf.ru_maxrss * 1024LL;
      (*strmap)["mem_peak"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_size"] = strprintf("%lld", (long long)size);
      (*strmap)["mem_rss"] = strprintf("%lld", (long long)size);
    }
  }
#elif defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(strmap);
  ::DWORD pid = ::GetCurrentProcessId();
  ::HANDLE ph = ::OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);
  if (ph) {
    ::FILETIME ct, et, kt, ut;
    if (::GetProcessTimes(ph, &ct, &et, &kt, &ut)) {
      ::LARGE_INTEGER li;
      li.LowPart = ut.dwLowDateTime;
      li.HighPart = ut.dwHighDateTime;
      (*strmap)["ru_utime"] = strprintf("%0.6f", li.QuadPart / 10000000.0);
      li.LowPart = kt.dwLowDateTime;
      li.HighPart = kt.dwHighDateTime;
      (*strmap)["ru_stime"] = strprintf("%0.6f", li.QuadPart / 10000000.0);
    }
    ::CloseHandle(ph);
  }
  ::MEMORYSTATUSEX msbuf;
  msbuf.dwLength = sizeof(msbuf);
  ::GlobalMemoryStatusEx(&msbuf);
  (*strmap)["mem_total"] = strprintf("%lld", (long long)msbuf.ullTotalPhys);
  (*strmap)["mem_free"] = strprintf("%lld", (long long)msbuf.ullAvailPhys);
  int64_t cached = msbuf.ullTotalPhys - msbuf.ullAvailPhys;
  (*strmap)["mem_cached"] = strprintf("%lld", (long long)cached);
#else
  _assert_(strmap);
#endif
}


/**
 * Set the standard streams into the binary mode.
 */
void setstdiobin() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  _setmode(_fileno(stdin), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stderr), O_BINARY);
#else
  _assert_(true);
#endif
}


}                                        // common namespace

// END OF FILE
