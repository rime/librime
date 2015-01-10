//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#ifndef RIME_CHARSET_FILTER_H_
#define RIME_CHARSET_FILTER_H_

#include <string>
#include <rime/filter.h>
#include <rime/translation.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class CharsetFilterTranslation : public Translation {
 public:
  CharsetFilterTranslation(shared_ptr<Translation> translation);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();

 protected:
  bool LocateNextCandidate();

  shared_ptr<Translation> translation_;
};

struct DictEntry;

class CharsetFilter : public Filter, TagMatching {
 public:
  explicit CharsetFilter(const Ticket& ticket);

  virtual shared_ptr<Translation> Apply(shared_ptr<Translation> translation,
                                        CandidateList* candidates);

  virtual bool AppliesToSegment(Segment* segment) {
    return TagsMatch(segment);
  }

  // return true to accept, false to reject the tested item
  static bool FilterText(const std::string& text);
  static bool FilterDictEntry(shared_ptr<DictEntry> entry);
};

}  // namespace rime

#endif  // RIME_CHARSET_FILTER_H_
