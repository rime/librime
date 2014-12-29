//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#ifndef RIME_CJK_MINIFIER_H_
#define RIME_CJK_MINIFIER_H_

#include <stdint.h> // for uint32_t
#include <string>
#include <rime/filter.h>

namespace rime {

bool is_extended_cjk(uint32_t ch);
bool contains_extended_cjk(const std::string &text);

class CJKMinifier : public Filter {
 public:
  explicit CJKMinifier(const Ticket& ticket);

  virtual void Apply(CandidateList* recruited,
                     CandidateList* candidates);
};

}  // namespace rime

#endif  // RIME_CJK_MINIFIER_H_
