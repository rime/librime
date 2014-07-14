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


#ifndef _KCFILE_H                        // duplication check
#define _KCFILE_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>

namespace kyotocabinet {                 // common namespace


/**
 * Filesystem abstraction.
 */
class File {
 public:
  struct Status;
 public:
  /** Path delimiter character. */
  static const char PATHCHR;
  /** Path delimiter string. */
  static const char* const PATHSTR;
  /** Extension delimiter character. */
  static const char EXTCHR;
  /** Extension delimiter string. */
  static const char* const EXTSTR;
  /** Current directory string. */
  static const char* const CDIRSTR;
  /** Parent directory string. */
  static const char* const PDIRSTR;
  /**
   * Status information.
   */
  struct Status {
    bool isdir;                          ///< whether directory or not
    int64_t size;                        ///< file size
    int64_t mtime;                       ///< last modified time
  };
  /**
   * Open modes.
   */
  enum OpenMode {
    OREADER = 1 << 0,                    ///< open as a reader
    OWRITER = 1 << 1,                    ///< open as a writer
    OCREATE = 1 << 2,                    ///< writer creating
    OTRUNCATE = 1 << 3,                  ///< writer truncating
    ONOLOCK = 1 << 4,                    ///< open without locking
    OTRYLOCK = 1 << 5                    ///< lock without blocking
  };
  /**
   * Default constructor.
   */
  explicit File();
  /**
   * Destructor.
   * @note If the file is not closed, it is closed implicitly.
   */
  ~File();
  /**
   * Get the last happened error information.
   * @return the last happened error information.
   */
  const char* error() const;
  /**
   * Open a file.
   * @param path the path of a file.
   * @param mode the connection mode.  File::OWRITER as a writer, File::OREADER as a reader.
   * The following may be added to the writer mode by bitwise-or: File::OCREATE, which means it
   * creates a new file if the file does not exist, File::OTRUNCATE, which means it creates a
   * new file regardless if the file exists.  The following may be added to both of the reader
   * mode and the writer mode by bitwise-or: File::ONOLOCK, which means it opens the file
   * without file locking, File::TRYLOCK, which means locking is performed without blocking.
   * @param msiz the size of the internal memory-mapped region.
   * @return true on success, or false on failure.
   */
  bool open(const std::string& path, uint32_t mode = OWRITER | OCREATE, int64_t msiz = 0);
  /**
   * Close the file.
   * @return true on success, or false on failure.
   */
  bool close();
  /**
   * Write data.
   * @param off the offset of the destination.
   * @param buf the pointer to the data region.
   * @param size the size of the data region.
   * @return true on success, or false on failure.
   */
  bool write(int64_t off, const void* buf, size_t size);
  /**
   * Write data.
   * @note Equal to the original File::write method except that the sigunature is different.
   */
  bool write(int64_t off, const std::string& str) {
    _assert_(off >= 0);
    return write(off, str.c_str(), str.size());
  }
  /**
   * Write data with assuring the region does not spill from the file size.
   * @param off the offset of the destination.
   * @param buf the pointer to the data region.
   * @param size the size of the data region.
   * @return true on success, or false on failure.
   */
  bool write_fast(int64_t off, const void* buf, size_t size);
  /**
   * Write data with assuring the region does not spill from the file size.
   * @note Equal to the original File::write_fast method except that the sigunature is different.
   */
  bool write_fast(int64_t off, const std::string& str) {
    _assert_(off >= 0);
    return write_fast(off, str.c_str(), str.size());
  }
  /**
   * Write data at the end of the file.
   * @param buf the pointer to the data region.
   * @param size the size of the data region.
   * @return true on success, or false on failure.
   */
  bool append(const void* buf, size_t size);
  /**
   * Write data at the end of the file.
   * @note Equal to the original File::append method except that the sigunature is different.
   */
  bool append(const std::string& str) {
    _assert_(true);
    return append(str.c_str(), str.size());
  }
  /**
   * Read data.
   * @param off the offset of the source.
   * @param buf the pointer to the destination region.
   * @param size the size of the data to be read.
   * @return true on success, or false on failure.
   */
  bool read(int64_t off, void* buf, size_t size);
  /**
   * Read data.
   * @note Equal to the original File::read method except that the sigunature is different.
   */
  bool read(int64_t off, std::string* buf, size_t size) {
    _assert_(off >= 0 && buf);
    char* tbuf = new char[size];
    if (!read(off, tbuf, size)) {
      delete[] tbuf;
      return false;
    }
    buf->append(std::string(tbuf, size));
    delete[] tbuf;
    return true;
  }
  /**
   * Read data with assuring the region does not spill from the file size.
   * @param off the offset of the source.
   * @param buf the pointer to the destination region.
   * @param size the size of the data to be read.
   * @return true on success, or false on failure.
   */
  bool read_fast(int64_t off, void* buf, size_t size);
  /**
   * Read data.
   * @note Equal to the original File::read method except that the sigunature is different.
   */
  bool read_fast(int64_t off, std::string* buf, size_t size) {
    _assert_(off >= 0 && buf);
    char* tbuf = new char[size];
    if (!read_fast(off, tbuf, size)) {
      delete[] tbuf;
      return false;
    }
    buf->append(std::string(tbuf, size));
    delete[] tbuf;
    return true;
  }
  /**
   * Truncate the file.
   * @param size the new size of the file.
   * @return true on success, or false on failure.
   */
  bool truncate(int64_t size);
  /**
   * Synchronize updated contents with the file and the device.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool synchronize(bool hard);
  /**
   * Refresh the internal state for update by others.
   * @return true on success, or false on failure.
   */
  bool refresh();
  /**
   * Begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @param off the beginning offset of the guarded region
   * @return true on success, or false on failure.
   */
  bool begin_transaction(bool hard, int64_t off);
  /**
   * End transaction.
   * @param commit true to commit the transaction, or false to abort the transaction.
   * @return true on success, or false on failure.
   */
  bool end_transaction(bool commit);
  /**
   * Write a WAL message of transaction explicitly.
   * @param off the offset of the source.
   * @param size the size of the data to be read.
   * @return true on success, or false on failure.
   */
  bool write_transaction(int64_t off, size_t size);
  /**
   * Get the size of the file.
   * @return the size of the file, or 0 on failure.
   */
  int64_t size() const;
  /**
   * Get the path of the file.
   * @return the path of the file in bytes, or an empty string on failure.
   */
  std::string path() const;
  /**
   * Check whether the file was recovered or not.
   * @return true if recovered, or false if not.
   */
  bool recovered() const;
  /**
   * Read the whole data from a file.
   * @param path the path of a file.
   * @param sp the pointer to the variable into which the size of the region of the return value
   * is assigned.
   * @param limit the limit length to read.  If it is nagative, no limit is specified.
   * @return the pointer to the region of the read data, or NULL on failure.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a C-style string.  Because the region of the
   * return value is allocated with the the new[] operator, it should be released with the
   * delete[] operator when it is no longer in use.
   */
  static char* read_file(const std::string& path, int64_t* sp, int64_t limit = -1);
  /**
   * Write the whole data into a file.
   * @param path the path of a file.
   * @param buf the data buffer to write.
   * @param size the size of the data buffer.
   * @return true on success, or false on failure.
   * @note The existing file corresponding to the path is overwritten.  If no file corresponds
   * to the path, a new file is created.
   */
  static bool write_file(const std::string& path, const char* buf, int64_t size);
  /**
   * Get the status information of a file.
   * @param path the path of a file.
   * @param buf a structure of status information.  If it is NULL, it is omitted.
   * @return true on success, or false on failure.
   */
  static bool status(const std::string& path, Status* buf = NULL);
  /**
   * Get the absolute path of a file.
   * @param path the path of a file.
   * @return the absolute path of the file, or an empty string on failure.
   */
  static std::string absolute_path(const std::string& path);
  /**
   * Remove a file.
   * @param path the path of a file.
   * @return true on success, or false on failure.
   */
  static bool remove(const std::string& path);
  /**
   * Change the name or location of a file.
   * @param opath the old path of a file.
   * @param npath the new path of a file.
   * @return true on success, or false on failure.
   */
  static bool rename(const std::string& opath, const std::string& npath);
  /**
   * Read a directory.
   * @param path the path of a directory.
   * @param strvec a string list to contain the result.
   * @return true on success, or false on failure.
   */
  static bool read_directory(const std::string& path, std::vector<std::string>* strvec);
  /**
   * Make a directory.
   * @param path the path of a directory.
   * @return true on success, or false on failure.
   */
  static bool make_directory(const std::string& path);
  /**
   * Remove a directory.
   * @param path the path of a directory.
   * @return true on success, or false on failure.
   */
  static bool remove_directory(const std::string& path);
  /**
   * Remove a file or a directory recursively.
   * @param path the path of a file or a directory.
   * @return true on success, or false on failure.
   */
  static bool remove_recursively(const std::string& path);
  /**
   * Get the path of the current working directory.
   * @return the path of the current working directory, or an empty string on failure.
   */
  static std::string get_current_directory();
  /**
   * Set the current working directory.
   * @param path the path of a directory.
   * @return true on success, or false on failure.
   */
  static bool set_current_directory(const std::string& path);
  /**
   * Synchronize the whole of the file system with the device.
   * @return true on success, or false on failure.
   */
  static bool synchronize_whole();
 private:
  /** Dummy constructor to forbid the use. */
  File(const File&);
  /** Dummy Operator to forbid the use. */
  File& operator =(const File&);
  /** Opaque pointer. */
  void* opq_;
};


/**
 * Directory stream abstraction.
 */
class DirStream {
 public:
  /**
   * Default constructor.
   */
  explicit DirStream();
  /**
   * Destructor.
   * @note If the file is not closed, it is closed implicitly.
   */
  ~DirStream();
  /**
   * Open a directory.
   * @param path the path of a directory.
   * @return true on success, or false on failure.
   */
  bool open(const std::string& path);
  /**
   * Close the file.
   * @return true on success, or false on failure.
   */
  bool close();
  /**
   * Read the next file in the directory.
   * @param path a string to store the file path.
   * @return true on success, or false on failure.
   */
  bool read(std::string* path);
 private:
  /** Dummy constructor to forbid the use. */
  DirStream(const DirStream&);
  /** Dummy Operator to forbid the use. */
  DirStream& operator =(const DirStream&);
  /** Opaque pointer. */
  void* opq_;
};


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
