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


#ifndef _KCREGEX_H                       // duplication check
#define _KCREGEX_H

#include <kccommon.h>
#include <kcutil.h>

namespace kyotocabinet {                 // common namespace


/**
 * Regular expression.
 */
class Regex {
 public:
  /**
   * Options.
   */
  enum Option {
    IGNCASE = 1 << 0,                    ///< case-insensitive
    MATCHONLY = 1 << 1,                  ///< matching only
  };
  /**
   * Default constructor.
   */
  explicit Regex();
  /**
   * Destructor.
   */
  ~Regex();
  /**
   * Compile a string of regular expression.
   * @param regex the string of regular expression.
   * @param opts the optional features by bitwise-or: Regex::IGNCASE for case-insensitive
   * matching, Regex::MATCHONLY for matching only usage.
   */
  bool compile(const std::string& regex, uint32_t opts = 0);
  /**
   * Check whether a string matches the regular expression.
   * @param str the string.
   * @return true if the string matches, or false if not.
   */
  bool match(const std::string& str);
  /**
   * Check whether a string matches the regular expression.
   * @param str the string.
   * @param alt the alternative string with which each substring is replaced.  Each "$" in the
   * string escapes the following character.  Special escapes "$1" through "$9" refer to partial
   * substrings corresponding to sub-expressions in the regular expression.  "$0" and "$&" refer
   * to the whole matching substring.
   * @return the result string.
   */
  std::string replace(const std::string& str, const std::string& alt);
  /**
   * Check whether a string matches a regular expression.
   * @param str the string.
   * @param pattern the matching pattern.
   * @param opts the optional features by bitwise-or: Regex::IGNCASE for case-insensitive
   * matching, Regex::MATCHONLY for matching only usage.
   * @return true if the string matches, or false if not.
   */
  static bool match(const std::string& str, const std::string& pattern, uint32_t opts = 0) {
    Regex regex;
    if (!regex.compile(pattern, opts)) return false;
    return regex.match(str);
  }
  /**
   * Check whether a string matches the regular expression.
   * @param str the string.
   * @param pattern the matching pattern.
   * @param alt the alternative string with which each substring is replaced.  Each "$" in the
   * string escapes the following character.  Special escapes "$1" through "$9" refer to partial
   * substrings corresponding to sub-expressions in the regular expression.  "$0" and "$&" refer
   * to the whole matching substring.
   * @param opts the optional features by bitwise-or: Regex::IGNCASE for case-insensitive
   * matching, Regex::MATCHONLY for matching only usage.
   * @return the result string.
   */
  static std::string replace(const std::string& str, const std::string& pattern,
                             const std::string& alt, uint32_t opts = 0) {
    Regex regex;
    if (!regex.compile(pattern, opts)) return str;
    return regex.replace(str, alt);
  }
 private:
  /** Dummy constructor to forbid the use. */
  Regex(const Regex&);
  /** Dummy Operator to forbid the use. */
  Regex& operator =(const Regex&);
  /** Opaque pointer. */
  void* opq_;
};


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
