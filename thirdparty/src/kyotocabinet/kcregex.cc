/*************************************************************************************************
 * Regular expression
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


#include "kcregex.h"
#include "myconf.h"

#if _KC_PXREGEX
extern "C" {
#include <regex.h>
}
#else
#include <regex>
#endif


namespace kyotocabinet {                 // common namespace


/**
 * Regex internal.
 */
struct RegexCore {
#if _KC_PXREGEX
  ::regex_t rbuf;
  bool alive;
  bool nosub;
#else
  std::regex* rbuf;
#endif
};


/**
 * Default constructor.
 */
Regex::Regex() : opq_(NULL) {
#if _KC_PXREGEX
  _assert_(true);
  RegexCore* core = new RegexCore;
  core->alive = false;
  core->nosub = false;
  opq_ = (void*)core;
#else
  _assert_(true);
  RegexCore* core = new RegexCore;
  core->rbuf = NULL;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
Regex::~Regex() {
#if _KC_PXREGEX
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (core->alive) ::regfree(&core->rbuf);
  delete core;
#else
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  delete core->rbuf;
  delete core;
#endif
}


/**
 * Compile a string of regular expression.
 */
bool Regex::compile(const std::string& regex, uint32_t opts) {
#if _KC_PXREGEX
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (core->alive) {
    ::regfree(&core->rbuf);
    core->alive = false;
  }
  int32_t cflags = REG_EXTENDED;
  if (opts & IGNCASE) cflags |= REG_ICASE;
  if ((opts & MATCHONLY) || regex.empty()) {
    cflags |= REG_NOSUB;
    core->nosub = true;
  }
  if (::regcomp(&core->rbuf, regex.c_str(), cflags) != 0) return false;
  core->alive = true;
  return true;
#else
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (core->rbuf) {
    delete core->rbuf;
    core->rbuf = NULL;
  }
  int32_t cflags = std::regex::ECMAScript;
  if (opts & IGNCASE) cflags |= std::regex::icase;
  if ((opts & MATCHONLY) || regex.empty()) cflags |= std::regex::nosubs;
  try {
    core->rbuf = new std::regex(regex, (std::regex::flag_type)cflags);
  } catch (...) {
    core->rbuf = NULL;
    return false;
  }
  return true;
#endif
}


/**
 * Check whether a string matches the regular expression.
 */
bool Regex::match(const std::string& str) {
#if _KC_PXREGEX
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (!core->alive) return false;
  if (core->nosub) return ::regexec(&core->rbuf, str.c_str(), 0, NULL, 0) == 0;
  ::regmatch_t subs[1];
  return ::regexec(&core->rbuf, str.c_str(), 1, subs, 0) == 0;
#else
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (!core->rbuf) return false;
  std::smatch res;
  return std::regex_search(str, res, *core->rbuf);
#endif
}


/**
 * Check whether a string matches the regular expression.
 */
std::string Regex::replace(const std::string& str, const std::string& alt) {
#if _KC_PXREGEX
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (!core->alive || core->nosub) return str;
  regmatch_t subs[256];
  if (::regexec(&core->rbuf, str.c_str(), sizeof(subs) / sizeof(*subs), subs, 0) != 0)
    return str;
  const char* sp = str.c_str();
  std::string xstr;
  bool first = true;
  while (sp[0] != '\0' && ::regexec(&core->rbuf, sp, 10, subs, first ? 0 : REG_NOTBOL) == 0) {
    first = false;
    if (subs[0].rm_so == -1) break;
    xstr.append(sp, subs[0].rm_so);
    for (const char* rp = alt.c_str(); *rp != '\0'; rp++) {
      if (*rp == '$') {
        if (rp[1] >= '0' && rp[1] <= '9') {
          int32_t num = rp[1] - '0';
          if (subs[num].rm_so != -1 && subs[num].rm_eo != -1)
            xstr.append(sp + subs[num].rm_so, subs[num].rm_eo - subs[num].rm_so);
          ++rp;
        } else if (rp[1] == '&') {
          xstr.append(sp + subs[0].rm_so, subs[0].rm_eo - subs[0].rm_so);
          ++rp;
        } else if (rp[1] != '\0') {
          xstr.append(++rp, 1);
        }
      } else {
        xstr.append(rp, 1);
      }
    }
    sp += subs[0].rm_eo;
    if (subs[0].rm_eo < 1) break;
  }
  xstr.append(sp);
  return xstr;
#else
  _assert_(true);
  RegexCore* core = (RegexCore*)opq_;
  if (!core->rbuf) return str;
  return std::regex_replace(str, *core->rbuf, alt);
#endif
}


}                                        // common namespace

// END OF FILE
