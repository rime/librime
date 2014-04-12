//
// Copyleft RIME Developers
// License: GPLv3
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/gear/cjk_minifier.h>

namespace rime {

bool is_extended_cjk(uint32_t ch)
{
  // ripoff from translator_commons.cc
  if ((ch >= 0x3400 && ch <= 0x4DBF) ||    // CJK Unified Ideographs Extension A
      (ch >= 0x20000 && ch <= 0x2A6DF) ||  // CJK Unified Ideographs Extension B
      (ch >= 0x2A700 && ch <= 0x2B73F) ||  // CJK Unified Ideographs Extension C
      (ch >= 0x2B740 && ch <= 0x2B81F))    // CJK Unified Ideographs Extension D
    return true;

  return false;
}

bool contains_extended_cjk(const std::string &text)
{
  const char *p = text.c_str();
  uint32_t ch;

  while ((ch = utf8::unchecked::next(p)) != 0) {
    if (is_extended_cjk(ch)) {
      return true;
    }
  }

  return false;
}

// CJKMinifier

CJKMinifier::CJKMinifier(const Ticket& ticket) : Filter(ticket) {
}

void CJKMinifier::Apply(CandidateList* recruited,
                        CandidateList* candidates) {
  if (engine_->context()->get_option("extended_charset"))
    return;

  if (!candidates || candidates->empty())
    return;

  for (auto it = candidates->rbegin(); it != candidates->rend(); ++it) {
    if (contains_extended_cjk((*it)->text())) {
      candidates->erase(std::next(it).base());
    }
  }
}

}  // namespace rime
