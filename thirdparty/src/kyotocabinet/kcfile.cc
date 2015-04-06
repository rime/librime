/*************************************************************************************************
 * Filesystem abstraction
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


#include "kcfile.h"
#include "myconf.h"

namespace kyotocabinet {                 // common namespace


/**
 * Constants for implementation.
 */
namespace {
const int32_t FILEPERM = 00644;          ///< default permission of a new file
const int32_t DIRPERM = 00755;           ///< default permission of a new directory
const int32_t PATHBUFSIZ = 8192;         ///< size of the path buffer
const int32_t IOBUFSIZ = 16384;          ///< size of the IO buffer
const int64_t FILEMAXSIZ = INT64MAX - INT32MAX;  // maximum size of a file
const char* const WALPATHEXT = "wal";    ///< extension of the WAL file
const char WALMAGICDATA[] = "KW\n";      ///< magic data of the WAL file
const uint8_t WALMSGMAGIC = 0xee;        ///< magic data for WAL record
}


/**
 * File internal.
 */
struct FileCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  Mutex alock;                           ///< attribute lock
  TSDKey errmsg;                         ///< error message
  ::HANDLE fh;                           ///< file handle
  ::HANDLE mh;                           ///< map view handle
  char* map;                             ///< mapped memory
  int64_t msiz;                          ///< map size
  int64_t lsiz;                          ///< logical size
  int64_t psiz;                          ///< physical size
  std::string path;                      ///< file path
  bool recov;                            ///< flag of recovery
  uint32_t omode;                        ///< open mode
  ::HANDLE walfh;                        ///< file handle for WAL
  int64_t walsiz;                        ///< size of WAL
  bool tran;                             ///< whether in transaction
  bool trhard;                           ///< whether hard transaction
  int64_t trbase;                        ///< base offset of guarded region
  int64_t trmsiz;                        ///< minimum size during transaction
#else
  Mutex alock;                           ///< attribute lock
  TSDKey errmsg;                         ///< error message
  int32_t fd;                            ///< file descriptor
  char* map;                             ///< mapped memory
  int64_t msiz;                          ///< map size
  int64_t lsiz;                          ///< logical size
  int64_t psiz;                          ///< physical size
  std::string path;                      ///< file path
  bool recov;                            ///< flag of recovery
  uint32_t omode;                        ///< open mode
  int32_t walfd;                         ///< file descriptor for WAL
  int64_t walsiz;                        ///< size of WAL
  bool tran;                             ///< whether in transaction
  bool trhard;                           ///< whether hard transaction
  int64_t trbase;                        ///< base offset of guarded region
  int64_t trmsiz;                        ///< minimum size during transaction
#endif
};


/**
 * WAL message.
 */
struct WALMessage {
  int64_t off;                           ///< offset of the region
  std::string body;                      ///< body data
};


/**
 * DirStream internal.
 */
struct DirStreamCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  Mutex alock;                           ///< attribute lock
  ::HANDLE dh;                           ///< directory handle
  std::string cur;                       ///< current file
#else
  Mutex alock;                           ///< attribute lock
  ::DIR* dh;                             ///< directory handle
#endif
};


/**
 * Set the error message.
 * @param core the inner condition.
 * @param msg the error message.
 */
static void seterrmsg(FileCore* core, const char* msg);


/**
 * Get the path of the WAL file.
 * @param path the path of the destination file.
 * @return the path of the WAL file.
 */
static std::string walpath(const std::string& path);


/**
 * Write a log message into the WAL file.
 * @param core the inner condition.
 * @param off the offset of the destination.
 * @param size the size of the data region.
 * @param base the base offset.
 * @return true on success, or false on failure.
 */
static bool walwrite(FileCore *core, int64_t off, size_t size, int64_t base);


/**
 * Apply log messages in the WAL file.
 * @param core the inner condition.
 * @return true on success, or false on failure.
 */
static bool walapply(FileCore* core);


/**
 * Write data into a file.
 * @param fd the file descriptor.
 * @param off the offset of the destination.
 * @param buf the pointer to the data region.
 * @param size the size of the data region.
 * @return true on success, or false on failure.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static bool mywrite(::HANDLE fh, int64_t off, const void* buf, size_t size);
#else
static bool mywrite(int32_t fd, int64_t off, const void* buf, size_t size);
#endif


/**
 * Read data from a file.
 * @param fd the file descriptor.
 * @param buf the pointer to the destination region.
 * @param size the size of the data to be read.
 * @return true on success, or false on failure.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static size_t myread(::HANDLE fh, void* buf, size_t size);
#else
static size_t myread(int32_t fd, void* buf, size_t count);
#endif


/**
 * System call emulation for Win32.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static int64_t win_pwrite(::HANDLE fh, const void* buf, size_t count, int64_t offset);
static int64_t win_pread(::HANDLE fh, void* buf, size_t count, int64_t offset);
static int64_t win_write(::HANDLE fh, const void* buf, size_t count);
static int64_t win_read(::HANDLE fh, void* buf, size_t count);
static int32_t win_ftruncate(::HANDLE fh, int64_t length);
#endif


/** Path delimiter character. */
const char File::PATHCHR = MYPATHCHR;


/** Path delimiter string. */
const char* const File::PATHSTR = MYPATHSTR;


/** Extension delimiter character. */
const char File::EXTCHR = MYEXTCHR;


/** Extension delimiter string. */
const char* const File::EXTSTR = MYEXTSTR;


/** Current directory string. */
const char* const File::CDIRSTR = MYCDIRSTR;


/** Parent directory string. */
const char* const File::PDIRSTR = MYPDIRSTR;


/**
 * Default constructor.
 */
File::File() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = new FileCore;
  core->fh = NULL;
  core->mh = NULL;
  core->map = NULL;
  core->msiz = 0;
  core->lsiz = 0;
  core->psiz = 0;
  core->recov = false;
  core->omode = 0;
  core->walfh = NULL;
  core->walsiz = 0;
  core->tran = false;
  core->trhard = false;
  core->trmsiz = 0;
  opq_ = core;
#else
  _assert_(true);
  FileCore* core = new FileCore;
  core->fd = -1;
  core->map = NULL;
  core->msiz = 0;
  core->lsiz = 0;
  core->psiz = 0;
  core->recov = false;
  core->omode = 0;
  core->walfd = -1;
  core->walsiz = 0;
  core->tran = false;
  core->trhard = false;
  core->trmsiz = 0;
  opq_ = core;
#endif
}


/**
 * Destructor.
 */
File::~File() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  if (core->fh) close();
  delete core;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  if (core->fd >= 0) close();
  delete core;
#endif
}


/**
 * Get the last happened error information.
 */
const char* File::error() const {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  const char* msg = (const char*)core->errmsg.get();
  if (!msg) msg = "no error";
  return msg;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  const char* msg = (const char*)core->errmsg.get();
  if (!msg) msg = "no error";
  return msg;
#endif
}


/**
 * Open a file.
 */
bool File::open(const std::string& path, uint32_t mode, int64_t msiz) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(msiz >= 0 && msiz <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  ::DWORD amode = GENERIC_READ;
  ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  ::DWORD cmode = OPEN_EXISTING;
  if (mode & OWRITER) {
    amode |= GENERIC_WRITE;
    if (mode & OCREATE) {
      cmode = OPEN_ALWAYS;
      if (mode & OTRUNCATE) cmode = CREATE_ALWAYS;
    } else {
      if (mode & OTRUNCATE) cmode = TRUNCATE_EXISTING;
    }
  }
  ::HANDLE fh = ::CreateFile(path.c_str(), amode, smode, NULL, cmode,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (!fh || fh == INVALID_HANDLE_VALUE) {
    seterrmsg(core, "CreateFile failed");
    return false;
  }
  if (!(mode & ONOLOCK)) {
    ::DWORD lmode = mode & OWRITER ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    if (mode & OTRYLOCK) lmode |= LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED ol;
    ol.Offset = INT32MAX;
    ol.OffsetHigh = 0;
    ol.hEvent = 0;
    if (!::LockFileEx(fh, lmode, 0, 1, 0, &ol)) {
      seterrmsg(core, "LockFileEx failed");
      ::CloseHandle(fh);
      return false;
    }
  }
  ::LARGE_INTEGER sbuf;
  if (!::GetFileSizeEx(fh, &sbuf)) {
    seterrmsg(core, "GetFileSizeEx failed");
    ::CloseHandle(fh);
    return false;
  }
  bool recov = false;
  if ((!(mode & OWRITER) || !(mode & OTRUNCATE)) && !(mode & ONOLOCK)) {
    const std::string& wpath = walpath(path);
    ::HANDLE walfh = ::CreateFile(wpath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
    if (walfh && walfh != INVALID_HANDLE_VALUE) {
      recov = true;
      ::LARGE_INTEGER li;
      if (::GetFileSizeEx(walfh, &li) && li.QuadPart >= (int64_t)sizeof(WALMAGICDATA)) {
        char mbuf[sizeof(WALMAGICDATA)];
        if (myread(walfh, mbuf, sizeof(mbuf)) &&
            !std::memcmp(mbuf, WALMAGICDATA, sizeof(WALMAGICDATA))) {
          ::HANDLE ofh = fh;
          if (!(mode & OWRITER)) ofh = ::CreateFile(wpath.c_str(), GENERIC_WRITE, 0, NULL,
                                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
          if (ofh && ofh != INVALID_HANDLE_VALUE) {
            core->fh = ofh;
            core->walfh = walfh;
            walapply(core);
            if (ofh != fh && !::CloseHandle(ofh)) seterrmsg(core, "CloseHandle failed");
            li.QuadPart = 0;
            if (win_ftruncate(walfh, 0) != 0) seterrmsg(core, "win_ftruncate failed");
            core->fh = NULL;
            core->walfh = NULL;
            if (!::GetFileSizeEx(fh, &sbuf)) {
              seterrmsg(core, "GetFileSizeEx failed");
              ::CloseHandle(fh);
              return false;
            }
          } else {
            seterrmsg(core, "CreateFile failed");
          }
        }
      }
      if (!::CloseHandle(walfh)) seterrmsg(core, "CloseHandle failed");
      ::DeleteFile(wpath.c_str());
    }
  }
  int64_t lsiz = sbuf.QuadPart;
  int64_t psiz = lsiz;
  int64_t diff = msiz % PAGESIZ;
  if (diff > 0) msiz += PAGESIZ - diff;
  ::DWORD mprot = PAGE_READONLY;
  ::DWORD vmode = FILE_MAP_READ;
  if (mode & OWRITER) {
    mprot = PAGE_READWRITE;
    vmode = FILE_MAP_WRITE;
  } else if (msiz > lsiz) {
    msiz = lsiz;
  }
  sbuf.QuadPart = msiz;
  ::HANDLE mh = NULL;
  void* map = NULL;
  if (msiz > 0) {
    mh = ::CreateFileMapping(fh, NULL, mprot, sbuf.HighPart, sbuf.LowPart, NULL);
    if (!mh || mh == INVALID_HANDLE_VALUE) {
      seterrmsg(core, "CreateFileMapping failed");
      ::CloseHandle(fh);
      return false;
    }
    map = ::MapViewOfFile(mh, vmode, 0, 0, 0);
    if (!map) {
      seterrmsg(core, "MapViewOfFile failed");
      ::CloseHandle(mh);
      ::CloseHandle(fh);
      return false;
    }
    if (psiz < msiz) psiz = msiz;
  }
  core->fh = fh;
  core->mh = mh;
  core->map = (char*)map;
  core->msiz = msiz;
  core->lsiz = lsiz;
  core->psiz = psiz;
  core->recov = recov;
  core->omode = mode;
  core->path.append(path);
  return true;
#else
  _assert_(msiz >= 0 && msiz <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  int32_t oflags = O_RDONLY;
  if (mode & OWRITER) {
    oflags = O_RDWR;
    if (mode & OCREATE) oflags |= O_CREAT;
    if (mode & OTRUNCATE) oflags |= O_TRUNC;
  }
  int32_t fd = ::open(path.c_str(), oflags, FILEPERM);
  if (fd < 0) {
    switch (errno) {
      case EACCES: seterrmsg(core, "open failed (permission denied)"); break;
      case EISDIR: seterrmsg(core, "open failed (directory)"); break;
      case ENOENT: seterrmsg(core, "open failed (file not found)"); break;
      case ENOTDIR: seterrmsg(core, "open failed (invalid path)"); break;
      case ENOSPC: seterrmsg(core, "open failed (no space)"); break;
      default: seterrmsg(core, "open failed"); break;
    }
    return false;
  }
  if (!(mode & ONOLOCK)) {
    struct flock flbuf;
    std::memset(&flbuf, 0, sizeof(flbuf));
    flbuf.l_type = mode & OWRITER ? F_WRLCK : F_RDLCK;
    flbuf.l_whence = SEEK_SET;
    flbuf.l_start = 0;
    flbuf.l_len = 0;
    flbuf.l_pid = 0;
    int32_t cmd = mode & OTRYLOCK ? F_SETLK : F_SETLKW;
    while (::fcntl(fd, cmd, &flbuf) != 0) {
      if (errno != EINTR) {
        seterrmsg(core, "fcntl failed");
        ::close(fd);
        return false;
      }
    }
  }
  struct ::stat sbuf;
  if (::fstat(fd, &sbuf) != 0) {
    seterrmsg(core, "fstat failed");
    ::close(fd);
    return false;
  }
  if (!S_ISREG(sbuf.st_mode)) {
    seterrmsg(core, "not a regular file");
    ::close(fd);
    return false;
  }
  bool recov = false;
  if ((!(mode & OWRITER) || !(mode & OTRUNCATE)) && !(mode & ONOLOCK)) {
    const std::string& wpath = walpath(path);
    int32_t walfd = ::open(wpath.c_str(), O_RDWR, FILEPERM);
    if (walfd >= 0) {
      struct ::stat wsbuf;
      if (::fstat(walfd, &wsbuf) == 0 && wsbuf.st_uid == sbuf.st_uid) {
        recov = true;
        if (wsbuf.st_size >= (int64_t)sizeof(WALMAGICDATA)) {
          char mbuf[sizeof(WALMAGICDATA)];
          if (myread(walfd, mbuf, sizeof(mbuf)) &&
              !std::memcmp(mbuf, WALMAGICDATA, sizeof(WALMAGICDATA))) {
            int32_t ofd = mode & OWRITER ? fd : ::open(path.c_str(), O_WRONLY, FILEPERM);
            if (ofd >= 0) {
              core->fd = ofd;
              core->walfd = walfd;
              walapply(core);
              if (ofd != fd && ::close(ofd) != 0) seterrmsg(core, "close failed");
              if (::ftruncate(walfd, 0) != 0) seterrmsg(core, "ftruncate failed");
              core->fd = -1;
              core->walfd = -1;
              if (::fstat(fd, &sbuf) != 0) {
                seterrmsg(core, "fstat failed");
                ::close(fd);
                return false;
              }
            } else {
              seterrmsg(core, "open failed");
            }
          }
        }
      }
      if (::close(walfd) != 0) seterrmsg(core, "close failed");
      if (::unlink(wpath.c_str()) != 0) seterrmsg(core, "unlink failed");
    }
  }
  int64_t lsiz = sbuf.st_size;
  int64_t psiz = lsiz;
  int64_t diff = msiz % PAGESIZ;
  if (diff > 0) msiz += PAGESIZ - diff;
  int32_t mprot = PROT_READ;
  if (mode & OWRITER) {
    mprot |= PROT_WRITE;
  } else if (msiz > lsiz) {
    msiz = lsiz;
  }
  void* map = NULL;
  if (msiz > 0) {
    map = ::mmap(0, msiz, mprot, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
      seterrmsg(core, "mmap failed");
      ::close(fd);
      return false;
    }
  }
  core->fd = fd;
  core->map = (char*)map;
  core->msiz = msiz;
  core->lsiz = lsiz;
  core->psiz = psiz;
  core->recov = recov;
  core->omode = mode;
  core->path.append(path);
  return true;
#endif
}


/**
 * Close the file.
 */
bool File::close() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  if (core->tran && !end_transaction(false)) err = true;
  if (core->walfh) {
    if (!::CloseHandle(core->walfh)) {
      seterrmsg(core, "CloseHandle failed");
      err = true;
    }
    const std::string& wpath = walpath(core->path);
    ::DeleteFile(wpath.c_str());
  }
  if (core->msiz > 0) {
    if (!::UnmapViewOfFile(core->map)) {
      seterrmsg(core, "UnmapViewOfFile failed");
      err = true;
    }
    if (!::CloseHandle(core->mh)) {
      seterrmsg(core, "CloseHandle failed");
      err = true;
    }
  }
  ::LARGE_INTEGER li;
  if (::GetFileSizeEx(core->fh, &li)) {
    if ((li.QuadPart != core->lsiz || core->psiz != core->lsiz) &&
        win_ftruncate(core->fh, core->lsiz) != 0) {
      seterrmsg(core, "win_ftruncate failed");
      err = true;
    }
  } else {
    seterrmsg(core, "GetFileSizeEx failed");
    err = true;
  }
  if (!(core->omode & ONOLOCK)) {
    OVERLAPPED ol;
    ol.Offset = INT32MAX;
    ol.OffsetHigh = 0;
    ol.hEvent = 0;
    if (!::UnlockFileEx(core->fh, 0, 1, 0, &ol)) {
      seterrmsg(core, "UnlockFileEx failed");
      err = true;
    }
  }
  if (!::CloseHandle(core->fh)) {
    seterrmsg(core, "CloseHandle failed");
    err = true;
  }
  core->fh = NULL;
  core->mh = NULL;
  core->map = NULL;
  core->msiz = 0;
  core->lsiz = 0;
  core->psiz = 0;
  core->path.clear();
  core->walfh = NULL;
  core->walsiz = 0;
  core->tran = false;
  core->trhard = false;
  core->trmsiz = 0;
  return !err;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  if (core->tran && !end_transaction(false)) err = true;
  if (core->walfd >= 0) {
    if (::close(core->walfd) != 0) {
      seterrmsg(core, "close failed");
      err = true;
    }
    const std::string& wpath = walpath(core->path);
    struct ::stat sbuf;
    if (::lstat(wpath.c_str(), &sbuf) == 0 && S_ISREG(sbuf.st_mode) &&
        ::unlink(wpath.c_str()) != 0) {
      seterrmsg(core, "unlink failed");
      err = true;
    }
  }
  if (core->msiz > 0 && ::munmap(core->map, core->msiz) != 0) {
    seterrmsg(core, "munmap failed");
    err = true;
  }
  if (core->psiz != core->lsiz && ::ftruncate(core->fd, core->lsiz) != 0) {
    seterrmsg(core, "ftruncate failed");
    err = true;
  }
  if (!(core->omode & ONOLOCK)) {
    struct flock flbuf;
    std::memset(&flbuf, 0, sizeof(flbuf));
    flbuf.l_type = F_UNLCK;
    flbuf.l_whence = SEEK_SET;
    flbuf.l_start = 0;
    flbuf.l_len = 0;
    flbuf.l_pid = 0;
    while (::fcntl(core->fd, F_SETLKW, &flbuf) != 0) {
      if (errno != EINTR) {
        seterrmsg(core, "fcntl failed");
        err = true;
        break;
      }
    }
  }
  if (::close(core->fd) != 0) {
    seterrmsg(core, "close failed");
    err = true;
  }
  core->fd = -1;
  core->map = NULL;
  core->msiz = 0;
  core->lsiz = 0;
  core->psiz = 0;
  core->path.clear();
  core->walfd = -1;
  core->walsiz = 0;
  core->tran = false;
  core->trhard = false;
  core->trmsiz = 0;
  return !err;
#endif
}


/**
 * Write data.
 */
bool File::write(int64_t off, const void* buf, size_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  if (core->tran && !walwrite(core, off, size, core->trbase)) return false;
  int64_t end = off + size;
  core->alock.lock();
  if (end <= core->msiz) {
    if (end > core->psiz) {
      int64_t psiz = end + core->psiz / 2;
      int64_t diff = psiz % PAGESIZ;
      if (diff > 0) psiz += PAGESIZ - diff;
      if (psiz > core->msiz) psiz = core->msiz;
      if (win_ftruncate(core->fh, psiz) != 0) {
        seterrmsg(core, "win_ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = psiz;
    }
    if (end > core->lsiz) core->lsiz = end;
    core->alock.unlock();
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    if (end > core->psiz) {
      if (win_ftruncate(core->fh, end) != 0) {
        seterrmsg(core, "win_ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = end;
    }
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  if (end > core->lsiz) core->lsiz = end;
  if (end > core->psiz) {
    if (core->psiz < core->msiz && win_ftruncate(core->fh, core->msiz) != 0) {
      seterrmsg(core, "win_ftruncate failed");
      core->alock.unlock();
      return false;
    }
    core->psiz = end;
  }
  core->alock.unlock();
  if (!mywrite(core->fh, off, buf, size)) {
    seterrmsg(core, "mywrite failed");
    return false;
  }
  return true;
#else
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  if (core->tran && !walwrite(core, off, size, core->trbase)) return false;
  int64_t end = off + size;
  core->alock.lock();
  if (end <= core->msiz) {
    if (end > core->psiz) {
      int64_t psiz = end + core->psiz / 2;
      int64_t diff = psiz % PAGESIZ;
      if (diff > 0) psiz += PAGESIZ - diff;
      if (psiz > core->msiz) psiz = core->msiz;
      if (::ftruncate(core->fd, psiz) != 0) {
        seterrmsg(core, "ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = psiz;
    }
    if (end > core->lsiz) core->lsiz = end;
    core->alock.unlock();
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    if (end > core->psiz) {
      if (::ftruncate(core->fd, end) != 0) {
        seterrmsg(core, "ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = end;
    }
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  if (end > core->lsiz) core->lsiz = end;
  if (end > core->psiz) {
    if (core->psiz < core->msiz && ::ftruncate(core->fd, core->msiz) != 0) {
      seterrmsg(core, "ftruncate failed");
      core->alock.unlock();
      return false;
    }
    core->psiz = end;
  }
  core->alock.unlock();
  if (!mywrite(core->fd, off, buf, size)) {
    seterrmsg(core, "mywrite failed");
    return false;
  }
  return true;
#endif
}


/**
 * Write data with assuring the region does not spill from the file size.
 */
bool File::write_fast(int64_t off, const void* buf, size_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  if (core->tran && !walwrite(core, off, size, core->trbase)) return false;
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  if (!mywrite(core->fh, off, buf, size)) {
    seterrmsg(core, "mywrite failed");
    return false;
  }
  return true;
#else
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  if (core->tran && !walwrite(core, off, size, core->trbase)) return false;
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  if (!mywrite(core->fd, off, buf, size)) {
    seterrmsg(core, "mywrite failed");
    return false;
  }
  return true;
#endif
}


/**
 * Write data at the end of the file.
 */
bool File::append(const void* buf, size_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  core->alock.lock();
  int64_t off = core->lsiz;
  int64_t end = off + size;
  if (end <= core->msiz) {
    if (end > core->psiz) {
      int64_t psiz = end + core->psiz / 2;
      int64_t diff = psiz % PAGESIZ;
      if (diff > 0) psiz += PAGESIZ - diff;
      if (psiz > core->msiz) psiz = core->msiz;
      if (win_ftruncate(core->fh, psiz) != 0) {
        seterrmsg(core, "win_ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = psiz;
    }
    core->lsiz = end;
    core->alock.unlock();
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    if (end > core->psiz) {
      if (win_ftruncate(core->fh, end) != 0) {
        seterrmsg(core, "win_ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = end;
    }
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  core->lsiz = end;
  core->psiz = end;
  core->alock.unlock();
  while (true) {
    int64_t wb = win_pwrite(core->fh, buf, size, off);
    if (wb >= (int64_t)size) {
      return true;
    } else if (wb > 0) {
      buf = (char*)buf + wb;
      size -= wb;
      off += wb;
    } else if (wb == -1) {
      seterrmsg(core, "win_pwrite failed");
      return false;
    } else if (size > 0) {
      seterrmsg(core, "win_pwrite failed");
      return false;
    }
  }
  return true;
#else
  _assert_(buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  core->alock.lock();
  int64_t off = core->lsiz;
  int64_t end = off + size;
  if (end <= core->msiz) {
    if (end > core->psiz) {
      int64_t psiz = end + core->psiz / 2;
      int64_t diff = psiz % PAGESIZ;
      if (diff > 0) psiz += PAGESIZ - diff;
      if (psiz > core->msiz) psiz = core->msiz;
      if (::ftruncate(core->fd, psiz) != 0) {
        seterrmsg(core, "ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = psiz;
    }
    core->lsiz = end;
    core->alock.unlock();
    std::memcpy(core->map + off, buf, size);
    return true;
  }
  if (off < core->msiz) {
    if (end > core->psiz) {
      if (::ftruncate(core->fd, end) != 0) {
        seterrmsg(core, "ftruncate failed");
        core->alock.unlock();
        return false;
      }
      core->psiz = end;
    }
    size_t hsiz = core->msiz - off;
    std::memcpy(core->map + off, buf, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  core->lsiz = end;
  core->psiz = end;
  core->alock.unlock();
  while (true) {
    ssize_t wb = ::pwrite(core->fd, buf, size, off);
    if (wb >= (ssize_t)size) {
      return true;
    } else if (wb > 0) {
      buf = (char*)buf + wb;
      size -= wb;
      off += wb;
    } else if (wb == -1) {
      if (errno != EINTR) {
        seterrmsg(core, "pwrite failed");
        return false;
      }
    } else if (size > 0) {
      seterrmsg(core, "pwrite failed");
      return false;
    }
  }
  return true;
#endif
}


/**
 * Read data.
 */
bool File::read(int64_t off, void* buf, size_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  int64_t end = off + size;
  core->alock.lock();
  if (end > core->lsiz) {
    seterrmsg(core, "out of bounds");
    core->alock.unlock();
    return false;
  }
  core->alock.unlock();
  if (end <= core->msiz) {
    std::memcpy(buf, core->map + off, size);
    return true;
  }
  if (off < core->msiz) {
    int64_t hsiz = core->msiz - off;
    std::memcpy(buf, core->map + off, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  while (true) {
    int64_t rb = win_pread(core->fh, buf, size, off);
    if (rb >= (int64_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
      off += rb;
    } else if (rb == -1) {
      seterrmsg(core, "win_pread failed");
      return false;
    } else if (size > 0) {
      Thread::yield();
    }
  }
  return true;
#else
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  if (size < 1) return true;
  FileCore* core = (FileCore*)opq_;
  int64_t end = off + size;
  core->alock.lock();
  if (end > core->lsiz) {
    seterrmsg(core, "out of bounds");
    core->alock.unlock();
    return false;
  }
  core->alock.unlock();
  if (end <= core->msiz) {
    std::memcpy(buf, core->map + off, size);
    return true;
  }
  if (off < core->msiz) {
    int64_t hsiz = core->msiz - off;
    std::memcpy(buf, core->map + off, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  while (true) {
    ssize_t rb = ::pread(core->fd, buf, size, off);
    if (rb >= (ssize_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
      off += rb;
    } else if (rb == -1) {
      if (errno != EINTR) {
        seterrmsg(core, "pread failed");
        return false;
      }
    } else if (size > 0) {
      Thread::yield();
    }
  }
  return true;
#endif
}


/**
 * Read data with assuring the region does not spill from the file size.
 */
bool File::read_fast(int64_t off, void* buf, size_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(buf, core->map + off, size);
    return true;
  }
  if (off < core->msiz) {
    int64_t hsiz = core->msiz - off;
    std::memcpy(buf, core->map + off, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  while (true) {
    int64_t rb = win_pread(core->fh, buf, size, off);
    if (rb >= (int64_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
      off += rb;
      Thread::yield();
    } else if (rb == -1) {
      seterrmsg(core, "win_pread failed");
      return false;
    } else if (size > 0) {
      if (end > core->lsiz) {
        seterrmsg(core, "out of bounds");
        return false;
      }
      Thread::yield();
    }
  }
  return true;
#else
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(buf, core->map + off, size);
    return true;
  }
  if (off < core->msiz) {
    int64_t hsiz = core->msiz - off;
    std::memcpy(buf, core->map + off, hsiz);
    off += hsiz;
    buf = (char*)buf + hsiz;
    size -= hsiz;
  }
  while (true) {
    ssize_t rb = ::pread(core->fd, buf, size, off);
    if (rb >= (ssize_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
      off += rb;
      Thread::yield();
    } else if (rb == -1) {
      if (errno != EINTR) {
        seterrmsg(core, "pread failed");
        return false;
      }
    } else if (size > 0) {
      if (end > core->lsiz) {
        seterrmsg(core, "out of bounds");
        return false;
      }
      Thread::yield();
    }
  }
  return true;
#endif
}


/**
 * Truncate the file.
 */
bool File::truncate(int64_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(size >= 0 && size <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  if (core->tran && size < core->trmsiz) {
    if (!walwrite(core, size, core->trmsiz - size, core->trbase)) return false;
    core->trmsiz = size;
  }
  bool err = false;
  core->alock.lock();
  if (core->msiz > 0) {
    if (!::UnmapViewOfFile(core->map)) {
      seterrmsg(core, "UnmapViewOfFile failed");
      err = true;
    }
    if (!::CloseHandle(core->mh)) {
      seterrmsg(core, "CloseHandle failed");
      err = true;
    }
  }
  if (win_ftruncate(core->fh, size) != 0) {
    seterrmsg(core, "win_ftruncate failed");
    err = true;
  }
  if (core->msiz) {
    ::LARGE_INTEGER li;
    li.QuadPart = core->msiz;
    ::HANDLE mh = ::CreateFileMapping(core->fh, NULL, PAGE_READWRITE,
                                      li.HighPart, li.LowPart, NULL);
    if (mh && mh != INVALID_HANDLE_VALUE) {
      void* map = ::MapViewOfFile(mh, FILE_MAP_WRITE, 0, 0, 0);
      if (map) {
        core->mh = mh;
        core->map = (char*)map;
      } else {
        seterrmsg(core, "MapViewOfFile failed");
        ::CloseHandle(mh);
        core->mh = NULL;
        core->map = NULL;
        core->msiz = 0;
        err = true;
      }
    } else {
      seterrmsg(core, "CreateFileMapping failed");
      core->mh = NULL;
      core->map = NULL;
      core->msiz = 0;
      err = true;
    }
  }
  core->lsiz = size;
  core->psiz = size;
  core->alock.unlock();
  return !err;
#else
  _assert_(size >= 0 && size <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  if (core->tran && size < core->trmsiz) {
    if (!walwrite(core, size, core->trmsiz - size, core->trbase)) return false;
    core->trmsiz = size;
  }
  bool err = false;
  core->alock.lock();
  if (::ftruncate(core->fd, size) != 0) {
    seterrmsg(core, "ftruncate failed");
    err = true;
  }
  core->lsiz = size;
  core->psiz = size;
  core->alock.unlock();
  return !err;
#endif
}


/**
 * Synchronize updated contents with the file and the device.
 */
bool File::synchronize(bool hard) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  core->alock.lock();
  if (hard && core->msiz > 0) {
    int64_t msiz = core->msiz;
    if (msiz > core->psiz) msiz = core->psiz;
    if (msiz > 0 && !::FlushViewOfFile(core->map, msiz)) {
      seterrmsg(core, "FlushViewOfFile failed");
      err = true;
    }
  }
  if (win_ftruncate(core->fh, core->lsiz) != 0) {
    seterrmsg(core, "win_ftruncate failed");
    err = true;
  }
  if (core->psiz > core->lsiz) core->psiz = core->lsiz;
  if (hard && !::FlushFileBuffers(core->fh)) {
    seterrmsg(core, "FlushFileBuffers failed");
    err = true;
  }
  core->alock.unlock();
  return !err;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  core->alock.lock();
  if (hard && core->msiz > 0) {
    int64_t msiz = core->msiz;
    if (msiz > core->psiz) msiz = core->psiz;
    if (msiz > 0 && ::msync(core->map, msiz, MS_SYNC) != 0) {
      seterrmsg(core, "msync failed");
      err = true;
    }
  }
  if (::ftruncate(core->fd, core->lsiz) != 0) {
    seterrmsg(core, "ftruncate failed");
    err = true;
  }
  if (core->psiz > core->lsiz) core->psiz = core->lsiz;
  if (hard && ::fsync(core->fd) != 0) {
    seterrmsg(core, "fsync failed");
    err = true;
  }
  core->alock.unlock();
  return !err;
#endif
}


/**
 * Refresh the internal state for update by others.
 */
bool File::refresh() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  ::LARGE_INTEGER sbuf;
  if (!::GetFileSizeEx(core->fh, &sbuf)) {
    seterrmsg(core, "GetFileSizeEx failed");
    return false;
  }
  core->lsiz = sbuf.QuadPart;
  core->psiz = sbuf.QuadPart;
  return true;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  struct ::stat sbuf;
  if (::fstat(core->fd, &sbuf) != 0) {
    seterrmsg(core, "fstat failed");
    return false;
  }
  core->lsiz = sbuf.st_size;
  core->psiz = sbuf.st_size;
  bool err = false;
  int64_t msiz = core->msiz;
  if (msiz > core->psiz) msiz = core->psiz;
  if (msiz > 0 && ::msync(core->map, msiz, MS_INVALIDATE) != 0) {
    seterrmsg(core, "msync failed");
    err = true;
  }
  return !err;
#endif
}


/**
 * Begin transaction.
 */
bool File::begin_transaction(bool hard, int64_t off) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(off >= 0 && off <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  core->alock.lock();
  if (!core->walfh) {
    const std::string& wpath = walpath(core->path);
    ::DWORD amode = GENERIC_READ | GENERIC_WRITE;
    ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    ::HANDLE fh = ::CreateFile(wpath.c_str(), amode, smode, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (!fh || fh == INVALID_HANDLE_VALUE) {
      seterrmsg(core, "CreateFile failed");
      core->alock.unlock();
      return false;
    }
    if (hard && !::FlushFileBuffers(fh)) {
      seterrmsg(core, "FlushFileBuffers failed");
      ::CloseHandle(fh);
      core->alock.unlock();
      return false;
    }
    core->walfh = fh;
  }
  char wbuf[NUMBUFSIZ];
  char* wp = wbuf;
  std::memcpy(wp, WALMAGICDATA, sizeof(WALMAGICDATA));
  wp += sizeof(WALMAGICDATA);
  int64_t num = hton64(core->lsiz);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  int64_t wsiz = wp - wbuf;
  if (!mywrite(core->walfh, 0, wbuf, wsiz)) {
    seterrmsg(core, "mywrite failed");
    core->alock.unlock();
    return false;
  }
  core->walsiz = wsiz;
  core->tran = true;
  core->trhard = hard;
  core->trbase = off;
  core->trmsiz = core->lsiz;
  core->alock.unlock();
  return true;
#else
  _assert_(off >= 0 && off <= FILEMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  core->alock.lock();
  if (core->walfd < 0) {
    const std::string& wpath = walpath(core->path);
    int32_t fd = ::open(wpath.c_str(), O_RDWR | O_CREAT | O_TRUNC, FILEPERM);
    if (fd < 0) {
      switch (errno) {
        case EACCES: seterrmsg(core, "open failed (permission denied)"); break;
        case ENOENT: seterrmsg(core, "open failed (file not found)"); break;
        case ENOTDIR: seterrmsg(core, "open failed (invalid path)"); break;
        default: seterrmsg(core, "open failed"); break;
      }
      core->alock.unlock();
      return false;
    }
    core->walfd = fd;
  }
  char wbuf[NUMBUFSIZ];
  char* wp = wbuf;
  std::memcpy(wp, WALMAGICDATA, sizeof(WALMAGICDATA));
  wp += sizeof(WALMAGICDATA);
  int64_t num = hton64(core->lsiz);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  int64_t wsiz = wp - wbuf;
  if (!mywrite(core->walfd, 0, wbuf, wsiz)) {
    seterrmsg(core, "mywrite failed");
    core->alock.unlock();
    return false;
  }
  core->walsiz = wsiz;
  core->tran = true;
  core->trhard = hard;
  core->trbase = off;
  core->trmsiz = core->lsiz;
  core->alock.unlock();
  return true;
#endif
}


/**
 * Commit transaction.
 */
bool File::end_transaction(bool commit) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  core->alock.lock();
  if (!commit && !walapply(core)) err = true;
  if (!err) {
    if (core->walsiz <= IOBUFSIZ) {
      char mbuf[IOBUFSIZ];
      std::memset(mbuf, 0, core->walsiz);
      if (!mywrite(core->walfh, 0, mbuf, core->walsiz)) {
        seterrmsg(core, "mywrite failed");
        err = true;
      }
    } else {
      if (win_ftruncate(core->walfh, 0) != 0) {
        seterrmsg(core, "win_ftruncate failed");
        err = true;
      }
    }
  }
  if (core->trhard) {
    int64_t msiz = core->msiz;
    if (msiz > core->psiz) msiz = core->psiz;
    if (msiz > 0 && !::FlushViewOfFile(core->map, msiz)) {
      seterrmsg(core, "FlushViewOfFile failed");
      err = true;
    }
    if (!::FlushFileBuffers(core->fh)) {
      seterrmsg(core, "FlushFileBuffers failed");
      err = true;
    }
    if (!::FlushFileBuffers(core->walfh)) {
      seterrmsg(core, "FlushFileBuffers failed");
      err = true;
    }
  }
  core->tran = false;
  core->alock.unlock();
  return !err;
#else
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  bool err = false;
  core->alock.lock();
  if (!commit && !walapply(core)) err = true;
  if (!err) {
    if (core->walsiz <= IOBUFSIZ) {
      char mbuf[IOBUFSIZ];
      std::memset(mbuf, 0, core->walsiz);
      if (!mywrite(core->walfd, 0, mbuf, core->walsiz)) {
        seterrmsg(core, "mywrite failed");
        err = true;
      }
    } else {
      if (::ftruncate(core->walfd, 0) != 0) {
        seterrmsg(core, "ftruncate failed");
        err = true;
      }
    }
  }
  if (core->trhard) {
    int64_t msiz = core->msiz;
    if (msiz > core->psiz) msiz = core->psiz;
    if (msiz > 0 && ::msync(core->map, msiz, MS_SYNC) != 0) {
      seterrmsg(core, "msync failed");
      err = true;
    }
    if (::fsync(core->fd) != 0) {
      seterrmsg(core, "fsync failed");
      err = true;
    }
    if (::fsync(core->walfd) != 0) {
      seterrmsg(core, "fsync failed");
      err = true;
    }
  }
  core->tran = false;
  core->alock.unlock();
  return !err;
#endif
}


/**
 * Write a WAL message of transaction explicitly.
 */
bool File::write_transaction(int64_t off, size_t size) {
  _assert_(off >= 0 && off <= FILEMAXSIZ && size <= MEMMAXSIZ);
  FileCore* core = (FileCore*)opq_;
  return walwrite(core, off, size, 0);
}


/**
 * Get the size of the file.
 */
int64_t File::size() const {
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  return core->lsiz;
}


/**
 * Get the path of the file.
 */
std::string File::path() const {
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  return core->path;
}


/**
 * Check whether the file was recovered or not.
 */
bool File::recovered() const {
  _assert_(true);
  FileCore* core = (FileCore*)opq_;
  return core->recov;
}


/**
 * Read the whole data from a file.
 */
char* File::read_file(const std::string& path, int64_t* sp, int64_t limit) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(sp);
  if (limit < 0) limit = INT64MAX;
  ::DWORD amode = GENERIC_READ;
  ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  ::DWORD cmode = OPEN_EXISTING;
  ::HANDLE fh = ::CreateFile(path.c_str(), amode, smode, NULL, cmode,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (!fh || fh == INVALID_HANDLE_VALUE) return NULL;
  ::LARGE_INTEGER sbuf;
  if (!::GetFileSizeEx(fh, &sbuf)) {
    ::CloseHandle(fh);
    return false;
  }
  if (limit > (int64_t)sbuf.QuadPart) limit = sbuf.QuadPart;
  char* buf = new char[limit+1];
  char* wp = buf;
  int64_t rsiz;
  while ((rsiz = win_read(fh, wp, limit - (wp - buf))) > 0) {
    wp += rsiz;
  }
  *wp = '\0';
  ::CloseHandle(fh);
  *sp = wp - buf;
  return buf;
#else
  _assert_(sp);
  if (limit < 0) limit = INT64MAX;
  int32_t fd = ::open(path.c_str(), O_RDONLY, FILEPERM);
  if (fd < 0) return NULL;
  struct stat sbuf;
  if (::fstat(fd, &sbuf) == -1 || !S_ISREG(sbuf.st_mode)) {
    ::close(fd);
    return NULL;
  }
  if (limit > (int64_t)sbuf.st_size) limit = sbuf.st_size;
  char* buf = new char[limit+1];
  char* wp = buf;
  ssize_t rsiz;
  while ((rsiz = ::read(fd, wp, limit - (wp - buf))) > 0) {
    wp += rsiz;
  }
  *wp = '\0';
  ::close(fd);
  *sp = wp - buf;
  return buf;
#endif
}


/**
 * Write the whole data into a file.
 */
bool File::write_file(const std::string& path, const char* buf, int64_t size) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(buf && size >= 0 && size <= FILEMAXSIZ);
  ::DWORD amode = GENERIC_WRITE;
  ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  ::DWORD cmode = CREATE_ALWAYS;
  double wsec = 1.0 / CLOCKTICK;
  ::HANDLE fh = INVALID_HANDLE_VALUE;
  for (int32_t i = 0; i < 10; i++) {
    fh = ::CreateFile(path.c_str(), amode, smode, NULL, cmode, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh && fh != INVALID_HANDLE_VALUE) break;
    if (::GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (wsec > 1.0) wsec = 1.0;
    Thread::sleep(wsec);
    wsec *= 2;
  }
  bool err = false;
  const char* rp = buf;
  while (!err && size > 0) {
    int64_t wb = win_write(fh, rp, size);
    switch (wb) {
      case -1: {
        if (errno != EINTR) {
          err = true;
          break;
        }
      }
      case 0: {
        break;
      }
      default: {
        rp += wb;
        size -= wb;
        break;
      }
    }
  }
  if (!::CloseHandle(fh)) err = true;
  return !err;
#else
  _assert_(buf && size >= 0 && size <= FILEMAXSIZ);
  int32_t fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, FILEPERM);
  if (fd < 0) return false;
  bool err = false;
  const char* rp = buf;
  while (!err && size > 0) {
    ssize_t wb = ::write(fd, rp, size);
    switch (wb) {
      case -1: {
        if (errno != EINTR) {
          err = true;
          break;
        }
      }
      case 0: {
        break;
      }
      default: {
        rp += wb;
        size -= wb;
        break;
      }
    }
  }
  if (::close(fd) != 0) err = true;
  return !err;
#endif
}


/**
 * Get the status information of a file.
 */
bool File::status(const std::string& path, Status* buf) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::WIN32_FILE_ATTRIBUTE_DATA ibuf;
  if (!::GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &ibuf)) return false;
  if (buf) {
    buf->isdir = ibuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    ::LARGE_INTEGER li;
    li.LowPart = ibuf.nFileSizeLow;
    li.HighPart = ibuf.nFileSizeHigh;
    buf->size = li.QuadPart;
    li.LowPart = ibuf.ftLastWriteTime.dwLowDateTime;
    li.HighPart = ibuf.ftLastWriteTime.dwHighDateTime;
    buf->mtime = li.QuadPart;
  }
  return true;
#else
  _assert_(true);
  struct ::stat sbuf;
  if (::lstat(path.c_str(), &sbuf) != 0) return false;
  if (buf) {
    buf->isdir = S_ISDIR(sbuf.st_mode);
    buf->size = sbuf.st_size;
    buf->mtime = sbuf.st_mtime;
  }
  return true;
#endif
}


/**
 * Get the absolute path of a file.
 */
std::string File::absolute_path(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  char buf[PATHBUFSIZ];
  ::DWORD size = ::GetFullPathName(path.c_str(), sizeof(buf), buf, NULL);
  if (size < 1) return "";
  if (size < sizeof(buf)) return std::string(buf);
  char* lbuf = new char[size];
  ::DWORD nsiz = ::GetFullPathName(path.c_str(), size, lbuf, NULL);
  if (nsiz < 1 || nsiz >= size) {
    delete[] lbuf;
    return "";
  }
  std::string rbuf(lbuf);
  delete[] lbuf;
  return rbuf;
#else
  _assert_(true);
  char buf[PATHBUFSIZ];
  if (!realpath(path.c_str(), buf)) return "";
  return std::string(buf);
#endif
}


/**
 * Remove a file.
 */
bool File::remove(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  double wsec = 1.0 / CLOCKTICK;
  for (int32_t i = 0; i < 10; i++) {
    if (::DeleteFile(path.c_str())) return true;
    if (::GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (wsec > 1.0) wsec = 1.0;
    Thread::sleep(wsec);
    wsec *= 2;
  }
  std::string tmppath;
  strprintf(&tmppath, "%s%ctmp%c%llx", path.c_str(), EXTCHR, EXTCHR,
            ((unsigned long long)(time() * UINT16MAX)) % UINT32MAX);
  if (::MoveFileEx(path.c_str(), tmppath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    ::DeleteFile(tmppath.c_str());
    ::DWORD amode = GENERIC_READ | GENERIC_WRITE;
    ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    ::HANDLE fh = ::CreateFile(tmppath.c_str(), amode, smode, NULL, OPEN_EXISTING,
                               FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (fh && fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
    return true;
  }
  return false;
#else
  _assert_(true);
  return ::unlink(path.c_str()) == 0;
#endif
}


/**
 * Change the name or location of a file.
 */
bool File::rename(const std::string& opath, const std::string& npath) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  double wsec = 1.0 / CLOCKTICK;
  for (int32_t i = 0; i < 10; i++) {
    if (::MoveFileEx(opath.c_str(), npath.c_str(), MOVEFILE_REPLACE_EXISTING)) return true;
    if (::GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (wsec > 1.0) wsec = 1.0;
    Thread::sleep(wsec);
    wsec *= 2;
  }
  std::string tmppath;
  strprintf(&tmppath, "%s%ctmp%c%llx", npath.c_str(), EXTCHR, EXTCHR,
            ((unsigned long long)(time() * UINT16MAX)) % UINT32MAX);
  if (::MoveFileEx(npath.c_str(), tmppath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    if (::MoveFileEx(opath.c_str(), npath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
      ::DeleteFile(tmppath.c_str());
      ::DWORD amode = GENERIC_READ | GENERIC_WRITE;
      ::DWORD smode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
      ::HANDLE fh = ::CreateFile(tmppath.c_str(), amode, smode, NULL, OPEN_EXISTING,
                                 FILE_FLAG_DELETE_ON_CLOSE, NULL);
      if (fh && fh != INVALID_HANDLE_VALUE) ::CloseHandle(fh);
      return true;
    } else {
      wsec = 1.0 / CLOCKTICK;
      for (int32_t i = 0; i < 10; i++) {
        if (::MoveFileEx(tmppath.c_str(), npath.c_str(), MOVEFILE_REPLACE_EXISTING)) break;
        if (wsec > 1.0) wsec = 1.0;
        Thread::sleep(wsec);
        wsec *= 2;
      }
    }
  }
  return false;
#else
  _assert_(true);
  return ::rename(opath.c_str(), npath.c_str()) == 0;
#endif
}


/**
 * Read a directory.
 */
bool File::read_directory(const std::string& path, std::vector<std::string>* strvec) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(strvec);
  std::string dpath = path;
  size_t plen = path.size();
  if (plen < 1 || path[plen-1] != PATHCHR) dpath.append(PATHSTR);
  dpath.append("*");
  ::WIN32_FIND_DATA fbuf;
  ::HANDLE dh = ::FindFirstFile(dpath.c_str(), &fbuf);
  if (!dh || dh == INVALID_HANDLE_VALUE) return false;
  if (std::strcmp(fbuf.cFileName, CDIRSTR) && std::strcmp(fbuf.cFileName, PDIRSTR))
    strvec->push_back(fbuf.cFileName);
  while (::FindNextFile(dh, &fbuf)) {
    if (std::strcmp(fbuf.cFileName, CDIRSTR) && std::strcmp(fbuf.cFileName, PDIRSTR))
      strvec->push_back(fbuf.cFileName);
  }
  if (!::FindClose(dh)) return false;
  return true;
#else
  _assert_(strvec);
  ::DIR* dir = ::opendir(path.c_str());
  if (!dir) return false;
  struct ::dirent *dp;
  while ((dp = ::readdir(dir)) != NULL) {
    if (std::strcmp(dp->d_name, CDIRSTR) && std::strcmp(dp->d_name, PDIRSTR))
      strvec->push_back(dp->d_name);
  }
  if (::closedir(dir) != 0) return false;
  return true;
#endif
}


/**
 * Make a directory.
 */
bool File::make_directory(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  double wsec = 1.0 / CLOCKTICK;
  for (int32_t i = 0; i < 10; i++) {
    if (::CreateDirectory(path.c_str(), NULL)) return true;
    if (::GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (wsec > 1.0) wsec = 1.0;
    Thread::sleep(wsec);
    wsec *= 2;
  }
  return false;
#else
  _assert_(true);
  return ::mkdir(path.c_str(), DIRPERM) == 0;
#endif
}


/**
 * Remove a directory.
 */
bool File::remove_directory(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  double wsec = 1.0 / CLOCKTICK;
  for (int32_t i = 0; i < 10; i++) {
    if (::RemoveDirectory(path.c_str())) return true;
    if (::GetLastError() != ERROR_ACCESS_DENIED) return false;
    if (wsec > 1.0) wsec = 1.0;
    Thread::sleep(wsec);
    wsec *= 2;
  }
  return false;
#else
  _assert_(true);
  return ::rmdir(path.c_str()) == 0;
#endif
}


/**
 * Remove a file or a directory recursively.
 */
bool File::remove_recursively(const std::string& path) {
  bool err = false;
  std::vector<std::string> list;
  list.push_back(path);
  while (!list.empty()) {
    const std::string& cpath = list.back();
    Status sbuf;
    if (status(cpath, &sbuf)) {
      if (sbuf.isdir) {
        if (remove_directory(cpath)) {
          list.pop_back();
        } else {
          DirStream dir;
          if (dir.open(cpath)) {
            std::string ccname;
            while (dir.read(&ccname)) {
              const std::string& ccpath = cpath + MYPATHCHR + ccname;
              if (!remove(ccpath)) list.push_back(ccpath);
            }
            if (!dir.close()) err = true;
          } else {
            list.pop_back();
            err = true;
          }
        }
      } else {
        if (!remove(cpath)) err = true;
        list.pop_back();
      }
    } else {
      list.pop_back();
      err = true;
    }
  }
  return !err;
}


/**
 * Get the path of the current working directory.
 */
std::string File::get_current_directory() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  char buf[PATHBUFSIZ];
  ::DWORD size = ::GetCurrentDirectory(sizeof(buf), buf);
  if (size < 1) return "";
  if (size < sizeof(buf)) return std::string(buf);
  char* lbuf = new char[size];
  ::DWORD nsiz = ::GetCurrentDirectory(size, lbuf);
  if (nsiz < 1 || nsiz >= size) {
    delete[] lbuf;
    return "";
  }
  std::string rbuf(lbuf);
  delete[] lbuf;
  return rbuf;
#else
  _assert_(true);
  char buf[PATHBUFSIZ];
  if (!::getcwd(buf, sizeof(buf))) return "";
  return std::string(buf);
#endif
}


/**
 * Set the current working directory.
 */
bool File::set_current_directory(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  return ::SetCurrentDirectory(path.c_str());
#else
  _assert_(true);
  return ::chdir(path.c_str()) == 0;
#endif
}


/**
 * Synchronize the whole of the file system with the device.
 */
bool File::synchronize_whole() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  return true;
#else
  _assert_(true);
  ::sync();
  return true;
#endif
}



/**
 * Default constructor.
 */
DirStream::DirStream() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  DirStreamCore* core = new DirStreamCore;
  core->dh = NULL;
  opq_ = core;
#else
  _assert_(true);
  DirStreamCore* core = new DirStreamCore;
  core->dh = NULL;
  opq_ = core;
#endif
}


/**
 * Destructor.
 */
DirStream::~DirStream() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  if (core->dh) close();
  delete core;
#else
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  if (core->dh) close();
  delete core;
#endif
}


/**
 * Open a directory.
 */
bool DirStream::open(const std::string& path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (core->dh) return false;
  std::string dpath = path;
  size_t plen = path.size();
  if (plen < 1 || path[plen-1] != File::PATHCHR) dpath.append(File::PATHSTR);
  dpath.append("*");
  ::WIN32_FIND_DATA fbuf;
  ::HANDLE dh = ::FindFirstFile(dpath.c_str(), &fbuf);
  if (!dh || dh == INVALID_HANDLE_VALUE) return false;
  core->dh = dh;
  core->cur = fbuf.cFileName;
  return true;
#else
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (core->dh) return false;
  ::DIR* dh = ::opendir(path.c_str());
  if (!dh) return false;
  core->dh = dh;
  return true;
#endif
}


/**
 * Close the file.
 */
bool DirStream::close() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (!core->dh) return false;
  bool err = false;
  if (!::FindClose(core->dh)) err = true;
  core->dh = NULL;
  core->cur.clear();
  return !err;
#else
  _assert_(true);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (!core->dh) return false;
  bool err = false;
  if (::closedir(core->dh) != 0) err = true;
  core->dh = NULL;
  return !err;
#endif
}


/**
 * Read the next file in the directory.
 */
bool DirStream::read(std::string* path) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(path);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (!core->dh) return false;
  while (core->cur == File::CDIRSTR || core->cur == File::PDIRSTR) {
    ::WIN32_FIND_DATA fbuf;
    if (::FindNextFile(core->dh, &fbuf)) {
      core->cur = fbuf.cFileName;
    } else {
      core->cur.clear();
      return false;
    }
  }
  if (core->cur.empty()) return false;
  path->clear();
  path->append(core->cur);
  ::WIN32_FIND_DATA fbuf;
  if (::FindNextFile(core->dh, &fbuf)) {
    core->cur = fbuf.cFileName;
  } else {
    core->cur.clear();
  }
  return true;
#else
  _assert_(path);
  DirStreamCore* core = (DirStreamCore*)opq_;
  ScopedMutex lock(&core->alock);
  if (!core->dh) return false;
  struct ::dirent *dp;
  do {
    dp = ::readdir(core->dh);
    if (!dp) return false;
  } while (!std::strcmp(dp->d_name, File::CDIRSTR) || !std::strcmp(dp->d_name, File::PDIRSTR));
  path->clear();
  path->append(dp->d_name);
  return true;
#endif
}


/**
 * Set the error message.
 */
static void seterrmsg(FileCore* core, const char* msg) {
  _assert_(core && msg);
  core->errmsg.set((void*)msg);
}


/**
 * Get the path of the WAL file.
 */
static std::string walpath(const std::string& path) {
  _assert_(true);
  return path + File::EXTCHR + WALPATHEXT;
}


/**
 * Write a log message into the WAL file.
 */
static bool walwrite(FileCore *core, int64_t off, size_t size, int64_t base) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core && off >= 0 && off <= FILEMAXSIZ && size <= MEMMAXSIZ && base >= 0);
  bool err = false;
  if (off < base) {
    int64_t diff = base - off;
    if (diff >= (int64_t)size) return true;
    off = base;
    size -= diff;
  }
  int64_t rem = core->trmsiz - off;
  if (rem < 1) return true;
  if (rem < (int64_t)size) size = rem;
  char stack[IOBUFSIZ];
  size_t rsiz = sizeof(int8_t) + sizeof(int64_t) * 2 + size;
  char* rbuf = rsiz > sizeof(stack) ? new char[rsiz] : stack;
  char* wp = rbuf;
  *(wp++) = WALMSGMAGIC;
  int64_t num = hton64(off);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  num = hton64(size);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  core->alock.lock();
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(wp, core->map + off, size);
  } else {
    if (off < core->msiz) {
      int64_t hsiz = core->msiz - off;
      std::memcpy(wp, core->map + off, hsiz);
      off += hsiz;
      wp += hsiz;
      size -= hsiz;
    }
    while (true) {
      int64_t rb = win_pread(core->fh, wp, size, off);
      if (rb >= (int64_t)size) {
        break;
      } else if (rb > 0) {
        wp += rb;
        size -= rb;
        off += rb;
      } else {
        err = true;
        break;
      }
    }
    if (err) {
      seterrmsg(core, "win_pread failed");
      std::memset(wp, 0, size);
    }
  }
  if (!mywrite(core->walfh, core->walsiz, rbuf, rsiz)) {
    seterrmsg(core, "mywrite failed");
    err = true;
  }
  if (core->trhard && !::FlushFileBuffers(core->walfh)) {
    seterrmsg(core, "FlushFileBuffers failed");
    err = true;
  }
  core->walsiz += rsiz;
  if (rbuf != stack) delete[] rbuf;
  core->alock.unlock();
  return !err;
#else
  _assert_(core && off >= 0 && off <= FILEMAXSIZ && size <= MEMMAXSIZ && base >= 0);
  bool err = false;
  if (off < base) {
    int64_t diff = base - off;
    if (diff >= (int64_t)size) return true;
    off = base;
    size -= diff;
  }
  int64_t rem = core->trmsiz - off;
  if (rem < 1) return true;
  if (rem < (int64_t)size) size = rem;
  char stack[IOBUFSIZ];
  size_t rsiz = sizeof(int8_t) + sizeof(int64_t) * 2 + size;
  char* rbuf = rsiz > sizeof(stack) ? new char[rsiz] : stack;
  char* wp = rbuf;
  *(wp++) = WALMSGMAGIC;
  int64_t num = hton64(off);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  num = hton64(size);
  std::memcpy(wp, &num, sizeof(num));
  wp += sizeof(num);
  core->alock.lock();
  int64_t end = off + size;
  if (end <= core->msiz) {
    std::memcpy(wp, core->map + off, size);
  } else {
    if (off < core->msiz) {
      int64_t hsiz = core->msiz - off;
      std::memcpy(wp, core->map + off, hsiz);
      off += hsiz;
      wp += hsiz;
      size -= hsiz;
    }
    while (true) {
      ssize_t rb = ::pread(core->fd, wp, size, off);
      if (rb >= (ssize_t)size) {
        break;
      } else if (rb > 0) {
        wp += rb;
        size -= rb;
        off += rb;
      } else if (rb == -1) {
        if (errno != EINTR) {
          err = true;
          break;
        }
      } else {
        err = true;
        break;
      }
    }
    if (err) {
      seterrmsg(core, "pread failed");
      std::memset(wp, 0, size);
    }
  }
  if (!mywrite(core->walfd, core->walsiz, rbuf, rsiz)) {
    seterrmsg(core, "mywrite failed");
    err = true;
  }
  if (core->trhard && ::fsync(core->walfd) != 0) {
    seterrmsg(core, "fsync failed");
    err = true;
  }
  core->walsiz += rsiz;
  if (rbuf != stack) delete[] rbuf;
  core->alock.unlock();
  return !err;
#endif
}


/**
 * Apply log messages in the WAL file.
 */
static bool walapply(FileCore* core) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core);
  bool err = false;
  char buf[IOBUFSIZ];
  int64_t hsiz = sizeof(WALMAGICDATA) + sizeof(int64_t);
  ::LARGE_INTEGER li;
  if (!::GetFileSizeEx(core->walfh, &li)) {
    seterrmsg(core, "GetFileSizeEx failed");
    return false;
  }
  int64_t rem = li.QuadPart;
  if (rem < hsiz) {
    seterrmsg(core, "too short WAL file");
    return false;
  }
  li.QuadPart = 0;
  if (!::SetFilePointerEx(core->walfh, li, NULL, FILE_BEGIN)) {
    seterrmsg(core, "SetFilePointerEx failed");
    return false;
  }
  if (!myread(core->walfh, buf, hsiz)) {
    seterrmsg(core, "myread failed");
    return false;
  }
  if (*buf == 0) return true;
  if (std::memcmp(buf, WALMAGICDATA, sizeof(WALMAGICDATA))) {
    seterrmsg(core, "invalid magic data of WAL");
    return false;
  }
  int64_t osiz;
  std::memcpy(&osiz, buf + sizeof(WALMAGICDATA), sizeof(osiz));
  osiz = ntoh64(osiz);
  rem -= hsiz;
  hsiz = sizeof(uint8_t) + sizeof(int64_t) * 2;
  std::vector<WALMessage> msgs;
  int64_t end = 0;
  while (rem >= hsiz) {
    if (!myread(core->walfh, buf, hsiz)) {
      seterrmsg(core, "myread failed");
      err = true;
      break;
    }
    if (*buf == 0) {
      rem = 0;
      break;
    }
    rem -= hsiz;
    char* rp = buf;
    if (*(uint8_t*)(rp++) != WALMSGMAGIC) {
      seterrmsg(core, "invalid magic data of WAL message");
      err = true;
      break;
    }
    if (rem > 0) {
      int64_t off;
      std::memcpy(&off, rp, sizeof(off));
      off = ntoh64(off);
      rp += sizeof(off);
      int64_t size;
      std::memcpy(&size, rp, sizeof(size));
      size = ntoh64(size);
      rp += sizeof(size);
      if (off < 0 || size < 0) {
        seterrmsg(core, "invalid meta data of WAL message");
        err = true;
        break;
      }
      if (rem < size) {
        seterrmsg(core, "too short WAL message");
        err = true;
        break;
      }
      char* rbuf = size > (int64_t)sizeof(buf) ? new char[size] : buf;
      if (!myread(core->walfh, rbuf, size)) {
        seterrmsg(core, "myread failed");
        if (rbuf != buf) delete[] rbuf;
        err = true;
        break;
      }
      rem -= size;
      WALMessage msg = { off, std::string(rbuf, size) };
      msgs.push_back(msg);
      if (off + size > end) end = off + size;
      if (rbuf != buf) delete[] rbuf;
    }
  }
  if (rem != 0) {
    if (!myread(core->walfh, buf, 1)) {
      seterrmsg(core, "myread failed");
      err = true;
    } else if (*buf != 0) {
      seterrmsg(core, "too few messages of WAL");
      err = true;
    }
  }
  if (end > core->msiz) end = core->msiz;
  if (core->psiz < end && win_ftruncate(core->fh, end) != 0) {
    seterrmsg(core, "win_ftruncate failed");
    err = true;
  }
  for (int64_t i = (int64_t)msgs.size() - 1; i >= 0; i--) {
    const WALMessage& msg = msgs[i];
    int64_t off = msg.off;
    const char* rbuf = msg.body.c_str();
    size_t size = msg.body.size();
    int64_t end = off + size;
    if (end <= core->msiz) {
      std::memcpy(core->map + off, rbuf, size);
    } else {
      if (off < core->msiz) {
        size_t hsiz = core->msiz - off;
        std::memcpy(core->map + off, rbuf, hsiz);
        off += hsiz;
        rbuf += hsiz;
        size -= hsiz;
      }
      while (true) {
        int64_t wb = win_pwrite(core->fh, rbuf, size, off);
        if (wb >= (int64_t)size) {
          break;
        } else if (wb > 0) {
          rbuf += wb;
          size -= wb;
          off += wb;
        } else if (wb == -1) {
          seterrmsg(core, "win_pwrite failed");
          err = true;
          break;
        } else if (size > 0) {
          seterrmsg(core, "pwrite failed");
          err = true;
          break;
        }
      }
    }
  }
  if (win_ftruncate(core->fh, osiz) == 0) {
    core->lsiz = osiz;
    core->psiz = osiz;
  } else {
    seterrmsg(core, "win_ftruncate failed");
    err = true;
  }
  return !err;
#else
  _assert_(core);
  bool err = false;
  char buf[IOBUFSIZ];
  int64_t hsiz = sizeof(WALMAGICDATA) + sizeof(int64_t);
  int64_t rem = ::lseek(core->walfd, 0, SEEK_END);
  if (rem < hsiz) {
    seterrmsg(core, "lseek failed");
    return false;
  }
  if (::lseek(core->walfd, 0, SEEK_SET) != 0) {
    seterrmsg(core, "lseek failed");
    return false;
  }
  if (!myread(core->walfd, buf, hsiz)) {
    seterrmsg(core, "myread failed");
    return false;
  }
  if (*buf == 0) return true;
  if (std::memcmp(buf, WALMAGICDATA, sizeof(WALMAGICDATA))) {
    seterrmsg(core, "invalid magic data of WAL");
    return false;
  }
  int64_t osiz;
  std::memcpy(&osiz, buf + sizeof(WALMAGICDATA), sizeof(osiz));
  osiz = ntoh64(osiz);
  rem -= hsiz;
  hsiz = sizeof(uint8_t) + sizeof(int64_t) * 2;
  std::vector<WALMessage> msgs;
  int64_t end = 0;
  while (rem >= hsiz) {
    if (!myread(core->walfd, buf, hsiz)) {
      seterrmsg(core, "myread failed");
      err = true;
      break;
    }
    if (*buf == 0) {
      rem = 0;
      break;
    }
    rem -= hsiz;
    char* rp = buf;
    if (*(uint8_t*)(rp++) != WALMSGMAGIC) {
      seterrmsg(core, "invalid magic data of WAL message");
      err = true;
      break;
    }
    if (rem > 0) {
      int64_t off;
      std::memcpy(&off, rp, sizeof(off));
      off = ntoh64(off);
      rp += sizeof(off);
      int64_t size;
      std::memcpy(&size, rp, sizeof(size));
      size = ntoh64(size);
      rp += sizeof(size);
      if (off < 0 || size < 0) {
        seterrmsg(core, "invalid meta data of WAL message");
        err = true;
        break;
      }
      if (rem < size) {
        seterrmsg(core, "too short WAL message");
        err = true;
        break;
      }
      char* rbuf = size > (int64_t)sizeof(buf) ? new char[size] : buf;
      if (!myread(core->walfd, rbuf, size)) {
        seterrmsg(core, "myread failed");
        if (rbuf != buf) delete[] rbuf;
        err = true;
        break;
      }
      rem -= size;
      WALMessage msg = { off, std::string(rbuf, size) };
      msgs.push_back(msg);
      if (off + size > end) end = off + size;
      if (rbuf != buf) delete[] rbuf;
    }
  }
  if (rem != 0) {
    if (!myread(core->walfd, buf, 1)) {
      seterrmsg(core, "myread failed");
      err = true;
    } else if (*buf != 0) {
      seterrmsg(core, "too few messages of WAL");
      err = true;
    }
  }
  if (end > core->msiz) end = core->msiz;
  if (core->psiz < end && ::ftruncate(core->fd, end) != 0) {
    seterrmsg(core, "ftruncate failed");
    err = true;
  }
  for (int64_t i = (int64_t)msgs.size() - 1; i >= 0; i--) {
    const WALMessage& msg = msgs[i];
    int64_t off = msg.off;
    const char* rbuf = msg.body.c_str();
    size_t size = msg.body.size();
    int64_t end = off + size;
    if (end <= core->msiz) {
      std::memcpy(core->map + off, rbuf, size);
    } else {
      if (off < core->msiz) {
        size_t hsiz = core->msiz - off;
        std::memcpy(core->map + off, rbuf, hsiz);
        off += hsiz;
        rbuf += hsiz;
        size -= hsiz;
      }
      while (true) {
        ssize_t wb = ::pwrite(core->fd, rbuf, size, off);
        if (wb >= (ssize_t)size) {
          break;
        } else if (wb > 0) {
          rbuf += wb;
          size -= wb;
          off += wb;
        } else if (wb == -1) {
          if (errno != EINTR) {
            seterrmsg(core, "pwrite failed");
            err = true;
            break;
          }
        } else if (size > 0) {
          seterrmsg(core, "pwrite failed");
          err = true;
          break;
        }
      }
    }
  }
  if (::ftruncate(core->fd, osiz) == 0) {
    core->lsiz = osiz;
    core->psiz = osiz;
  } else {
    seterrmsg(core, "ftruncate failed");
    err = true;
  }
  return !err;
#endif
}


/**
 * Write data into a file.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static bool mywrite(::HANDLE fh, int64_t off, const void* buf, size_t size) {
  _assert_(off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  while (true) {
    int64_t wb = win_pwrite(fh, buf, size, off);
    if (wb >= (int64_t)size) {
      return true;
    } else if (wb > 0) {
      buf = (char*)buf + wb;
      size -= wb;
      off += wb;
    } else if (wb == -1) {
      return false;
    } else if (size > 0) {
      return false;
    }
  }
  return true;
}
#else
static bool mywrite(int32_t fd, int64_t off, const void* buf, size_t size) {
  _assert_(fd >= 0 && off >= 0 && off <= FILEMAXSIZ && buf && size <= MEMMAXSIZ);
  while (true) {
    ssize_t wb = ::pwrite(fd, buf, size, off);
    if (wb >= (ssize_t)size) {
      return true;
    } else if (wb > 0) {
      buf = (char*)buf + wb;
      size -= wb;
      off += wb;
    } else if (wb == -1) {
      if (errno != EINTR) return false;
    } else if (size > 0) {
      return false;
    }
  }
  return true;
}
#endif


/**
 * Read data from a file.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static size_t myread(::HANDLE fh, void* buf, size_t size) {
  _assert_(buf && size <= MEMMAXSIZ);
  while (true) {
    int64_t rb = win_read(fh, buf, size);
    if (rb >= (int64_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
    } else if (rb == -1) {
      return false;
    } else if (size > 0) {
      return false;
    }
  }
  return true;
}
#else
static size_t myread(int32_t fd, void* buf, size_t size) {
  _assert_(fd >= 0 && buf && size <= MEMMAXSIZ);
  while (true) {
    ssize_t rb = ::read(fd, buf, size);
    if (rb >= (ssize_t)size) {
      break;
    } else if (rb > 0) {
      buf = (char*)buf + rb;
      size -= rb;
    } else if (rb == -1) {
      if (errno != EINTR) return false;
    } else if (size > 0) {
      return false;
    }
  }
  return true;
}
#endif


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
/**
 * Emulate the pwrite call
 */
static int64_t win_pwrite(::HANDLE fh, const void* buf, size_t count, int64_t offset) {
  _assert_(buf && count <= MEMMAXSIZ && offset >= 0 && offset <= FILEMAXSIZ);
  ::DWORD wb;
  ::LARGE_INTEGER li;
  li.QuadPart = offset;
  ::OVERLAPPED ol;
  ol.Offset = li.LowPart;
  ol.OffsetHigh = li.HighPart;
  ol.hEvent = NULL;
  if (!::WriteFile(fh, buf, count, &wb, &ol)) return -1;
  return wb;
}
#endif


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
/**
 * Emulate the pread call
 */
static int64_t win_pread(::HANDLE fh, void* buf, size_t count, int64_t offset) {
  _assert_(buf && count <= MEMMAXSIZ && offset >= 0 && offset <= FILEMAXSIZ);
  ::DWORD rb;
  ::LARGE_INTEGER li;
  li.QuadPart = offset;
  ::OVERLAPPED ol;
  ol.Offset = li.LowPart;
  ol.OffsetHigh = li.HighPart;
  ol.hEvent = NULL;
  if (!::ReadFile(fh, buf, count, &rb, &ol)) return -1;
  return rb;
}
#endif


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
/**
 * Emulate the write call
 */
static int64_t win_write(::HANDLE fh, const void* buf, size_t count) {
  _assert_(buf && count <= MEMMAXSIZ);
  ::DWORD wb;
  if (!::WriteFile(fh, buf, count, &wb, NULL)) return -1;
  return wb;
}
#endif


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
/**
 * Emulate the read call
 */
static int64_t win_read(::HANDLE fh, void* buf, size_t count) {
  _assert_(buf && count <= MEMMAXSIZ);
  ::DWORD rb;
  if (!::ReadFile(fh, buf, count, &rb, NULL)) return -1;
  return rb;
}
#endif


#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
/**
 * Emulate the ftruncate call
 */
static int32_t win_ftruncate(::HANDLE fh, int64_t length) {
  _assert_(length >= 0 && length <= FILEMAXSIZ);
  ::LARGE_INTEGER li;
  li.QuadPart = length;
  if (!::SetFilePointerEx(fh, li, NULL, FILE_BEGIN)) return -1;
  if (!::SetEndOfFile(fh) && ::GetLastError() != 1224) return -1;
  return 0;
}
#endif


}                                        // common namespace

// END OF FILE
