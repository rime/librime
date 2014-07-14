/*************************************************************************************************
 * Comparator functions
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


#ifndef _KCCOMPARE_H                     // duplication check
#define _KCCOMPARE_H

#include <kccommon.h>
#include <kcutil.h>

namespace kyotocabinet {                 // common namespace


/**
 * Interfrace of comparator of record keys.
 */
class Comparator {
 public:
  /**
   * Destructor.
   */
  virtual ~Comparator() {}
  /**
   * Compare two keys.
   * @param akbuf the pointer to the region of one key.
   * @param aksiz the size of the region of one key.
   * @param bkbuf the pointer to the region of the other key.
   * @param bksiz the size of the region of the other key.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  virtual int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) = 0;
};


/**
 * Comparator in the lexical order.
 */
class LexicalComparator : public Comparator {
 public:
  explicit LexicalComparator() {}
  int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    _assert_(akbuf && bkbuf);
    size_t msiz = aksiz < bksiz ? aksiz : bksiz;
    for (size_t i = 0; i < msiz; i++) {
      if (((uint8_t*)akbuf)[i] != ((uint8_t*)bkbuf)[i])
        return ((uint8_t*)akbuf)[i] - ((uint8_t*)bkbuf)[i];
    }
    return (int32_t)aksiz - (int32_t)bksiz;
  }
};


/**
 * Comparator in the lexical descending order.
 */
class LexicalDescendingComparator : public Comparator {
 public:
  explicit LexicalDescendingComparator() : comp_() {}
  int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    return -comp_.compare(akbuf, aksiz, bkbuf, bksiz);
  }
 private:
  LexicalComparator comp_;
};


/**
 * Comparator in the decimal order.
 */
class DecimalComparator : public Comparator {
 public:
  explicit DecimalComparator() {}
  int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    _assert_(akbuf && bkbuf);
    const int32_t LDBLCOLMAX = 16;
    const unsigned char* arp = (unsigned char*)akbuf;
    int32_t alen = aksiz;
    while (alen > 0 && (*arp <= ' ' || *arp == 0x7f)) {
      arp++;
      alen--;
    }
    int64_t anum = 0;
    int32_t asign = 1;
    if (alen > 0 && *arp == '-') {
      arp++;
      alen--;
      asign = -1;
    }
    while (alen > 0) {
      int32_t c = *arp;
      if (c < '0' || c > '9') break;
      anum = anum * 10 + c - '0';
      arp++;
      alen--;
    }
    anum *= asign;
    const unsigned char* brp = (unsigned char*)bkbuf;
    int32_t blen = bksiz;
    while (blen > 0 && (*brp <= ' ' || *brp == 0x7f)) {
      brp++;
      blen--;
    }
    int64_t bnum = 0;
    int32_t bsign = 1;
    if (blen > 0 && *brp == '-') {
      brp++;
      blen--;
      bsign = -1;
    }
    while (blen > 0) {
      int32_t c = *brp;
      if (c < '0' || c > '9') break;
      bnum = bnum * 10 + c - '0';
      brp++;
      blen--;
    }
    bnum *= bsign;
    if (anum < bnum) return -1;
    if (anum > bnum) return 1;
    if ((alen > 1 && *arp == '.') || (blen > 1 && *brp == '.')) {
      long double aflt = 0;
      if (alen > 1 && *arp == '.') {
        arp++;
        alen--;
        if (alen > LDBLCOLMAX) alen = LDBLCOLMAX;
        long double base = 10;
        while (alen > 0) {
          if (*arp < '0' || *arp > '9') break;
          aflt += (*arp - '0') / base;
          arp++;
          alen--;
          base *= 10;
        }
        aflt *= asign;
      }
      long double bflt = 0;
      if (blen > 1 && *brp == '.') {
        brp++;
        blen--;
        if (blen > LDBLCOLMAX) blen = LDBLCOLMAX;
        long double base = 10;
        while (blen > 0) {
          if (*brp < '0' || *brp > '9') break;
          bflt += (*brp - '0') / base;
          brp++;
          blen--;
          base *= 10;
        }
        bflt *= bsign;
      }
      if (aflt < bflt) return -1;
      if (aflt > bflt) return 1;
    }
    LexicalComparator lexcomp;
    int32_t rv = lexcomp.compare(akbuf, aksiz, bkbuf, bksiz);
    return rv;
  }
};


/**
 * Comparator in the decimal descending order.
 */
class DecimalDescendingComparator : public Comparator {
 public:
  explicit DecimalDescendingComparator() : comp_() {}
  int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    return -comp_.compare(akbuf, aksiz, bkbuf, bksiz);
  }
 private:
  DecimalComparator comp_;
};


/**
 * Prepared pointer of the comparator in the lexical order.
 */
extern LexicalComparator* const LEXICALCOMP;


/**
 * Prepared pointer of the comparator in the lexical descending order.
 */
extern LexicalDescendingComparator* const LEXICALDESCCOMP;


/**
 * Prepared pointer of the comparator in the decimal order.
 */
extern DecimalComparator* const DECIMALCOMP;


/**
 * Prepared pointer of the comparator in the decimal descending order.
 */
extern DecimalDescendingComparator* const DECIMALDESCCOMP;


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
