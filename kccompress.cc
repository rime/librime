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


#include "kccompress.h"
#include "myconf.h"

#if _KC_ZLIB
extern "C" {
#include <zlib.h>
}
#endif

#if _KC_LZO
extern "C" {
#include <lzo/lzo1x.h>
}
#endif

#if _KC_LZMA
extern "C" {
#include <lzma.h>
}
#endif

namespace kyotocabinet {                 // common namespace


/**
 * Compress a serial data.
 */
char* ZLIB::compress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_ZLIB
  _assert_(buf && size <= MEMMAXSIZ && sp);
  z_stream zs;
  zs.zalloc = Z_NULL;
  zs.zfree = Z_NULL;
  zs.opaque = Z_NULL;
  switch (mode) {
    default: {
      if (deflateInit2(&zs, 6, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY) != Z_OK) return NULL;
      break;
    }
    case DEFLATE: {
      if (deflateInit2(&zs, 6, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY) != Z_OK) return NULL;
      break;
    }
    case GZIP: {
      if (deflateInit2(&zs, 6, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY) != Z_OK) return NULL;
      break;
    }
  }
  const char* rp = (const char*)buf;
  size_t zsiz = size + size / 8 + 32;
  char* zbuf = new char[zsiz+1];
  char* wp = zbuf;
  zs.next_in = (Bytef*)rp;
  zs.avail_in = size;
  zs.next_out = (Bytef*)wp;
  zs.avail_out = zsiz;
  if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
    delete[] zbuf;
    deflateEnd(&zs);
    return NULL;
  }
  deflateEnd(&zs);
  zsiz -= zs.avail_out;
  zbuf[zsiz] = '\0';
  if (mode == RAW) zsiz++;
  *sp = zsiz;
  return zbuf;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  char* zbuf = new char[size+2];
  char* wp = zbuf;
  *(wp++) = 'z';
  *(wp++) = (uint8_t)mode;
  std::memcpy(wp, buf, size);
  *sp = size + 2;
  return zbuf;
#endif
}


/**
 * Decompress a serial data.
 */
char* ZLIB::decompress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_ZLIB
  _assert_(buf && size <= MEMMAXSIZ && sp);
  size_t zsiz = size * 8 + 32;
  while (true) {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    switch (mode) {
      default: {
        if (inflateInit2(&zs, -15) != Z_OK) return NULL;
        break;
      }
      case DEFLATE: {
        if (inflateInit2(&zs, 15) != Z_OK) return NULL;
        break;
      }
      case GZIP: {
        if (inflateInit2(&zs, 15 + 16) != Z_OK) return NULL;
        break;
      }
    }
    char* zbuf = new char[zsiz+1];
    zs.next_in = (Bytef*)buf;
    zs.avail_in = size;
    zs.next_out = (Bytef*)zbuf;
    zs.avail_out = zsiz;
    int32_t rv = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (rv == Z_STREAM_END) {
      zsiz -= zs.avail_out;
      zbuf[zsiz] = '\0';
      *sp = zsiz;
      return zbuf;
    } else if (rv == Z_BUF_ERROR) {
      delete[] zbuf;
      zsiz *= 2;
    } else {
      delete[] zbuf;
      break;
    }
  }
  return NULL;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  if (size < 2 || ((char*)buf)[0] != 'z' || ((char*)buf)[1] != (uint8_t)mode) return NULL;
  buf = (char*)buf + 2;
  size -= 2;
  char* zbuf = new char[size+1];
  std::memcpy(zbuf, buf, size);
  zbuf[size] = '\0';
  *sp = size;
  return zbuf;
#endif
}


/**
 * Calculate the CRC32 checksum of a serial data.
 */
uint32_t ZLIB::calculate_crc(const void* buf, size_t size, uint32_t seed) {
#if _KC_ZLIB
  _assert_(buf && size <= MEMMAXSIZ);
  return crc32(seed, (unsigned char*)buf, size);
#else
  _assert_(buf && size <= MEMMAXSIZ);
  return 0;
#endif
}


/**
 * Hidden resources for LZO.
 */
#if _KC_LZO
static int32_t lzo_init_func() {
  if (lzo_init() != LZO_E_OK) throw std::runtime_error("lzo_init");
  return 0;
}
int32_t lzo_init_var = lzo_init_func();
#endif


/**
 * Compress a serial data.
 */
char* LZO::compress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_LZO
  _assert_(buf && size <= MEMMAXSIZ && sp);
  char* zbuf = new char[size+size/16+80];
  lzo_uint zsiz;
  char wrkmem[LZO1X_1_MEM_COMPRESS];
  if (lzo1x_1_compress((lzo_bytep)buf, size, (lzo_bytep)zbuf, &zsiz, wrkmem) != LZO_E_OK) {
    delete[] zbuf;
    return NULL;
  }
  if (mode == CRC) {
    uint32_t hash = lzo_crc32(0, (const lzo_bytep)zbuf, zsiz);
    writefixnum(zbuf + zsiz, hash, sizeof(hash));
    zsiz += sizeof(hash);
  }
  zbuf[zsiz] = '\0';
  *sp = zsiz;
  return (char*)zbuf;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  char* zbuf = new char[size+2];
  char* wp = zbuf;
  *(wp++) = 'o';
  *(wp++) = mode;
  std::memcpy(wp, buf, size);
  *sp = size + 2;
  return zbuf;
#endif
}


/**
 * Decompress a serial data.
 */
char* LZO::decompress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_LZO
  _assert_(buf && size <= MEMMAXSIZ && sp);
  if (mode == CRC) {
    if (size < sizeof(uint32_t)) return NULL;
    uint32_t hash = readfixnum((const char*)buf + size - sizeof(hash), sizeof(hash));
    size -= sizeof(hash);
    if (lzo_crc32(0, (const lzo_bytep)buf, size) != hash) return NULL;
  }
  char* zbuf;
  lzo_uint zsiz;
  int32_t rat = 6;
  while (true) {
    zsiz = (size + 256) * rat + 3;
    zbuf = new char[zsiz+1];
    int32_t rv;
    if (mode == RAW) {
      rv = lzo1x_decompress_safe((lzo_bytep)buf, size, (lzo_bytep)zbuf, &zsiz, NULL);
    } else {
      rv = lzo1x_decompress((lzo_bytep)buf, size, (lzo_bytep)zbuf, &zsiz, NULL);
    }
    if (rv == LZO_E_OK) {
      break;
    } else if (rv == LZO_E_OUTPUT_OVERRUN) {
      delete[] zbuf;
      rat *= 2;
    } else {
      delete[] zbuf;
      return NULL;
    }
  }
  zbuf[zsiz] = '\0';
  if (sp) *sp = zsiz;
  return (char*)zbuf;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  if (size < 2 || ((char*)buf)[0] != 'o' || ((char*)buf)[1] != mode) return NULL;
  buf = (char*)buf + 2;
  size -= 2;
  char* zbuf = new char[size+1];
  std::memcpy(zbuf, buf, size);
  zbuf[size] = '\0';
  *sp = size;
  return zbuf;
#endif
}


/**
 * Calculate the CRC32 checksum of a serial data.
 */
uint32_t LZO::calculate_crc(const void* buf, size_t size, uint32_t seed) {
#if _KC_LZO
  _assert_(buf && size <= MEMMAXSIZ);
  return lzo_crc32(seed, (const lzo_bytep)buf, size);
#else
  _assert_(buf && size <= MEMMAXSIZ);
  return 0;
#endif
}


/**
 * Compress a serial data.
 */
char* LZMA::compress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_LZMA
  _assert_(buf && size <= MEMMAXSIZ && sp);
  lzma_stream zs = LZMA_STREAM_INIT;
  const char* rp = (const char*)buf;
  size_t zsiz = size + 1024;
  char* zbuf = new char[zsiz+1];
  char* wp = zbuf;
  zs.next_in = (const uint8_t*)rp;
  zs.avail_in = size;
  zs.next_out = (uint8_t*)wp;
  zs.avail_out = zsiz;
  switch (mode) {
    default: {
      if (lzma_easy_encoder(&zs, 6, LZMA_CHECK_NONE) != LZMA_OK) return NULL;
      break;
    }
    case CRC: {
      if (lzma_easy_encoder(&zs, 6, LZMA_CHECK_CRC32) != LZMA_OK) return NULL;
      break;
    }
    case SHA: {
      if (lzma_easy_encoder(&zs, 6, LZMA_CHECK_SHA256) != LZMA_OK) return NULL;
      break;
    }
  }
  if (lzma_code(&zs, LZMA_FINISH) != LZMA_STREAM_END) {
    delete[] zbuf;
    lzma_end(&zs);
    return NULL;
  }
  lzma_end(&zs);
  zsiz -= zs.avail_out;
  *sp = zsiz;
  return zbuf;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  char* zbuf = new char[size+2];
  char* wp = zbuf;
  *(wp++) = 'x';
  *(wp++) = mode;
  std::memcpy(wp, buf, size);
  *sp = size + 2;
  return zbuf;
#endif
}


/**
 * Decompress a serial data.
 */
char* LZMA::decompress(const void* buf, size_t size, size_t* sp, Mode mode) {
#if _KC_LZMA
  _assert_(buf && size <= MEMMAXSIZ && sp);
  size_t zsiz = size * 8 + 32;
  while (true) {
    lzma_stream zs = LZMA_STREAM_INIT;
    const char* rp = (const char*)buf;
    char* zbuf = new char[zsiz+1];
    char* wp = zbuf;
    zs.next_in = (const uint8_t*)rp;
    zs.avail_in = size;
    zs.next_out = (uint8_t*)wp;
    zs.avail_out = zsiz;
    if (lzma_auto_decoder(&zs, 1ULL << 30, 0) != LZMA_OK) return NULL;
    int32_t rv = lzma_code(&zs, LZMA_FINISH);
    lzma_end(&zs);
    if (rv == LZMA_STREAM_END) {
      zsiz -= zs.avail_out;
      zbuf[zsiz] = '\0';
      *sp = zsiz;
      return zbuf;
    } else if (rv == LZMA_OK) {
      delete[] zbuf;
      zsiz *= 2;
    } else {
      delete[] zbuf;
      break;
    }
  }
  return NULL;
#else
  _assert_(buf && size <= MEMMAXSIZ && sp);
  if (size < 2 || ((char*)buf)[0] != 'x' || ((char*)buf)[1] != mode) return NULL;
  buf = (char*)buf + 2;
  size -= 2;
  char* zbuf = new char[size+1];
  std::memcpy(zbuf, buf, size);
  zbuf[size] = '\0';
  *sp = size;
  return zbuf;
#endif
}


/**
 * Calculate the CRC32 checksum of a serial data.
 */
uint32_t LZMA::calculate_crc(const void* buf, size_t size, uint32_t seed) {
#if _KC_LZMA
  _assert_(buf && size <= MEMMAXSIZ);
  return lzma_crc32((const uint8_t*)buf, size, seed);
#else
  _assert_(buf && size <= MEMMAXSIZ);
  return 0;
#endif
}


/**
 * Prepared pointer of the ZLIB raw mode.
 */
ZLIBCompressor<ZLIB::RAW> zlibrawfunc;
ZLIBCompressor<ZLIB::RAW>* const ZLIBRAWCOMP = &zlibrawfunc;


}                                        // common namespace

// END OF FILE
