/*************************************************************************************************
 * Data compressor and decompressor
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


#ifndef _KCCOMPRESS_H                    // duplication check
#define _KCCOMPRESS_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>

namespace kyotocabinet {                 // common namespace


/**
 * Interfrace of data compression and decompression.
 */
class Compressor {
 public:
  /**
   * Destructor.
   */
  virtual ~Compressor() {}
  /**
   * Compress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because the region of the return value is allocated with the the new[] operator, it
   * should be released with the delete[] operator when it is no longer in use.
   */
  virtual char* compress(const void* buf, size_t size, size_t* sp) = 0;
  /**
   * Decompress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a C-style string.  Because the region of the
   * return value is allocated with the the new[] operator, it should be released with the
   * delete[] operator when it is no longer in use.
   */
  virtual char* decompress(const void* buf, size_t size, size_t* sp) = 0;
};


/**
 * ZLIB compressor.
 */
class ZLIB {
 public:
  /**
   * Compression modes.
   */
  enum Mode {
    RAW,                                 ///< without any checksum
    DEFLATE,                             ///< with Adler32 checksum
    GZIP                                 ///< with CRC32 checksum and various meta data
  };
  /**
   * Compress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because the region of the return value is allocated with the the new[] operator, it
   * should be released with the delete[] operator when it is no longer in use.
   */
  static char* compress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Decompress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a C-style string.  Because the region of the
   * return value is allocated with the the new[] operator, it should be released with the
   * delete[] operator when it is no longer in use.
   */
  static char* decompress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Calculate the CRC32 checksum of a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param seed the cyclic seed value.
   * @return the CRC32 checksum.
   */
  static uint32_t calculate_crc(const void* buf, size_t size, uint32_t seed = 0);
};


/**
 * LZO compressor.
 */
class LZO {
 public:
  /**
   * Compression modes.
   */
  enum Mode {
    RAW,                                 ///< without any checksum
    CRC                                  ///< with CRC32 checksum
  };
  /**
   * Compress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because the region of the return value is allocated with the the new[] operator, it
   * should be released with the delete[] operator when it is no longer in use.
   */
  static char* compress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Decompress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a C-style string.  Because the region of the
   * return value is allocated with the the new[] operator, it should be released with the
   * delete[] operator when it is no longer in use.
   */
  static char* decompress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Calculate the CRC32 checksum of a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param seed the cyclic seed value.
   * @return the CRC32 checksum.
   */
  static uint32_t calculate_crc(const void* buf, size_t size, uint32_t seed = 0);
};


/**
 * LZMA compressor.
 */
class LZMA {
 public:
  /**
   * Compression modes.
   */
  enum Mode {
    RAW,                                 ///< without any checksum
    CRC,                                 ///< with CRC32 checksum
    SHA                                  ///< with SHA256 checksum
  };
  /**
   * Compress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because the region of the return value is allocated with the the new[] operator, it
   * should be released with the delete[] operator when it is no longer in use.
   */
  static char* compress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Decompress a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @param mode the compression mode.
   * @return the pointer to the result data, or NULL on failure.
   * @note Because an additional zero code is appended at the end of the region of the return
   * value, the return value can be treated as a C-style string.  Because the region of the
   * return value is allocated with the the new[] operator, it should be released with the
   * delete[] operator when it is no longer in use.
   */
  static char* decompress(const void* buf, size_t size, size_t* sp, Mode mode = RAW);
  /**
   * Calculate the CRC32 checksum of a serial data.
   * @param buf the input buffer.
   * @param size the size of the input buffer.
   * @param seed the cyclic seed value.
   * @return the CRC32 checksum.
   */
  static uint32_t calculate_crc(const void* buf, size_t size, uint32_t seed = 0);
};


/**
 * Compressor with ZLIB.
 */
template <ZLIB::Mode MODE>
class ZLIBCompressor : public Compressor {
 private:
  /**
   * Compress a serial data.
   */
  char* compress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return ZLIB::compress(buf, size, sp, MODE);
  }
  /**
   * Decompress a serial data.
   */
  char* decompress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return ZLIB::decompress(buf, size, sp, MODE);
  }
};


/**
 * Compressor with LZO.
 */
template <LZO::Mode MODE>
class LZOCompressor : public Compressor {
 private:
  /**
   * Compress a serial data.
   */
  char* compress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return LZO::compress(buf, size, sp, MODE);
  }
  /**
   * Decompress a serial data.
   */
  char* decompress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return LZO::decompress(buf, size, sp, MODE);
  }
};


/**
 * Compressor with LZMA.
 */
template <LZMA::Mode MODE>
class LZMACompressor : public Compressor {
 private:
  /**
   * Compress a serial data.
   */
  char* compress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return LZMA::compress(buf, size, sp, MODE);
  }
  /**
   * Decompress a serial data.
   */
  char* decompress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    return LZMA::decompress(buf, size, sp, MODE);
  }
};


/**
 * Compressor with the Arcfour cipher.
 */
class ArcfourCompressor : public Compressor {
 public:
  /**
   * Constructor.
   */
  ArcfourCompressor() : kbuf_(NULL), ksiz_(0), comp_(NULL), salt_(0), cycle_(false) {
    _assert_(true);
    kbuf_ = new char[1];
    ksiz_ = 0;
  }
  /**
   * Destructor.
   */
  ~ArcfourCompressor() {
    _assert_(true);
    delete[] kbuf_;
  }
  /**
   * Set the cipher key.
   * @param kbuf the pointer to the region of the cipher key.
   * @param ksiz the size of the region of the cipher key.
   */
  void set_key(const void* kbuf, size_t ksiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    delete[] kbuf_;
    if (ksiz > NUMBUFSIZ) ksiz = NUMBUFSIZ;
    kbuf_ = new char[ksiz];
    std::memcpy(kbuf_, kbuf, ksiz);
    ksiz_ = ksiz;
  }
  /**
   * Set an additional data compressor.
   * @param comp the additional data data compressor.
   */
  void set_compressor(Compressor* comp) {
    _assert_(comp);
    comp_ = comp;
  }
  /**
   * Begin the cycle of ciper salt.
   * @param salt the additional cipher salt.
   */
  void begin_cycle(uint64_t salt = 0) {
    salt_ = salt;
    cycle_ = true;
  }
 private:
  /**
   * Compress a serial data.
   */
  char* compress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    uint64_t salt = cycle_ ? salt_.add(1) : 0;
    char kbuf[NUMBUFSIZ*2];
    writefixnum(kbuf, salt, sizeof(salt));
    std::memcpy(kbuf + sizeof(salt), kbuf_, ksiz_);
    char* tbuf = NULL;
    if (comp_) {
      tbuf = comp_->compress(buf, size, &size);
      if (!tbuf) return NULL;
      buf = tbuf;
    }
    size_t zsiz = sizeof(salt) + size;
    char* zbuf = new char[zsiz];
    writefixnum(zbuf, salt, sizeof(salt));
    arccipher(buf, size, kbuf, sizeof(salt) + ksiz_, zbuf + sizeof(salt));
    delete[] tbuf;
    if (cycle_) {
      size_t range = zsiz - sizeof(salt);
      if (range > (size_t)INT8MAX) range = INT8MAX;
      salt_.add(hashmurmur(zbuf + sizeof(salt), range) << 32);
    }
    *sp = zsiz;
    return zbuf;
  }
  /**
   * Decompress a serial data.
   */
  char* decompress(const void* buf, size_t size, size_t* sp) {
    _assert_(buf && size <= MEMMAXSIZ && sp);
    if (size < sizeof(uint64_t)) return NULL;
    char kbuf[NUMBUFSIZ*2];
    std::memcpy(kbuf, buf, sizeof(uint64_t));
    std::memcpy(kbuf + sizeof(uint64_t), kbuf_, ksiz_);
    buf = (char*)buf + sizeof(uint64_t);
    size -= sizeof(uint64_t);
    char* zbuf = new char[size];
    arccipher(buf, size, kbuf, sizeof(uint64_t) + ksiz_, zbuf);
    if (comp_) {
      char* tbuf = comp_->decompress(zbuf, size, &size);
      delete[] zbuf;
      if (!tbuf) return NULL;
      zbuf = tbuf;
    }
    *sp = size;
    return zbuf;
  }
  /** The pointer to the key. */
  char* kbuf_;
  /** The size of the key. */
  size_t ksiz_;
  /** The data compressor. */
  Compressor* comp_;
  /** The cipher salt. */
  AtomicInt64 salt_;
  /** The flag of the salt cycle */
  bool cycle_;
};


/**
 * Prepared pointer of the compressor with ZLIB raw mode.
 */
extern ZLIBCompressor<ZLIB::RAW>* const ZLIBRAWCOMP;


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
