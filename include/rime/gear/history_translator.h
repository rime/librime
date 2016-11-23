//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2016-09-08 osfans <waxaca@163.com>
//
#ifndef RIME_HISTORY_TRANSLATOR_H_
#define RIME_HISTORY_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {

class HistoryTranslator : public Translator {
 public:
  HistoryTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                const Segment& segment);

 protected:
  string tag_;
  string input_;
  int size_;
  double initial_quality_;
};

}  // namespace rime

#endif  // RIME_HISTORY_TRANSLATOR_H_
