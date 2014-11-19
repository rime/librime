//
// Copyleft RIME Developers
// License: GPLv3
//
// 2014-11-19 Chen Gong <chen.sst@gmail.com>
//
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/gear/single_char_filter.h>
#include <rime/gear/translator_commons.h>

namespace rime {

SingleCharFilter::SingleCharFilter(const Ticket& ticket)
    : Filter(ticket) {
}

static inline size_t unistrlen(const std::string& text) {
  return utf8::unchecked::distance(
      text.c_str(), text.c_str() + text.length());
}

void SingleCharFilter::Apply(CandidateList* recruited,
                             CandidateList* candidates) {
  size_t insert_pos = recruited->size();
  for (auto rit = recruited->rbegin(); rit != recruited->rend(); ) {
    auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(*rit));
    if (!phrase || phrase->type() != "table") {
      break;
    }
    if (unistrlen((*rit)->text()) == 1) {
      break;
    }
    ++rit;
    insert_pos = rit.base() - recruited->begin();
  }
  if (insert_pos == recruited->size()) {
    return;
  }
  for (auto it = candidates->begin(); it != candidates->end(); ) {
    auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(*it));
    if (!phrase || phrase->type() != "table") {
      break;
    }
    if (unistrlen((*it)->text()) == 1) {
      recruited->insert(recruited->begin() + insert_pos, *it);
      ++insert_pos;
      candidates->erase(it);
    }
    else {
      ++it;
    }
  }
}

}  // namespace rime
