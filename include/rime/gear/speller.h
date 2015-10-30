//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SPELLER_H_
#define RIME_SPELLER_H_

#include <boost/regex.hpp>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Context;
struct Segment;

class Speller : public Processor {
 public:
  Speller(const Ticket& ticket);

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  enum AutoClearMethod { kClearNone, kClearAuto, kClearManual, kClearMaxLength };

  bool AutoSelectAtMaxCodeLength(Context* ctx);
  bool AutoSelectUniqueCandidate(Context* ctx);
  bool AutoSelectPreviousMatch(Context* ctx, Segment* previous_segment);
  bool FindEarlierMatch(Context* ctx, size_t start, size_t end);
  bool AutoClear(Context* ctx);

  string alphabet_;
  string delimiters_;
  string initials_;
  string finals_;
  int max_code_length_ = 0;
  bool auto_select_ = false;
  bool use_space_ = false;
  boost::regex auto_select_pattern_;
  AutoClearMethod auto_clear_ = kClearNone;
};

}  // namespace rime

#endif  // RIME_SPELLER_H_
